# 开发指南

本文面向 MindStudio Profiler 的开发和维护人员，介绍源码目录、构建方式、采集与解析链路、功能改动后的验证方法，以及资料联动更新要求。本文重点结合 MindStudio Profiler 当前仓库和现有文档内容编写，适用于新增命令参数、扩展解析能力、增加交付件或维护 run 包安装方式等场景。

## 1. MindStudio Profiler 开发概述

MindStudio Profiler 提供 AI 任务运行性能数据和昇腾 AI 处理器系统数据的采集、解析与导出能力。围绕开发工作，通常可以分为以下几类：

| 开发对象 | 典型内容 |
| --- | --- |
| 采集能力 | `msprof` 命令参数、采集流程、原始数据落盘 |
| 解析能力 | `msprof --export`、`msprof.py import/query/export`、通信数据解析 |
| 导出交付件 | `msprof_*.json`、`op_summary_*.csv`、`msprof_*.db` 等 |
| 打包发布 | `build/build.sh` 编译、生成 `mindstudio-profiler_<version>_<arch>.run` |
| 文档资料 | 安装指南、快速入门、解析说明、数据文件参考、扩展功能 |

## 2. 代码目录

根据当前仓库资料，msProf 项目主要目录如下：

| 目录 | 说明 |
| --- | --- |
| `analysis` | 数据解析主目录 |
| `analysis/analyzer` | 通信数据解析 |
| `analysis/framework` | 解析主流程 |
| `analysis/interface` | 解析接口 |
| `analysis/msinterface` | 命令行参数解析 |
| `analysis/msmodel` | DB 处理 |
| `analysis/msparser` | 二进制数据解析流程管理 |
| `analysis/msprof` | 解析入口 |
| `analysis/viewer` | timeline、summary、db 等交付件导出 |
| `build` | 构建目录，包含 `build.sh` |
| `scripts` | 三方依赖下载、run 包安装与升级脚本 |
| `test` | C++ 和 Python 测试代码 |
| `docs/zh` | 中文资料 |

在开发前，建议先明确改动落在哪一层：

1. 命令参数类改动，优先检查 `analysis/msinterface` 和对外文档说明。
2. 解析逻辑类改动，优先检查 `analysis/framework`、`analysis/msparser`、`analysis/msmodel`。
3. 导出文件类改动，优先检查 `analysis/viewer` 及数据文件参考文档。
4. 安装和发布类改动，优先检查 `build`、`scripts` 以及安装指南。

## 3. 开发环境配置

### 3.1 基础软件

| 软件名 | 版本要求 | 用途 |
| --- | --- | --- |
| Git | 无硬性要求 | 代码拉取与提交 |
| Python | 3.7.5 及以上 | 解析脚本运行 |
| SQLite3 | 编译时依赖 | 解析相关能力 |
| Bash | Linux 环境下建议具备 | 构建与脚本执行 |

### 3.2 前置条件

1. 已安装配套版本的 CANN 环境。
2. 具备可用的 `cann` 安装目录。
3. 具备源码编译所需的 SQLite3 依赖。

示例：

```bash
sudo apt update
sudo apt install sqlite3 libsqlite3-dev
```

或：

```bash
sudo yum install sqlite sqlite-devel
```

## 4. 获取代码与构建

### 4.1 获取代码

```bash
git clone https://gitcode.com/Ascend/msprof.git
cd msprof
```

### 4.2 下载三方依赖

```bash
bash scripts/download_thirdparty.sh
```

### 4.3 编译 run 包

`build/build.sh` 支持通过 `--mode` 指定编译内容：

| 模式 | 说明 |
| --- | --- |
| `all` | 编译包含采集和解析能力的全量 run 包 |
| `collector` | 编译仅包含采集能力的 run 包 |
| `analysis` | 编译仅包含解析能力的 run 包 |

示例：

```bash
bash build/build.sh --mode=all --version=<version>
```

或：

```bash
bash build/build.sh --mode=analysis --version=<version>
```

编译成功后，会在 `output` 目录下生成 `mindstudio-profiler_<version>_<arch>.run`。

### 4.4 安装 run 包

```bash
cd output
chmod +x mindstudio-profiler_<version>_<arch>.run
./mindstudio-profiler_<version>_<arch>.run --install
```

安装完成后，建议立即校验：

```bash
which msprof
msprof --help
```

## 5. 功能开发流程

### 5.1 新增或修改命令参数

若本次改动涉及 `msprof` 或 `msprof.py` 的命令参数，应至少完成以下工作：

1. 实现参数解析和默认值处理。
2. 校验参数约束、互斥关系和错误提示。
3. 核对帮助信息、示例命令和输出说明。
4. 同步更新解析说明或扩展功能文档。

常见联动文档如下：

