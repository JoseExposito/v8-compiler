
## About the v8-debugger

This is a Xcode project with a minimal source code that compiles a script and runs it.


## build-v8.sh

The `build-v8.sh` script compiles the specified v8 version in debug mode allowing you to easily debug it
to check possible problems in v8-compiler.

The script expects one argument with the target v8 tag version to compile. To know it, execute:

```
$ node -e "console.log(process.versions.v8)"
```

If necessary, link in the Xcode project all the generated libraries in `v8/out/x64.debug` and add the files
`natives_blob.bin` and `snapshot_blob.bin` to the copy files in `v8-debugger project file -> Build Phases`.
