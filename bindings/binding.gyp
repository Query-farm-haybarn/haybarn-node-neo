{
  # Detect musl vs glibc once at file load and expose suffix variables to
  # every target. The nested-variables pattern resolves <!()-shells before
  # the conditions that consume them. The OS=="linux" gate keeps gyp from
  # trying to run `ldd` on macOS or Windows.
  'variables': {
    'variables': {
      'conditions': [
        ['OS=="linux"', {
            'libc_musl%': '<!(ldd --version 2>&1 | head -n1 | grep "musl" | wc -l)',
          }, {
            'libc_musl%': 0,
          }
        ],
      ],
    },
    'conditions': [
      ['<(libc_musl) == 1', {
          'libc_pkg_suffix%': '-musl',
          'libc_script_suffix%': '_musl',
        }, {
          'libc_pkg_suffix%': '',
          'libc_script_suffix%': '',
        }
      ],
    ],
  },
  'targets': [
    {
      'target_name': 'fetch_libhaybarn',
      'type': 'none',
      'conditions': [
        ['OS=="linux" and target_arch=="x64"', {
          'variables': {
            'script_path': '<(module_root_dir)/scripts/fetch_libhaybarn_linux_amd64<(libc_script_suffix).py',
          },
        }],
        ['OS=="linux" and target_arch=="arm64"', {
          'variables': {
            'script_path': '<(module_root_dir)/scripts/fetch_libhaybarn_linux_arm64<(libc_script_suffix).py',
          },
        }],
        ['OS=="mac"', {
          'variables': {
            'script_path': '<(module_root_dir)/scripts/fetch_libhaybarn_osx_universal.py',
          },
        }],
        ['OS=="win" and target_arch=="arm64"', {
          'variables': {
            'script_path': '<(module_root_dir)/scripts/fetch_libhaybarn_windows_arm64.py',
          },
        }],
        ['OS=="win" and target_arch=="x64"', {
          'variables': {
            'script_path': '<(module_root_dir)/scripts/fetch_libhaybarn_windows_amd64.py',
          },
        }],
      ],
      'actions': [
        {
          'action_name': 'run_fetch_libhaybarn_script',
          'message': 'Fetching and extracting libhaybarn',
          'inputs': [],
          'action': ['python3', '<(script_path)'],
          'outputs': ['<(module_root_dir)/libhaybarn'],
        },
      ],
    },
    {
      'target_name': 'haybarn',
      'dependencies': [
        'fetch_libhaybarn',
        '<!(node -p "require(\'node-addon-api\').targets"):node_addon_api_except_all',
      ],
      'sources': ['src/duckdb_node_bindings.cpp'],
      'include_dirs': ['<(module_root_dir)/libhaybarn'],
      'conditions': [
        ['OS=="linux" and target_arch=="x64"', {
          'link_settings': {
            'libraries': [
              '-lhaybarn',
              '-L<(module_root_dir)/libhaybarn',
              '-Wl,-rpath,\'$$ORIGIN\'',
            ],
          },
          'copies': [
            {
              'files': ['<(module_root_dir)/libhaybarn/libhaybarn.so'],
              'destination': '<(module_root_dir)/pkgs/@haybarn/node-bindings-linux-x64<(libc_pkg_suffix)',
            },
          ],
        }],
        ['OS=="linux" and target_arch=="arm64"', {
          'link_settings': {
            'libraries': [
              '-lhaybarn',
              '-L<(module_root_dir)/libhaybarn',
              '-Wl,-rpath,\'$$ORIGIN\'',
            ],
          },
          'copies': [
            {
              'files': ['<(module_root_dir)/libhaybarn/libhaybarn.so'],
              'destination': '<(module_root_dir)/pkgs/@haybarn/node-bindings-linux-arm64<(libc_pkg_suffix)',
            },
          ],
        }],
        ['OS=="mac" and target_arch=="arm64"', {
          'cflags+': ['-fvisibility=hidden'],
          'xcode_settings': {
            'GCC_SYMBOLS_PRIVATE_EXTERN': 'YES', # -fvisibility=hidden
          },
          'link_settings': {
            'libraries': [
              '-lhaybarn',
              '-L<(module_root_dir)/libhaybarn',
              '-Wl,-rpath,@loader_path',
            ],
          },
          'copies': [
            {
              'files': ['<(module_root_dir)/libhaybarn/libhaybarn.dylib'],
              'destination': '<(module_root_dir)/pkgs/@haybarn/node-bindings-darwin-arm64',
            },
          ],
        }],
        ['OS=="mac" and target_arch=="x64"', {
          'cflags+': ['-fvisibility=hidden'],
          'xcode_settings': {
            'GCC_SYMBOLS_PRIVATE_EXTERN': 'YES', # -fvisibility=hidden
          },
          'link_settings': {
            'libraries': [
              '-lhaybarn',
              '-L<(module_root_dir)/libhaybarn',
              '-Wl,-rpath,@loader_path',
            ],
          },
          'copies': [
            {
              'files': ['<(module_root_dir)/libhaybarn/libhaybarn.dylib'],
              'destination': '<(module_root_dir)/pkgs/@haybarn/node-bindings-darwin-x64',
            },
          ],
        }],
        ['OS=="win" and target_arch=="arm64"', {
          'link_settings': {
            'libraries': [
              '<(module_root_dir)/libhaybarn/haybarn.lib',
            ],
          },
          'copies': [
            {
              'files': ['<(module_root_dir)/libhaybarn/haybarn.dll'],
              'destination': '<(module_root_dir)/pkgs/@haybarn/node-bindings-win32-arm64',
            },
          ],
        }],
        ['OS=="win" and target_arch=="x64"', {
          'link_settings': {
            'libraries': [
              '<(module_root_dir)/libhaybarn/haybarn.lib',
            ],
          },
          'copies': [
            {
              'files': ['<(module_root_dir)/libhaybarn/haybarn.dll'],
              'destination': '<(module_root_dir)/pkgs/@haybarn/node-bindings-win32-x64',
            },
          ],
        }],
      ],
    },
    {
      'target_name': 'copy_haybarn_node',
      'type': 'none',
      'dependencies': ['haybarn'],
      'conditions': [
        ['OS=="linux" and target_arch=="x64"', {
          'copies': [
            {
              'files': ['<(module_root_dir)/build/Release/haybarn.node'],
              'destination': '<(module_root_dir)/pkgs/@haybarn/node-bindings-linux-x64<(libc_pkg_suffix)',
            },
          ],
        }],
        ['OS=="linux" and target_arch=="arm64"', {
          'copies': [
            {
              'files': ['<(module_root_dir)/build/Release/haybarn.node'],
              'destination': '<(module_root_dir)/pkgs/@haybarn/node-bindings-linux-arm64<(libc_pkg_suffix)',
            },
          ],
        }],
        ['OS=="mac" and target_arch=="arm64"', {
          'copies': [
            {
              'files': ['<(module_root_dir)/build/Release/haybarn.node'],
              'destination': '<(module_root_dir)/pkgs/@haybarn/node-bindings-darwin-arm64',
            },
          ],
        }],
        ['OS=="mac" and target_arch=="x64"', {
          'copies': [
            {
              'files': ['<(module_root_dir)/build/Release/haybarn.node'],
              'destination': '<(module_root_dir)/pkgs/@haybarn/node-bindings-darwin-x64',
            },
          ],
        }],
        ['OS=="win" and target_arch=="arm64"', {
          'copies': [
            {
              'files': ['<(module_root_dir)/build/Release/haybarn.node'],
              'destination': '<(module_root_dir)/pkgs/@haybarn/node-bindings-win32-arm64',
            },
          ],
        }],
        ['OS=="win" and target_arch=="x64"', {
          'copies': [
            {
              'files': ['<(module_root_dir)/build/Release/haybarn.node'],
              'destination': '<(module_root_dir)/pkgs/@haybarn/node-bindings-win32-x64',
            },
          ],
        }],
      ],
    },
  ],
}
