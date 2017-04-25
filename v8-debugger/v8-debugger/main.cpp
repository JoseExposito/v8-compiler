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
 #include <iostream>

// v8
#include <v8.h>
#include <libplatform/libplatform.h>

// v8 internal headers
#include "src/compilation-cache.h"
#include "src/code-stubs.h"
#include "src/snapshot/code-serializer.h"
#include "src/snapshot/deserializer.h"

using namespace v8;


class ArrayBufferAllocator : public v8::ArrayBuffer::Allocator {
public:

    virtual void* Allocate(size_t length) {
        void* data = AllocateUninitialized(length);
        return data == NULL ? data : memset(data, 0, length);
    }

    virtual void* AllocateUninitialized(size_t length) { return malloc(length); }
    virtual void Free(void* data, size_t) { free(data); }
};


/*!
 This function is a copy of CodeSerializer::Deserialize (v8/src/snapshot/code-serializer.cc) but allowing to set the source hash from the outside.
 This allows to deserialize the cached data into a i::SharedFunctionInfo object and add it to the iIsolate->compilation_cache() so it does not
 get recompiled when executing it.
 Note: Make sure the C/C++ flag "-fno-rtti" is set in order to compile this.
 */
i::MaybeHandle<i::SharedFunctionInfo> myDeserialize(i::Isolate *isolate, i::ScriptData *cached_data, i::Handle<i::String> source, uint32_t sourceHash) {
    //i::HandleScope scope(isolate);

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

    //result->set_deserialized(true); // This line breaks the compilation here, but not in v8-compiler

    //return scope.CloseAndEscape(result);
    return result;
}

int main(int argc, const char * argv[]) {
    i::FLAG_lazy                = false;
    i::FLAG_serialize_toplevel  = true;
    i::FLAG_log_code            = true;
    i::FLAG_logfile_per_isolate = false;
    i::FLAG_harmony_shipping    = false;


    // Initialize V8
    V8::InitializeICU();
    V8::InitializeExternalStartupData(argv[0]);
    Platform *platform = platform::CreateDefaultPlatform();
    V8::InitializePlatform(platform);
    V8::Initialize();

    // Create a new Isolate and make it the current one.
    ArrayBufferAllocator allocator;
    v8::Isolate::CreateParams createParams;
    createParams.array_buffer_allocator = &allocator;

    v8::Isolate* isolate = v8::Isolate::New(createParams);
    {
        v8::Isolate::Scope isolateScope(isolate);

        // Create a stack-allocated handle scope
        v8::HandleScope handleScope(isolate);

        // Create a new context
        v8::Local<v8::Context> context = v8::Context::New(isolate);

        // Enter the context for compiling and running the script
        v8::Context::Scope contextScope(context);

        { // Cache to file
            i::Isolate *iIsolate = reinterpret_cast<i::Isolate*>(isolate);

            const char *scriptSourceCString = "function foo() { return 'Hello ' } function bar() { return 'World: ' } function baz() { let a = 1;let b = 2;let c = 300;return a + b + c } foo() + bar() + baz()";

            v8::Local<v8::String> scriptSourceString  = v8::String::NewFromUtf8(isolate, scriptSourceCString);
            i::Handle<i::String>  iScriptSourceString = iIsolate->factory()->NewStringFromUtf8(i::CStrVector(*v8::String::Utf8Value(scriptSourceString))).ToHandleChecked();

            v8::TryCatch exceptionHandler(isolate);

            v8::ScriptCompiler::Source sourceObject(scriptSourceString);
            v8::ScriptCompiler::Compile(isolate->GetCurrentContext(), &sourceObject, v8::ScriptCompiler::kProduceCodeCache).ToLocalChecked();
            const v8::ScriptCompiler::CachedData *cache = sourceObject.GetCachedData();

            if (exceptionHandler.HasCaught()) {
                std::cout << "Error compiling your script, please make sure it does not have any errors" << std::endl;
                return 1;
            }

            // Write the compiled script
            // A compiled binary has the format:
            // - Source Hash (uint32_t)
            // - Cache
            const char *filePath = "compiled.jsc";
            std::ofstream file;
            file.open(filePath, std::fstream::out | std::fstream::binary);

            if (!file.is_open()) {
                std::cout << "Error creating file " << filePath << std::endl;
                return 1;
            }

            // Write the source hash
            uint32_t sourceHash = i::SerializedCodeData::SourceHash(iScriptSourceString);
            std::cout << "Source Hash: " << sourceHash << std::endl;
            file.write(reinterpret_cast<const char *>(&sourceHash), sizeof(sourceHash));

            // And the cache
            file.write(reinterpret_cast<const char *>(cache->data), cache->length);
            file.close();
        }

        { // Cache from file
            i::Isolate *iIsolate = reinterpret_cast<i::Isolate*>(isolate);

            // Read the file
            const char *filePath = "compiled.jsc";
            std::ifstream file;
            file.open(filePath, std::fstream::in | std::fstream::binary);

            if (!file.is_open()) {
                std::cout << "Error reading file " << filePath << std::endl;
                return 1;
            }

            uint32_t sourceHash = 0;

            file.seekg(0, std::ios::end);
            size_t dataLength = file.tellg();
            dataLength -= sizeof(sourceHash);
            uint8_t *data = new uint8_t[dataLength];

            file.seekg(0, std::ios::beg);
            file.read(reinterpret_cast<char *>(&sourceHash), sizeof(sourceHash));
            file.read(reinterpret_cast<char *>(data), dataLength);
            file.close();

            // Load the cached data
            v8::ScriptCompiler::CachedData *cache = new v8::ScriptCompiler::CachedData(data, (int)dataLength, v8::ScriptCompiler::CachedData::BufferNotOwned);
            i::ScriptData *scriptData = new i::ScriptData(data, (int)dataLength);

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
                return 1;
            }

            // Compile the script and run it
            ScriptCompiler::Source scriptSource(sourceString, cache);
            v8::Local<v8::Script> finalScript = v8::ScriptCompiler::Compile(isolate->GetCurrentContext(), &scriptSource, v8::ScriptCompiler::kConsumeCodeCache).ToLocalChecked();
            v8::Handle<v8::Value> result = finalScript->Run();
            std::cout << *(v8::String::Utf8Value(result)) << std::endl;
        }
    }



    return 0;
}
