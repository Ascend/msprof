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

## 📢 最新消息

* [2025.12.30]：MindStudio Profiler项目首次上线 

## 📌 简介

MindStudio Profiler（msProf）是面向 AI 训练与推理场景的性能分析工具，支持采集与解析 CANN 平台及昇腾 AI 处理器的软硬件性能数据，帮助定位模型训练或推理过程中的性能问题。

![msprof](./docs/zh/figures/msprof.png)

## ⚙️ 功能介绍

| 功能名称      | 功能简介 |                                                                文档                                                                |  源码仓库 |
|------------| --- |:----------------------------------------------------------------------------------------------------------------------------------:|-----------|
| **性能数据采集** | 通过 `msProf` 命令采集 CANN 平台及昇腾 AI 处理器的软硬件性能数据。 | [性能数据采集](https://www.hiascend.com/document/detail/zh/CANNCommunityEdition/850alpha002/devaids/Profiling/atlasprofiling_16_0010.html) | [msprof](https://gitcode.com/cann/runtime/tree/master/src/dfx/msprof) |
| **性能数据解析** | 使用 `msProf` 工具对采集到的性能数据进行解析，生成可读的分析结果。 |                                       [性能数据解析](docs/zh/user_guide/msprof_parsing_instruct.md)                              | [analysis](https://gitcode.com/Ascend/msprof/tree/master/analysis) |

## 🛠️ 安装指南

msProf 工具内置在 CANN Toolkit 开发套件中，推荐直接下载 CANN 包进行安装，具体请参见《[CANN 快速安装](https://www.hiascend.com/cann/download)》。

如需通过源码编译方式安装，请参见 《[msProf 工具安装指南](docs/zh/getting_started/msprof_install_guide.md)》。

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

以 ResNet50 模型训练任务为例，《[快速入门](docs/zh/getting_started/quick_start.md)》贯穿性能调优全流程，帮助您在 10 分钟内快速体验 msProf 工具在数据采集、解析导出、性能分析等环节的核心功能。

## 🗂️ 目录结构

关键目录如下，详细信息参见[目录结构说明](docs/zh/dir_structure.md)。

```text
.
├── .gitcode                  # 仓库元数据
├── analysis                  # 数据解析目录
├── build                     # 构建目录
│   └── build.sh              # 构建脚本
├── cmake                     # CMake 文件目录
├── docs                      # 文档目录
│   └── zh                    # 中文文档
├── misc                      # 其他工具
│   ├── function_monitor      # 轻量化函数监控工具
│   └── gil_tracer            # Python GIL 锁检测工具
├── samples                   # 工具样例目录
│   └── README.md             # 样例说明
├── scripts                   # 安装、升级相关脚本
├── test                      # 测试与覆盖率统计脚本
└── README.md                 # 项目说明文档
```

## 📝 相关说明

- 《[贡献指南](CONTRIBUTING.md)》

- 《[License声明](docs/zh/legal/license_notice.md)》 

- 《[安全声明](docs/zh/legal/security_statement.md)》 

- 《[免责声明](docs/zh/legal/disclaimer.md)》  

## 💬 建议与交流

欢迎大家为社区做贡献。如果有任何疑问或建议，请提交 [Issues](https://gitcode.com/Ascend/msprof/issues)，我们会尽快回复。感谢您的支持。

|                   📱 关注 MindStudio 公众号                   | 💬 更多交流与支持                                             |
| :----------------------------------------------------------: | :----------------------------------------------------------- |
| <img src="https://raw.gitcode.com/Ascend/msot/files/master/docs/zh/figures/readme/officialAccount.png" width="120"><br><sub>*扫码关注获取最新动态*</sub> | 💡 **加入微信交流群**：<br>关注公众号，回复“交流群”即可获取入群二维码。<br><br>🛠️ **其他渠道**：<br>👉 昇腾助手：[![WeChat](https://img.shields.io/badge/WeChat-07C160?style=flat-square&logo=wechat&logoColor=white)](https://gitcode.com/Ascend/msot/blob/master/docs/zh/figures/readme/xiaozhushou.png)<br>👉 昇腾论坛：[![Website](https://img.shields.io/badge/Website-%231e37ff?style=flat-square&logo=RSS&logoColor=white)](https://www.hiascend.com/forum/) |

## 🤝 致谢

本工具由华为公司的下列部门贡献：   

- 昇腾计算MindStudio开发部  

感谢来自社区的每一个PR，欢迎贡献。

## 👥 关于MindStudio团队

华为MindStudio全流程开发工具链团队致力于提供端到端的昇腾AI应用开发解决方案，使能开发者高效完成训练开发、推理开发和算子开发。您可以通过以下渠道更深入了解华为MindStudio团队：
<div style="display: flex; align-items: center; gap: 10px;">
    <span>昇腾论坛：</span>
    <a href="https://www.hiascend.com/forum/" rel="nofollow">
        <img src="https://camo.githubusercontent.com/dd0b7ef70793ab93ce46688c049386e0755a18faab780e519df5d7f61153655e/68747470733a2f2f696d672e736869656c64732e696f2f62616467652f576562736974652d2532333165333766663f7374796c653d666f722d7468652d6261646765266c6f676f3d6279746564616e6365266c6f676f436f6c6f723d7768697465" data-canonical-src="https://img.shields.io/badge/Website-%231e37ff?style=for-the-badge&amp;logo=bytedance&amp;logoColor=white" style="max-width: 100%;">
    </a>
    <span style="margin-left: 20px;">昇腾小助手：</span>
    <a href="https://gitcode.com/Ascend/msinsight/blob/master/docs/zh/user_guide/figures/readme/xiaozhushou.png">
        <img src="https://camo.githubusercontent.com/22bbaa8aaa1bd0d664b5374d133c565213636ae50831af284ef901724e420f8f/68747470733a2f2f696d672e736869656c64732e696f2f62616467652f5765436861742d3037433136303f7374796c653d666f722d7468652d6261646765266c6f676f3d776563686174266c6f676f436f6c6f723d7768697465" data-canonical-src="./docs/zh/user_guide/figures/readme/xiaozhushou.png" style="max-width: 100%;">
    </a>
</div>
