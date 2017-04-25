/*
 * This file is part of v8-compiler.
 *
 * v8-compiler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * v8-compiler is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with v8-compiler.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <node.h>
#include <nan.h>

// v8
#include <v8.h>
#include <libplatform/libplatform.h>

// v8 internal headers
#include "src/compilation-cache.h"
#include "src/code-stubs.h"
#include "src/snapshot/code-serializer.h"
#include "src/snapshot/deserializer.h"


namespace v8_compiler {

/*!
 This function is a copy of CodeSerializer::Deserialize (v8/src/snapshot/code-serializer.cc) but allowing to set the source hash from the outside.
 This allows to deserialize the cached data into a i::SharedFunctionInfo object and add it to the iIsolate->compilation_cache() so it does not
 get recompiled when executing it.
 Note: Make sure the C/C++ flag "-fno-rtti" is set in order to compile this.
 */
i::MaybeHandle<i::SharedFunctionInfo> myDeserialize(i::Isolate *isolate, i::ScriptData *cached_data, i::Handle<i::String> source, uint32_t sourceHash) {
    i::HandleScope scope(isolate);

    i::SerializedCodeData::SanityCheckResult sanity_check_result = i::SerializedCodeData::CHECK_SUCCESS;
    const i::SerializedCodeData scd = i::SerializedCodeData::FromCachedData(isolate, cached_data, sourceHash, &sanity_check_result);

    if (sanity_check_result != i::SerializedCodeData::CHECK_SUCCESS) {
        return i::MaybeHandle<i::SharedFunctionInfo>();
    }

    i::Deserializer deserializer(&scd);
    deserializer.AddAttachedObject(source);
    i::Vector<const uint32_t> code_stub_keys = scd.CodeStubKeys();
    for (int i = 0; i < code_stub_keys.length(); i++) {
        deserializer.AddAttachedObject(i::CodeStub::GetCode(isolate, code_stub_keys[i]).ToHandleChecked());
    }

    // Deserialize.
    i::Handle<i::HeapObject> as_heap_object;
    if (!deserializer.DeserializeObject(isolate).ToHandle(&as_heap_object)) {
        // Deserializing may fail if the reservations cannot be fulfilled.
        return i::MaybeHandle<i::SharedFunctionInfo>();
    }

    i::Handle<i::SharedFunctionInfo> result = i::Handle<i::SharedFunctionInfo>::cast(as_heap_object);

    result->set_deserialized(true);

    return scope.CloseAndEscape(result);
}

/*!
 Compiles a script.
 @param  {String} JavaScript code to compile
 @return {Buffer} Compiled code in binary format
 */
void compileScript(const v8::FunctionCallbackInfo<v8::Value> &args) {
    v8::Isolate *isolate  = args.GetIsolate();
    i::Isolate  *iIsolate = reinterpret_cast<i::Isolate*>(isolate);

    // Arguments sanity check
    if (args.Length() != 1 || !args[0]->IsString()) {
        const char *error = "Wrong arguments: v8-compiler::compileScript only accepts one argument of type String";
        isolate->ThrowException(v8::Exception::TypeError(v8::String::NewFromUtf8(isolate, error)));
        return;
    }

    // Compile the code
    v8::Local<v8::String> scriptSourceString  = args[0]->ToString();
    i::Handle<i::String>  iScriptSourceString = iIsolate->factory()->NewStringFromUtf8(i::CStrVector(*v8::String::Utf8Value(scriptSourceString))).ToHandleChecked();

    v8::TryCatch exceptionHandler(isolate);

    v8::ScriptCompiler::Source sourceObject(scriptSourceString);
    v8::ScriptCompiler::Compile(isolate->GetCurrentContext(), &sourceObject, v8::ScriptCompiler::kProduceCodeCache).ToLocalChecked();
    const v8::ScriptCompiler::CachedData *cache = sourceObject.GetCachedData();

    if (exceptionHandler.HasCaught()) {
        const char *error = "Error compiling your script, please make sure it does not have any errors";
        isolate->ThrowException(v8::Exception::TypeError(v8::String::NewFromUtf8(isolate, error)));
        return;
    }

    // Write the compiled script to a Node.js Buffer
    // A compiled binary has the format:
    // - Source Hash (uint32_t)
    // - Cache
    uint32_t sourceHash = i::SerializedCodeData::SourceHash(iScriptSourceString);

    uint32_t bufferSize = sizeof(sourceHash) + cache->length;
    char *buffer = new char[bufferSize];

    memcpy(buffer, &sourceHash, sizeof(sourceHash));
    memcpy(buffer+sizeof(sourceHash), cache->data, cache->length);

    args.GetReturnValue().Set(Nan::NewBuffer(buffer, bufferSize).ToLocalChecked());
}

