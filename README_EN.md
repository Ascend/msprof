<h1 align="center">MindStudio Profiler</h1>

<div align="center">
<p><b><span style="font-size:24px;">Ascend Profile Data Collection Tool</span></b></p>

 [![Quick Start](https://badgen.net/badge/Quick%20Start/QuickStart/blue)](docs/en/quick_start/msprof_quick_start.md)
 [![AI Q&A DeepWiki](https://badgen.net/badge/AI%20Q%26A/DeepWiki/blue)](https://deepwiki.com/Ascend/msprof)
 [![AI Q&A ZRead](https://badgen.net/badge/AI%20Q%26A/ZRead/blue)](https://zread.ai/Ascend/msprof)
 [![Exact Search](https://badgen.net/badge/Exact%20Search/ReadTheDocs/blue)](https://mindstudio-docs-master.readthedocs.io/zh-cn/latest/msprof)
 [![Ascend Community](https://badgen.net/badge/Ascend%20Community/Community/blue)](https://www.hiascend.com/en/developer/software/mindstudio)
 [![Report an Issue](https://badgen.net/badge/Report%20an%20Issue/Issues/blue)](https://gitcode.com/Ascend/msprof/issues)

</div>

English | [简体中文](./README.md)

## ✨ What's New

🔹 [2025.12.30]: Initial release of the MindStudio Profiler project.

## ℹ️ Overview

MindStudio Profiler (msProf) is a performance analysis tool for AI training and inference scenarios. It supports the collection and parsing of software and hardware profile data from the CANN layer and the Ascend AI Processor NPU hardware layer, helping users identify performance issues during model training or inference. `msProf` also provides the foundational capability for other profiling collection interfaces, and many upper-layer performance collection and analysis capabilities rely on `msProf` for underlying data collection.

<div align="center">
  <img src="./docs/en/figures/msprof.png" alt="Architecture diagram" width="700">
</div>

## ⚙️ Features

| Feature | Description | Documentation | Source Code Repository |
| --- | --- | :---: | --- |
| **Profile data collection** | Collects software and hardware profile data from the CANN platform and Ascend AI Processors using the `msProf` command. | [Profile Data Collection](https://www.hiascend.com/document/detail/en/CANNCommunityEdition/latest/devaids/Profiling/atlasprofiling_16_0010.html) | [msprof](https://gitcode.com/cann/runtime/tree/master/src/dfx/msprof) |
| **Profile data parsing** | Parses the collected profile data using the `msProf` tool to generate readable analysis results. | [Profile Data Parsing](docs/en/user_guide/msprof_parsing_instruct.md) | [analysis](analysis) |

## 🚀 Quick Start

**10-minute hands-on experience**  
Using ResNet50 training as an example, this tutorial covers the complete workflow of **data collection, parsing and export, and performance analysis**. Click to get started: [msProf Quick Start](docs/en/quick_start/msprof_quick_start.md).

**Quick command-line reference**  
If you are already familiar with the workflow, you can directly run the collection command. The general syntax is `msprof --output=<output_dir> --application="<launch_command>"`. Examples:

```bash
# Example 1: Collect data for a Python training task
msprof --output=./output --application="python3 train.py"
# Example 2: Collect data for a task launched by a shell script
msprof --output=./output --application="./run_standalone_train.sh"
```

## 📦 Installation Guide

msProf is included in the CANN Toolkit development suite. You are advised to download and install the CANN package directly. For details, see [CANN Quick Installation](https://www.hiascend.com/cann/download).

To install msProf through building from source code, see [msProf Installation Guide](docs/en/install_guide/msprof_install_guide.md).

## 📘 User Guide

For detailed usage instructions, see [msProf User Guide](docs/en/user_guide/msprof_parsing_instruct.md).

## 💡 Typical Cases

To understand and master the tool through typical problem scenarios, see [msProf Typical Cases](docs/en/best_practices/basic_cases.md).

## ❓ FAQ

For common questions and solutions, see [msProf FAQ](docs/en/support/faq.md).

## 🌌 Smart Search

To improve the efficiency of documentation lookup, we provide several efficient search methods:

🔹 [AI Q&A (DeepWiki)](https://deepwiki.com/mindstudio-docs/master): Natural language Q&A for quickly understanding project architecture and module relationships.<br>
🔹 [AI Q&A (ZRead)](https://zread.ai/mindstudio-docs/master): A better Chinese Q&A experience for precisely locating feature usage and details.<br>
🔹 [Exact Search (ReadTheDocs)](https://mindstudio-docs-master.readthedocs.io): Full-text keyword search for directly accessing information about APIs, parameters, error messages, and more.<br>

## 🛠️ Contributing Guide

We welcome contributions to the project. See [Contributing Guide](docs/en/contributing/contributing_guide.md).

## ⚖️ Related Information

🔹 [Release Notes](https://gitcode.com/Ascend/msprof/releases)<br>
🔹 [License Statement](docs/en/legal/LICENSE.md)<br>
🔹 [Security Statement](docs/en/legal/SECURITY.md)<br>
🔹 [Disclaimer](docs/en/legal/disclaimer.md)<br>

## 🤝 Suggestions and Feedback

We welcome everyone to contribute to the community. If you have any questions or suggestions, please submit an [issue](https://gitcode.com/Ascend/msprof/issues), and we will respond as soon as possible. Thank you for your support.

| Instant Messaging (WeChat Group) | Official News (Official Account) | In-Depth Support (Assistant / Forum) |
| :---: | :----: | :---- |
| <img src="https://raw.gitcode.com/Ascend/docs/files/master/common/Writing_Template/figures/qr_code_wechat_work.png" width="120"><br><sub>*Scan to join the technical discussion group*</sub> | <img src="https://raw.gitcode.com/Ascend/docs/files/master/common/Writing_Template/figures/qr_code_wechat_official_account.png" width="120"><br><sub>*Scan to follow the official account*</sub> | Scan the QR code to join the group and follow the official account for the fastest way to connect with MindStudio users and developers:<br> **Ask questions:** Discuss technical issues with community members in real time<br>**Stay updated:** Get the latest version releases and feature update notifications firsthand<br> **Share experience:** Exchange best practices and practical insights with fellow developers  <br> <br> **More support channels**:<br>👉 Ascend Assistant: [![WeChat](https://img.shields.io/badge/WeChat-07C160?style=flat-square\&logo=wechat\&logoColor=white)](https://gitcode.com/Ascend/msit/blob/master/docs/zh/figures/readme/xiaozhushou.png)<br> 👉 Ascend Forum: [![Website](https://img.shields.io/badge/Website-%231e37ff?style=flat-square\&logo=RSS\&logoColor=white)](https://www.hiascend.com/forum/) |

## 🙏 Acknowledgments

This tool is contributed by the following Huawei department:

🔹 Ascend Computing MindStudio Development Department

Thank you to everyone in the community for your PRs. We warmly welcome your contributions.
