{
    'targets': [
        {
            'target_name': 'v8-compiler',
            'sources': [ 'addon/v8-compiler.cpp' ],
            'include_dirs': [
                'node_source/deps/v8',
                "<!(node -e \"require('nan')\")"
            ]
        }
    ]
}
