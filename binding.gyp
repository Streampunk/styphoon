{
  "targets": [{
    "target_name" : "styphoon",
    "include_dirs" : [
      "<!(node -e \"require('nan')\")"
    ],
    "conditions": [
      ['OS=="mac"', {
        'sources' : [ 
            "src/Capture.cpp",
            "src/styphoon.cpp",
            "src/TyphoonCapture.cpp",
            "src/BufferStatus.cpp"
        ],
        'xcode_settings': {
          'GCC_ENABLE_CPP_RTTI': 'YES',
          'MACOSX_DEPLOYMENT_TARGET': '10.7',
          'OTHER_CPLUSPLUSFLAGS': [
            '-std=c++11',
            '-stdlib=libc++'
          ]
        },
        "link_settings": {
          "libraries": [
          ]
        },
        "include_dirs" : [
        ]
      }],
      ['OS=="win"', {
        "sources" : [ 
            "src/Capture.cpp",
            "src/styphoon.cpp",
            "src/TyphoonTypeMaps.cpp",
            "src/TyphoonCapture.cpp",
            "src/TyphoonDevice.cpp",
            "src/TyphoonRegister.cpp",
            "src/BufferStatus.cpp"
		],
        "configurations": {
          "Release": {
		    "msvs_version" : "2013",
            "msvs_settings": {
              "VCCLCompilerTool": {
                "RuntimeTypeInfo": "true",
                "ExceptionHandling": 1,
                'RuntimeLibrary': 4
              }
            }
          }
        },
        "libraries": [
			"-l../CoreEl/TyphoonSDK_64.lib"
        ],
        "include_dirs" : [
          "src",
          "CoreEl"
        ],
		"defines": [ "MSWindows", "WIN32", "_WINDOWS" ]
      }]
    ]
  }]
}