/*!
 Runs a compiled script.
 @param  {Buffer} Compiled script binary buffer
 @return {String} Output returned by the run script
 */
void runScript(const v8::FunctionCallbackInfo<v8::Value> &args) {
    v8::Isolate *isolate  = args.GetIsolate();
    i::Isolate  *iIsolate = reinterpret_cast<i::Isolate*>(isolate);

    // Arguments sanity check
    if (args.Length() != 1 || !args[0]->IsObject()) {
        const char *error = "Wrong arguments: v8-compiler::runScript only accepts one argument of type Buffer";
        isolate->ThrowException(v8::Exception::TypeError(v8::String::NewFromUtf8(isolate, error)));
        return;
    }

    // Parse the compiled script binary buffer
    char *buffer = node::Buffer::Data(args[0]->ToObject());
    uint32_t bufferSize = node::Buffer::Length(args[0]->ToObject());

    uint32_t sourceHash;
    memcpy(&sourceHash, buffer, sizeof(sourceHash));

    int cacheSize = static_cast<int>(bufferSize - sizeof(sourceHash));
    const uint8_t *cachePointer = reinterpret_cast<const uint8_t *>(buffer + sizeof(sourceHash));

    // Load the cached data
    v8::ScriptCompiler::CachedData *cache = new v8::ScriptCompiler::CachedData(cachePointer, cacheSize, v8::ScriptCompiler::CachedData::BufferNotOwned);
    i::ScriptData *scriptData = new i::ScriptData(cachePointer, cacheSize);

    // Create an empty script source code
    v8::Local<v8::String> sourceString  = v8::String::NewFromUtf8(isolate, "");
    i::Handle<i::String>  iSourceString = iIsolate->factory()->NewStringFromAsciiChecked("");

    // Parse the cached data
    i::Handle<i::SharedFunctionInfo> compilationCacheResult = myDeserialize(iIsolate, scriptData, iSourceString, sourceHash).ToHandleChecked();

    // Add the compiled script to the compilation cache so it doesn't get recompiled
    i::CompilationCache *compilationCache = iIsolate->compilation_cache();
    i::LanguageMode languageMode = i::construct_language_mode(i::FLAG_use_strict);
    compilationCache->PutScript(iSourceString, v8::Utils::OpenHandle(*isolate->GetCurrentContext()), languageMode, compilationCacheResult);

    if (scriptData->rejected()) {
        isolate->ThrowException(v8::Exception::TypeError(v8::String::NewFromUtf8(isolate, "Error runnning the script: Cache rejected")));
        return;
    }

    // Compile the script and run it
    v8::ScriptCompiler::Source scriptSource(sourceString, cache);
    v8::Local<v8::Script> finalScript = v8::ScriptCompiler::Compile(isolate->GetCurrentContext(), &scriptSource, v8::ScriptCompiler::kConsumeCodeCache).ToLocalChecked();
    v8::Handle<v8::Value> result = finalScript->Run();

    args.GetReturnValue().Set(result);
}


void init(v8::Local<v8::Object> exports, v8::Local<v8::Object> module) {
    // This flags are required to get the cache working
    i::FLAG_lazy                = false;
    i::FLAG_serialize_toplevel  = true;
    i::FLAG_log_code            = true;
    i::FLAG_logfile_per_isolate = false;
    i::FLAG_harmony_shipping    = false;

    // Make the "require" function available to the JavaScript compiled code by adding it to the global context.
    // https://nodejs.org/api/globals.html
    // To make modules in node_modules available to the compiled code, set the environment variable NODE_PATH
    v8::Isolate *isolate = exports->GetIsolate();
    v8::Local<v8::String> requireFunctionName = v8::String::NewFromUtf8(isolate, "require");
    isolate->GetCurrentContext()->Global()->Set(requireFunctionName, module->Get(requireFunctionName));

    // Export the addon methods
    NODE_SET_METHOD(exports, "compileScript", compileScript);
    NODE_SET_METHOD(exports, "runScript", runScript);
}

NODE_MODULE(v8_compiler, init)

}  // namespace v8_compiler
