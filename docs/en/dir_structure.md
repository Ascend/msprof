# Project Directory

The full project directory structure is as follows:

```sh
    └── .gitcode                                  // Repository metadata directory
    └── analysis                                  // Data parsing directory
        └── analyzer                              // Communication data parsing directory
        └── common_func                           // Common methods directory
        └── csrc                                  // Directory storing C-based components of msProf
        └── framework                             // Main parsing process directory
        └── host_prof                             // Host-side system call data parsing directory
        └── interface                             // Parsing interfaces directory
        └── mscalculate                           // Analysis data calculation directory
        └── msconfig                              // Directory storing configuration classes for modules such as Stars and AI Core
        └── msinterface                           // Directory storing classes for parsing commands
        └── msmodel                               // Directory storing database processing classes
        └── msparser                              // Binary data parsing process management directory
        └── msprof                                // Entry point for msprof
        └── profiling_bean                        // Directory storing d binary data parsing classes
        └── tuning                                // Cluster data management directory
        └── viewer                                // Export deliverables directory
    └── build                                     // Build directory
        ├── build.sh                              // Build script
        ├── setup.py                              // Script for building and parsing Python code
    └── cmake                                     // CMake files for C-based components of msProf
    └── docs                                      // Documentation
        └── en                                    // English documentation
    └── samples                                   // Tool samples directory
        ├── README.md                             // Tool sample description
    └── scripts                                   // .run package installation- and upgrade-related scripts
        └── run_script                            // .run package installation-related scripts
            ├── install.sh                        // Installation scripts
        ├── create_run_package.sh                 // .run package creation script
        ├── download_thirdparty.sh                // Third-party dependency download script
    └── test                                      // Testing directory storing coverage statistics scripts
        └── msprof_cpp                            // C++ code test cases for data parsing
        └── msprof_python                         // Python code test cases for data parsing
    └── misc                                      // Miscellaneous tools
        ├── function_monitor                      // Lightweight function monitoring tool
        └── gil_tracer                            // Python GIL lock detection tool
    └── README.md                                 // Repository overview document

```
