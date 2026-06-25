<h1 align="center">MindStudio Profiler</h1>

<div align="center">
  <p><b>Ascend Performance Data Collection Tool</b></p>

 [![Ask DeepWiki](https://badgen.net/badge/Ask/DeepWiki/blue)](https://deepwiki.com/kali20gakki/msprof )
 [![Ask ZRead](https://badgen.net/badge/Ask/ZRead/orange)](https://zread.ai/kali20gakki/msprof)
 [![doc](https://badgen.net/badge/doc/readthedocs/green)](https://mindstudio-profiler-docs.readthedocs.io/zh-cn/latest/msprof/)
 [![License](https://badgen.net/badge/License/MulanPSL-2.0/blue)](https://raw.gitcode.com/Ascend/msinsight/files/master/License)
 [![Version](https://badgen.net/badge/Version/26.0.0-alpha.1/green)](https://gitcode.com/Ascend/msprof/releases)
 [![Ascend](https://img.shields.io/badge/Hardware-Ascend-orange.svg)](https://www.hiascend.com/)

</div>

## ✨ What's New

* [2025.12.30]: Initial release of the MindStudio Profiler (msProf) project.

## ℹ️ Overview

MindStudio Profiler (msProf) is a performance analysis tool for AI training and inference scenarios. It supports the collection and parsing of software and hardware performance data from the CANN layer and Ascend AI Processor NPU hardware layer, helping users identify performance bottlenecks during model training or inference. `msProf` also serves as the foundational capability for other profiling collection interfaces — many upper-layer performance collection and analysis capabilities ultimately rely on `msProf` for underlying data collection. For a complete picture of Ascend performance tuning tools, see [MindStudio Profiler Documentation Overview](https://mindstudio-profiler-docs.readthedocs.io/zh-cn/latest/).

![msprof](./docs/en/figures/msprof.png)

## ⚙️ Functions

| Function      | Description |                                                                Document                                                                |  Source Code Repository |
|------------| --- |:----------------------------------------------------------------------------------------------------------------------------------:|-----------|
| **Performance Data Collection** | Collects software and hardware performance data from the CANN platform and Ascend AI Processors using `msprof` commands. | [Performance Data Collection](https://www.hiascend.com/document/detail/en/canncommercial/850/devaids/profiling/atlasprofiling_16_0010.html) | [msprof](https://gitcode.com/cann/runtime/tree/master/src/dfx/msprof) |
| **Performance Data Parsing** | Parses the collected performance data using the `msProf` tool to generate readable analysis results. |                                       [Performance Data Parsing](docs/en/user_guide/msprof_parsing_instruct.md)                              | [analysis](https://gitcode.com/Ascend/msprof/tree/master/analysis) |

## 🚀 Quick Start

The msProf tool is invoked via the command line. The general collection command format is as follows:

```bash
msprof --output=<output_dir> --application="<application> <arguments>"
```

Examples:

```bash
# Example 1: Collect data for a Python task
msprof --output=./output --application="python3 train.py"

# Example 2: Collect data for an AI task launched by a shell script
msprof --output=./output --application="./run_standalone_train.sh"
```

Taking the ResNet-50 model training task as an example, [Quick Start](docs/en/getting_started/quick_start.md) guides you through the full performance tuning workflow, helping you quickly experience the core functions of the msProf tool — including data collection, parsing and export, and performance analysis — within 10 minutes.

## 📦 Installation Guide

The msProf tool is included in the CANN Toolkit development suite. It is recommended to download the CANN package directly for installation. For details, see [CANN Quick Installation](https://www.hiascend.com/cann/download).

To build and install from source code, see [msProf Installation Guide](docs/en/getting_started/msprof_install_guide.md).

## 📘 User Guide

For detailed usage instructions, see [msProf User Guide](docs/en/user_guide/msprof_parsing_instruct.md).

## 💡 Typical Cases

Explore typical problem scenarios to understand and master the tool. See [msProf Typical Cases](docs/zh/best_practices/basic_cases.md).

## ❓ FAQ

For common questions and solutions, see [msProf FAQ](docs/zh/support/faq.md).

## 🌌 Smart Search

To improve documentation lookup efficiency, we offer several efficient search methods:

🔹 [AI Q&A (DeepWiki)](https://deepwiki.com/mindstudio-docs/master): Natural language Q&A for quickly understanding project architecture and module relationships.<br>
🔹 [AI Q&A (ZRead)](https://zread.ai/mindstudio-docs/master): Optimized Chinese Q&A experience for precisely locating function usage and details.<br>
🔹 [Exact Search (ReadTheDocs)](https://mindstudio-docs-master.readthedocs.io): Full-text keyword search for direct access to APIs, parameters, and error messages.<br>

## 🛠️ Contributing Guide

We welcome contributions to the project. See [Contributing Guide](docs/zh/contributing/contributing_guide.md).

## ⚖️ Legal Notices

- [License Notice](docs/en/legal/license_notice.md)

- [Security Statement](docs/en/legal/security_statement.md)

- [Disclaimer](docs/en/legal/disclaimer.md)

## 🤝 Suggestions and Feedback

We welcome everyone to contribute to the community. If you have any questions or suggestions, please submit [Issues](https://gitcode.com/Ascend/msprof/issues). We will reply as soon as possible. Thank you for your support.

|                                                                            Instant Messaging (WeChat Group)                                                                            |                                                                                  Official News (Official Account)                                                                                   | In-Depth Support (Assistant / Forum)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 |
|:------------------------------------------------------------------------------------------------------------------------------------------------------------------:|:------------------------------------------------------------------------------------------------------------------------------------------------------------------------------:|:--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| <img src="https://raw.gitcode.com/mengguangxin/docs/files/dev_0526/common/Writing_Template/figures/qr_code_wechat_work.png" width="120"><br><sub>*Scan to join the technical discussion group*</sub> | <img src="https://raw.gitcode.com/mengguangxin/docs/files/dev_0526/common/Writing_Template/figures/qr_code_wechat_official_account.png" width="120"><br><sub>*Scan to follow the official account*</sub> | Scan the QR code to join the group and follow the official account for the fastest way to connect with the MindStudio user and developer community:<br> **Quick Questions:** Discuss technical issues with community members in real time<br>**Stay Updated:** Get notified about version releases and feature updates firsthand<br> **Share Experience:** Exchange best practices and practical insights with fellow developers  <br> <br> **More Support Channels**: 👉 Ascend Assistant: [![WeChat](https://img.shields.io/badge/WeChat-07C160?style=flat-square&logo=wechat&logoColor=white)](https://gitcode.com/Ascend/msit/blob/master/docs/zh/figures/readme/xiaozhushou.png) 👉 Ascend Forum: [![Website](https://img.shields.io/badge/Website-%231e37ff?style=flat-square&logo=RSS&logoColor=white)](https://www.hiascend.com/forum/) |

## 🙏 Acknowledgments

This tool is developed by the following Huawei department:

- Ascend Computing MindStudio Development Department

Thank you to everyone in the community for your PRs. We warmly welcome your contributions.
