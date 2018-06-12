{
  "targets": [
    {
      "target_name": "WebWorkerThreads",
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
      "win_delay_load_hook": "<!(node -e \"var v = process.version.substring(1,2); console.log(v > 0 && v < 4);\")",
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
    }
  ]
}