| 改动类型 | 需同步检查的文档 |
| --- | --- |
| `msprof --export` 参数 | `zh/user_guide/msprof_parsing_instruct.md` |
| `msprof.py import/query/export` 参数 | `zh/user_guide/extended_functions.md` |
| 安装、卸载、校验参数 | `zh/getting_started/msprof_install_guide.md` |
| 首次体验路径或基础示例 | `zh/getting_started/quick_start.md` |

### 5.2 新增解析能力

若本次改动涉及新的解析逻辑，例如增加新的统计项、支持新的采集场景或增强通信解析，建议按如下顺序自检：

1. 原始数据是否能成功导入。
2. `msprof --export=on` 是否能正常导出。
3. `python3 msprof.py import/query/export` 是否仍保持一致。
4. 导出的 timeline、summary、db 文件是否包含预期内容。
5. 失败场景下的报错是否可定位。

### 5.3 新增交付件或字段

若本次改动会新增或变更交付件，例如新增 `xx_*.csv`、修改 `msprof_*.json` 中层级，或新增 DB 表字段，建议至少完成以下工作：

1. 明确交付件名称、生成条件和输出路径。
2. 确认是否会影响 `mindstudio_profiler_output` 目录结构。
3. 明确字段含义、单位、取值范围和适用场景。
4. 同步更新数据文件参考文档。

重点联动文档：

| 变更对象 | 需更新文档 |
| --- | --- |
| timeline / summary / text 交付件 | `zh/user_guide/profile_data_file_references.md` |
| DB 表或字段 | `zh/user_guide/profile_data_file_references_db.md` |
| 用户可见的新能力 | `zh/user_guide/extended_functions.md` |

## 6. 开发验证

### 6.1 采集验证

功能开发完成后，建议先完成一轮最小化采集验证：

```bash
msprof --application="python train.py" --output=/home/prof_output
```

若功能正常，通常会在输出目录下生成 `PROF_XXX`，并包含如下结构：

```text
PROF_XXX
├── host
│   └── data
├── device_{id}
│   └── data
├── msprof_{timestamp}.db
└── mindstudio_profiler_output
    ├── msprof_{timestamp}.json
    ├── op_summary_{timestamp}.csv
    └── ...
```

### 6.2 解析验证

对解析链路的改动，建议至少验证以下命令：

```bash
msprof --export=on --output=/home/profiler_data/PROF_XXX
```

```bash
python3 msprof.py import -dir /home/profiler_data/PROF_XXX
```

```bash
python3 msprof.py query -dir /home/profiler_data/PROF_XXX
```

```bash
python3 msprof.py export timeline -dir /home/profiler_data/PROF_XXX
```

必要时继续验证：

```bash
python3 msprof.py export summary -dir /home/profiler_data/PROF_XXX
python3 msprof.py export db -dir /home/profiler_data/PROF_XXX
```

### 6.3 结果验证

重点检查以下内容：

1. `mindstudio_profiler_output` 是否生成预期文件。
2. `msprof_*.json` 是否能被可视化工具正常加载。
3. `op_summary_*.csv`、`op_statistic_*.csv` 等文件字段是否齐全。
4. `msprof_*.db` 是否成功生成。
5. 若功能涉及 `--reports`、`--iteration-id`、`--model-id`、`--clear`，需覆盖对应参数场景。

## 7. 资料联动更新要求

MindStudio Profiler 的开发改动通常会直接影响资料，禁止只改代码不改文档。建议按下表同步核对：

| 改动内容 | 必查文档 |
| --- | --- |
| 新增安装方式、依赖、打包参数 | `zh/getting_started/msprof_install_guide.md` |
| 新增基础使用流程或示例 | `zh/getting_started/quick_start.md` |
| 新增解析流程或命令参数 | `zh/user_guide/msprof_parsing_instruct.md` |
| 新增脚本能力或高级用法 | `zh/user_guide/extended_functions.md` |
| 新增导出文件、字段、图层 | `zh/user_guide/profile_data_file_references.md` |
| 新增 DB 数据结构说明 | `zh/user_guide/profile_data_file_references_db.md` |
| 产品能力概述变化 | `zh/overview.md` |

若新增截图或示意图：

1. 图片统一放在 `zh/figures`。
2. 文件名应与功能语义对应。
3. 正文中的图题、路径、说明文字要同步更新。

## 8. 提交前检查

提交前建议至少完成以下检查：

| 检查项 | 说明 |
| --- | --- |
| 构建检查 | run 包可成功编译 |
| 安装检查 | run 包可成功安装，`msprof --help` 可用 |
| 采集检查 | 能生成 `PROF_XXX` |
| 导出检查 | 能生成 json、csv 或 db 交付件 |
| 文档检查 | 参数、示例、输出说明已同步 |
| 兼容性检查 | 默认场景不回归，旧参数行为不变 |

若本次改动涉及用户可见行为，建议在提交说明中写清：

1. 改动的是采集、解析、导出还是安装链路。
2. 是否新增了参数、交付件或数据字段。
3. 已验证的命令和场景。
4. 已同步更新的文档范围。
