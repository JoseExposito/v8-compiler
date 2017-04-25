
**Follow v8-compiler on...** [![](https://lh3.googleusercontent.com/-kRgKvb-T4_4/T9psNwZN3TI/AAAAAAAAANA/pwasxapdWm0/s33/twitter.png "Twitter")](https://twitter.com/#!/Jose__Exposito)


# v8-compiler

v8-compiler allows you to compile your Node.js project and distribute it without source code.


## Code protection

v8-compiler uses the v8 cache representation to generate binary files Node.js can execute without the source code.


## No patching

v8-compiler does not patch Node.js or v8. It is just a C++ addon that you can use in your project without more complications.


## How to compile it?

```
$ git clone https://github.com/JoseExposito/v8-compiler.git
$ cd v8-compiler
$ npm install # Ignore build errors, we will build in the next commands
$ npm run clean
$ npm run --v8-compiler:target=v7.8.0 download-node-source
$ npm run --v8-compiler:target=v7.8.0 --v8-compiler:arch=x64 configure
$ npm run --v8-compiler:target=v7.8.0 --v8-compiler:arch=x64 build
```

Note that you can compile v8-compiler targeting the Node.js version you want by changing `--v8-compiler:target=vX.X.X`.


## Is Electron supported?

Not yet, I'm working on it as my highest priority:
https://github.com/electron/electron/issues/9275

So please, do not use the npm `electron:configure` and `electron:build` yet.


## How do I use v8-compiler?

Check the `examples` folder to know how to use it.


## Is the generated compiled file portable?

Binaries generated with v8-compiler can be executed in machines using the same operating system, processor architecture and Node.js version.
Cross compilation is not supported.
