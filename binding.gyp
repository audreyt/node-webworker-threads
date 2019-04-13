{
  "targets": [
    {
      "target_name": "<(module_name)",
      "sources": [
        "src/WebWorkerThreads.cc"
      ],
      "cflags!": [
        "-fno-exceptions",
        "-DV8_USE_UNSAFE_HANDLES"
      ],
      "cflags_cc!": [
        "-fno-exceptions",
        "-DV8_USE_UNSAFE_HANDLES"
      ],
      "include_dirs": [
        "<!(node -e \"require('nan')\")"
      ],
      "win_delay_load_hook": "<!(node -e \"var v = process.version.substring(1).split('.')[0]; console.log(v > 0 && v < 4);\")",
      "conditions": [
        [
          "OS==\"mac\"",
          {
            "xcode_settings": {
              "GCC_ENABLE_CPP_EXCEPTIONS": "YES"
            }
          }
        ]
      ]
    },
    {
      'target_name': 'action_after_build',
      'type': 'none',
      'dependencies': ['<(module_name)'],
      'copies': [
        {
          'files': ['<(PRODUCT_DIR)/<(module_name).node'],
          'destination': '<(module_path)'
        }
      ]
    }
  ]
}
