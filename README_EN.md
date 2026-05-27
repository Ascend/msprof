
<h1 align="center">MindStudio Profiler</h1>
<div align="center">
  <p>🚀 <b>Ascend Profile Data Collection Tool</b></p>

[📖 Documentation](https://www.hiascend.com/document/detail/zh/CANNCommunityEdition/850alpha002/devaids/Profiling/atlasprofiling_16_0010.html) |
[🔥 Ascend Community](https://www.hiascend.com/developer/software/mindstudio)|
[🌐Release](https://gitcode.com/Ascend/msprof/releases)

</div>

## 📢 What's New

* [2025.12.30]: Initial release of the MindStudio Profiler (msProf) project.

## 📌 Overview

msProf is a profile data analysis tool for AI training and inference scenarios. It supports the collection and analysis of software and hardware profile data from the CANN platform and Ascend AI Processors, helping users locate performance bottlenecks during model training or inference.

![msprof](./docs/en/figures/msprof.png)

## ⚙️ Functions

| Function     | Description|                                                                Document                                                               |  Source Code Repository|
|------------| --- |:----------------------------------------------------------------------------------------------------------------------------------:|-----------|
| **Profile data collection**| Collects software and hardware profile data from the CANN platform and Ascend AI Processors by using `msprof` commands.| [Profile Data Collection](https://www.hiascend.com/document/detail/en/canncommercial/850/devaids/profiling/atlasprofiling_16_0010.html) | [msprof](https://gitcode.com/cann/runtime/tree/master/src/dfx/msprof) |
| **Profile data parsing**| Parses the collected profile data to generate human-readable analysis results by using the msProf tool.|                                       [Profile Data Parsing](docs/en/user_guide/msprof_parsing_instruct.md)                             | [analysis](https://gitcode.com/Ascend/msprof/tree/master/analysis) |

### 🛠️ Tool Installation

msProf is included in the CANN Toolkit. You are advised to download and install the CANN package directly. For details, see [CANN Software Installation Guide](https://www.hiascend.com/document/detail/zh/canncommercial/850/softwareinst/instg/instg_0000.html?Mode=PmIns&InstallType=netconda&OS=openEuler).

After installing the CANN package, run the following command to set the environment variables: 

```bash
# ${install_path} indicates the installation directory of the CANN software, such as /usr/local/Ascend/ascend-toolkit.
source ${install_path}/set_env.sh
```

Run the following command to check whether the installation is successful: 

```bash
msprof --help
```

To build and install the tool from the source code, see [MindStudio Profiler Installation Guide](docs/en/getting_started/msprof_install_guide.md).

## 🚀 Quick Start

You can use the msProf tool through the command line, and the general syntax for data collection is as follows:

```bash
msprof --output=< output directory> --application="<application> <argument>"
```

Examples:

```bash
# Example 1: Collect profile data for Python tasks.
msprof --output=./output --application="python3 train.py"

# Example 2: Collect profile data for AI tasks launched by a shell script.
msprof --output=./output --application="./run_standalone_train.sh"
```

Taking profile data from a ResNet-50 model training task as an example, [Quick Start](docs/en/getting_started/quick_start.md) guides you through the entire profiling process. It helps you quickly experience the core functions of msProf, including data collection, parsing, export, and performance profiling, within a 10-minute workflow.

## 🗂️ Directory Structure

The key directories are as follows. For details, see [Project Directory](docs/en/dir_structure.md).

```text
.
├── .gitcode                  # Repository metadata directory
├── analysis                  # Data parsing directory
├── build                     # Build directory
│   └── build.sh              # Build script
├── cmake                     # CMake file directory
├── docs                      # Documentation directory
│   └── en                    # English documentation
├── misc                      # Miscellaneous tools
│   ├── function_monitor      # Lightweight function monitoring tool
│   └── gil_tracer            # Python GIL lock detection tool
├── samples                   # Tool sample directory
│   └── README.md             # Sample description
├── scripts                   # Installation- and upgrade-related scripts
├── test                      # Test and coverage statistics scripts
└── README.md                 # Project description
```

## 📝 References

- [Contributing Guide](CONTRIBUTING.md)

- [License Notice](docs/en/legal/license_notice.md)

- [Security Statement](docs/en/legal/security_statement.md)

- [Disclaimer](/docs/en/legal/disclaimer.md) 

## 💬 Suggestions and Feedback

You are welcome to contribute to the community. If you have any questions or suggestions, please submit [issues](https://gitcode.com/Ascend/msprof/issues). We will reply as soon as possible. Thank you for your support.

## 🤝 Acknowledgments

This tool is developed by the following Huawei department:

- Ascend Computing MindStudio Development Department 

Thank you to everyone in the community for your PRs. We warmly welcome your contributions.

## 👥 About the MindStudio Team

The Huawei MindStudio full-pipeline development toolchain team is dedicated to providing an end-to-end solution for building Ascend AI applications, accelerating the processes of training, inference, and operator development. You can learn more about the Huawei MindStudio team through the following channels:
<div style="display: flex; align-items: center; gap: 10px;">
    <span>Ascend forum:</span>
    <a href="https://www.hiascend.com/forum/" rel="nofollow">
        <img src="https://camo.githubusercontent.com/dd0b7ef70793ab93ce46688c049386e0755a18faab780e519df5d7f61153655e/68747470733a2f2f696d672e736869656c64732e696f2f62616467652f576562736974652d2532333165333766663f7374796c653d666f722d7468652d6261646765266c6f676f3d6279746564616e6365266c6f676f436f6c6f723d7768697465" data-canonical-src="https://img.shields.io/badge/Website-%231e37ff?style=for-the-badge&amp;logo=bytedance&amp;logoColor=white" style="max-width: 100%;">
    </a>
    <span style="margin-left: 20px;">Ascend open-source assistant:</span>
    <a href="https://gitcode.com/Ascend/msinsight/blob/master/docs/zh/user_guide/figures/readme/xiaozhushou.png">
        <img src="https://camo.githubusercontent.com/22bbaa8aaa1bd0d664b5374d133c565213636ae50831af284ef901724e420f8f/68747470733a2f2f696d672e736869656c64732e696f2f62616467652f5765436861742d3037433136303f7374796c653d666f722d7468652d6261646765266c6f676f3d776563686174266c6f676f436f6c6f723d7768697465" data-canonical-src="./docs/en/user_guide/figures/readme/xiaozhushou.png" style="max-width: 100%;">
    </a>
</div>
