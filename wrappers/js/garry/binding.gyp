{
  "targets": [
    {
      "target_name": "garry",
      "sources": ["src/garry.cc"],
      "include_dirs": [
        "<!@(node -p \"require('node-addon-api').include\")",
        "../../../include"
      ],
      "libraries": ["-lgarry"],
      "library_dirs": ["../../../build"],
      "cflags!": ["-fno-exceptions"],
      "cflags_cc!": ["-fno-exceptions"],
      "xcode_settings": {
        "GCC_ENABLE_CPP_EXCEPTIONS": "YES",
        "CLANG_CXX_LIBRARY": "libc++",
        "MACOSX_DEPLOYMENT_TARGET": "10.15",
        "OTHER_LDFLAGS": ["-Wl,-rpath,@loader_path/../../../../build"]
      },
      "msvs_settings": {
        "VCCLCompilerTool": { "ExceptionHandling": 1 }
      }
    }
  ]
}
