
## About the example

This example illustrates how to use v8-compiler to compile a very basic Hello World script.


## How to run it

v8-compiler has two main options:

#### Compile

Compiles a JavaScript file and outputs the binary generated:

```
$ v8-compiler compile --in hello.js --out hello.jsc
```

You can choose any extension for the generated compiled file.


#### Run

Runs a compiled file:

```
$ v8-compiler run hello.jsc
```

Feel free to delete `hello.js` to see how `hello.jsc` runs the same program without the source code.
