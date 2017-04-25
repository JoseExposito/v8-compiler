const webpack           = require('webpack')
const path              = require('path')

const webpackConfig = {
    target: 'node',
    node: { __dirname: true },
    entry: './main.js',
    output: {
        path: path.join(__dirname, 'build'),
        filename: 'bundle.js'
    }
    /*,

    module: {
        loaders: [
            { test: /(\.css)$/, loader: ExtractTextPlugin.extract('css-loader') },
            { test: /(\.pug)$/, loader: 'pug-loader' },
            { test: /\.(woff|png|jpg|gif)$/, loader: 'url-loader?limit=10000' }
        ]
    }*/
}

webpack(webpackConfig).run((err, stats) => {
    if (err) {
        console.error(err)
        return 1
    }

    let jsonStats = stats.toJson()
    if (jsonStats.hasErrors) {
        jsonStats.errors.map(error => console.error(error))
        return 1
    }

    if (jsonStats.hasWarnings) {
        console.log('Webpack warnings:')
        jsonStats.warnings.map(warning => console.log(warning))
    }

    console.log('Webpack bundle generated')
    return 0
})
