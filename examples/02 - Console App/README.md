
## About the example

This example illustrates how to use v8-compiler to compile a command line tool with the source code split in 2 different files.

As v8-compiler is only able to compile a single file, this example uses Webpack to merge the source code in a single file before compiling it.


## Why Webpack?

You can use your favorite tool to bundle your code before compiling it.
My favorite one is Webpack, that's why I'm using it :)


## Why v8-compiler doesn't bundle the files automatically?

Webpack and its alternatives are excellent tools, well documented and supported by a large community, so, why should I reinvent the wheel?


## How to run it

The first step is to install the dependencies:

```
$ npm install
```

Next, bundle the code together using Webpack:

```
$ node webpack-build.js
```

`webpack-build.js` is good example of a very basic Webpack configuration file that you can extend in your own project if you are not using Webpack already.
This command will generate the bundled code in `build/bundle.js`.

Now, we can use v8-compiler to compile the generated bundle:

```
$ v8-compiler compile --in build/bundle.js --out build/console-app-example.jsc
```

Finally, run it:

```
$ v8-compiler run build/console-app-example.jsc
```

Check example 01 to get a more detailed explanation of the v8-compiler command line tool.
