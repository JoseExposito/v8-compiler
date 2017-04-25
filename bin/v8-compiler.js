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
'use strict'

const args     = require('commander')
const fs       = require('fs')
const version  = require('../package.json').version
const compiler = require('../build/Release/v8-compiler')

// Command line arguments
args.command('compile')
    .description('Compiles a JavaScript file')
    .option('-i, --in <path>', 'File to compile')
    .option('-o, --out <path>', 'Output compiled file path')
    .action(options => compile(options))

args.command('run <file>')
    .description('Runs a compiled binary file')
    .action(file => run(file))

args.version(version).parse(process.argv)



function compile(options) {
    if (!options.in) {
        console.error('ERROR: Specify the JavaScript file to compile with --in')
        return
    }

    if (!options.out) {
        console.error('ERROR: Specify an output file with --out')
        return
    }

    try {
        let scriptToCompile = fs.readFileSync(options.in, 'utf8')
        let compiledScript  = compiler.compileScript(scriptToCompile)
        fs.writeFileSync(options.out, compiledScript)
    } catch(error) {
        console.error(error)
    }
}


function run(file) {
    try {
        let compiledScript = fs.readFileSync(file)
        compiler.runScript(compiledScript)
    } catch(error) {
        console.error(error)
    }
}
