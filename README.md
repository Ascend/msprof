<h1 align="center">MindStudio Profiler</h1>

<div align="center">
  <p><b>昇腾性能采集工具</b></p>

 [![Ask DeepWiki](https://badgen.net/badge/Ask/DeepWiki/blue)](https://deepwiki.com/kali20gakki/msprof )
 [![Ask ZRead](https://badgen.net/badge/Ask/ZRead/orange)](https://zread.ai/kali20gakki/msprof)
 [![doc](https://badgen.net/badge/doc/readthedocs/green)](https://mindstudio-profiler-docs.readthedocs.io/zh-cn/latest/msprof/)
 [![License](https://badgen.net/badge/License/MulanPSL-2.0/blue)](https://raw.gitcode.com/Ascend/msinsight/files/master/License)
 [![Version](https://badgen.net/badge/Version/26.0.0-alpha.1/green)](https://gitcode.com/Ascend/msprof/releases)
 [![Ascend](https://img.shields.io/badge/Hardware-Ascend-orange.svg)](https://www.hiascend.com/)

</div>

## ✨ 最新消息

* [2025.12.30]：MindStudio Profiler项目首次上线

## ℹ️ 简介

MindStudio Profiler（msProf）是面向 AI 训练与推理场景的性能分析工具，支持采集与解析 CANN 层和昇腾 AI 处理器 NPU 硬件层的软硬件性能数据，帮助定位模型训练或推理过程中的性能问题。`msProf` 也是其他 Profiling 采集接口的基础能力，许多上层性能采集与分析能力最终都依赖 `msProf` 完成底层数据采集。若希望了解昇腾性能调优工具的完整全景，可进一步参考[MindStudio Profiler 文档总览](https://mindstudio-profiler-docs.readthedocs.io/zh-cn/latest/)。

![msprof](./docs/zh/figures/msprof.png)

## ⚙️ 功能介绍

| 功能名称      | 功能简介 |                                                                文档                                                                |  源码仓库 |
|------------| --- |:----------------------------------------------------------------------------------------------------------------------------------:|-----------|
| **性能数据采集** | 通过 `msProf` 命令采集 CANN 平台及昇腾 AI 处理器的软硬件性能数据。 | [性能数据采集](https://www.hiascend.com/document/detail/zh/CANNCommunityEdition/850alpha002/devaids/Profiling/atlasprofiling_16_0010.html) | [msprof](https://gitcode.com/cann/runtime/tree/master/src/dfx/msprof) |
| **性能数据解析** | 使用 `msProf` 工具对采集到的性能数据进行解析，生成可读的分析结果。 |                                       [性能数据解析](docs/zh/user_guide/msprof_parsing_instruct.md)                              | [analysis](https://gitcode.com/Ascend/msprof/tree/master/analysis) |

## 🚀 快速入门

msProf 工具通过命令行调用，通用采集命令格式如下：

```bash
msprof --output=<输出目录> --application="<应用程序> <参数>"
```

示例：

```bash
# 示例1：采集Python任务
msprof --output=./output --application="python3 train.py"

# 示例2：采集Shell脚本拉起的AI任务
msprof --output=./output --application="./run_standalone_train.sh"
```

以 ResNet50 模型训练任务为例，《[快速入门](docs/zh/quick_start/quick_start.md)》贯穿性能调优全流程，帮助您在 10 分钟内快速体验 msProf 工具在数据采集、解析导出、性能分析等环节的核心功能。

## 📦 安装指南

msProf 工具内置在 CANN Toolkit 开发套件中，推荐直接下载 CANN 包进行安装，具体请参见《[CANN 快速安装](https://www.hiascend.com/cann/download)》。

如需通过源码编译方式安装，请参见 《[msProf 工具安装指南](docs/zh/install_guide/msprof_install_guide.md)》。

## 📘 使用指南

工具的详细使用方法，请参见《[msProf 使用指南](docs/zh/user_guide/msprof_parsing_instruct.md)》。

## 💡 典型案例

通过典型问题场景帮助用户理解并掌握工具使用，请参见《[msProf 典型案例](docs/zh/best_practices/basic_cases.md)》。

## ❓ FAQ

常见问题及解决方案，请参见《[msProf FAQ](docs/zh/support/faq.md)》。

## 🌌 智能检索

为提升文档查阅效率，我们提供多种高效检索方式：

🔹 [AI 问答（DeepWiki）](https://deepwiki.com/mindstudio-docs/master)：自然语言问答，快速把握项目架构与模块关系。<br>
🔹 [AI 问答（ZRead）](https://zread.ai/mindstudio-docs/master)：中文问答体验更优，精准定位功能用法与细节。<br>
🔹 [精确搜索（ReadTheDocs）](https://mindstudio-docs-master.readthedocs.io)：关键词全文检索，直达接口、参数与报错等信息。<br>

## 🛠️ 贡献指南

欢迎参与项目贡献，请参见 [《贡献指南》](docs/zh/contributing/contributing_guide.md)。

## ⚖️ 相关说明

- 《[License声明](docs/zh/legal/LICENSE.md)》

- 《[安全声明](docs/zh/legal/SECURITY.md)》

- 《[免责声明](docs/zh/legal/disclaimer.md)》

## 🤝 建议与交流

欢迎大家为社区做贡献。如果有任何疑问或建议，请提交 [Issues](https://gitcode.com/Ascend/msprof/issues)，我们会尽快回复。感谢您的支持。

|                                                                            即时互动（微信群）                                                                             |                                                                                  官方资讯（公众号）                                                                                   | 深度支持（助手/论坛）                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 |
|:------------------------------------------------------------------------------------------------------------------------------------------------------------------:|:------------------------------------------------------------------------------------------------------------------------------------------------------------------------------:|:--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| <img src="https://raw.gitcode.com/mengguangxin/docs/files/dev_0526/common/Writing_Template/figures/qr_code_wechat_work.png" width="120"><br><sub>*扫码加入技术交流群*</sub> | <img src="https://raw.gitcode.com/mengguangxin/docs/files/dev_0526/common/Writing_Template/figures/qr_code_wechat_official_account.png" width="120"><br><sub>*扫码关注官方公众号*</sub> | 扫码入群并关注公众号，直达 MindStudio 用户与开发者最快捷的交流平台：<br> **快速提问：** 与社区小伙伴即时探讨技术问题<br>**掌握动态：** 第一时间获取版本发布与功能更新通知<br> **经验共享：** 与广大开发者交流最佳实践与实战心得  <br> <br> **更多支持渠道**：👉 昇腾助手：[![WeChat](https://img.shields.io/badge/WeChat-07C160?style=flat-square&logo=wechat&logoColor=white)](https://gitcode.com/Ascend/msit/blob/master/docs/zh/figures/readme/xiaozhushou.png) 👉 昇腾论坛：[![Website](https://img.shields.io/badge/Website-%231e37ff?style=flat-square&logo=RSS&logoColor=white)](https://www.hiascend.com/forum/) |

## 🙏 致谢

本工具由华为公司的下列部门贡献：

- 昇腾计算MindStudio开发部

感谢来自社区的每一个PR，欢迎贡献。
