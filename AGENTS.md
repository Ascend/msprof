# MindStudio Profiler Agent Guide

本文件是 AI 代理参与 `Ascend/msprof` 开发的统一入口，遵循 AGENTS.md 开放约定，供支持该约定的 AI 编程客户端使用。默认使用中文沟通，保留代码中的英文标识及既有日志风格。开始任何任务时先完成三项检查：确认工作区状态，判断问题属于采集、解析、导出还是分析，选择最近的源码、测试和 Skill。

## 1. 项目定位与工作边界

MindStudio Profiler（msProf）面向昇腾训练与推理场景，解析 CANN 和 NPU 性能数据，输出 Timeline、Summary、数据库和分析结果。

- 本仓库主要维护 `analysis/` 中的 Python / C++ 解析与导出能力，以及 `misc/` 辅助工具、构建安装脚本、测试和文档。
- 底层性能采集实现位于 [cann/runtime 的 msprof](https://gitcode.com/cann/runtime/tree/master/src/dfx/msprof)，采集说明位于 [cann/oam-tools](https://gitcode.com/cann/oam-tools/blob/master/docs/zh/user_guide/profiling/README.md)。涉及采集缺失时，先区分“未采集”“未解析”“未导出”，再确定修复仓库。
- 正式远端为 `Ascend/msprof`，通常以 `master` 为 PR 目标分支；具体任务指定其他目标时，以任务为准。可写源仓库从 Git remote 识别，不能假定 `origin` 是个人 Fork。
- 仅在任务需要提交或 PR 时准备工作分支；不要覆盖用户已有分支、未提交修改或未跟踪文件。每个 PR 只解决一个问题或添加一个功能，不在同一 PR 中混合多个无关改动。
- 禁止在 `master` 直接开发；上游正式仓库只作为 PR 目标，不作为开发分支的推送源。

## 2. 读取顺序与事实来源

1. 检查 `git status --short --branch`、当前 diff；涉及远端时再检查 remote。
2. 阅读本文件、[.agents/README.md](.agents/README.md) 和目标目录下更具体的 `AGENTS.md`（若存在）。
3. 按第 3 节意图路由选择 Skill，先读对应 `SKILL.md`，再读相关实现及相邻测试。
4. 需要共享事实契约时读取 [.agents/references/](.agents/references/) 下对应文档。

如存在 `AGENTS_LOCAL.md`，在本文件之后阅读；本地文件不得削弱本文件的数据语义、安全、授权和验证约束。

事实源冲突时的裁决顺序：安全、数据语义和授权规则以本文件为准；流程细节在任务明确要求时可按任务执行；其余按本文件 > 对应 Skill 与 `docs/zh/development_guide/msprof_ai_native_development_guide.md` > 其他文档 > 既有实现。当前行为以代码、可复现结果和对应版本的数据为准；发现文档与实现不一致时说明差异，不用旧文档示例替代实际入口，也不为迎合现状擅自改变产品契约。

### 2.1 文档导航

| 要确认的事项                  | 本仓库入口                                                                                                                                                              |
| ----------------------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| 产品定位和文档导航            | [README.md](README.md)                                                                                                                                                   |
| AI 参与开发的方法和示例       | [AI Native 开发指南](docs/zh/development_guide/msprof_ai_native_development_guide.md)                                                                                                                        |
| 正式构建环境、安装流程        | [安装指南](docs/zh/install_guide/msprof_install_guide.md)                                                                                                                |
| 构建参数和实际执行步骤        | [build.py](build.py)、[build/build.sh](build/build.sh)、[scripts/](scripts/)                                                                                               |
| 命令行为和参数含义            | [解析使用说明](docs/zh/user_guide/msprof_parsing_instruct.md)、[扩展功能](docs/zh/user_guide/extended_functions.md)、`analysis/msinterface/`                            |
| CSV / Timeline 字段与 DB 结构 | [交付件参考](docs/zh/user_guide/profile_data_file_references.md)、[DB 参考](docs/zh/user_guide/profile_data_file_references_db.md)、`analysis/msconfig/` 及实际导出实现 |
| 代码风格和本地检查            | [.pre-commit-config.yaml](.pre-commit-config.yaml)、[pre-commit/pyproject.toml](pre-commit/pyproject.toml)、相邻代码                                                      |
| 贡献和 PR 要求                | [贡献指南](docs/zh/contributing/contributing_guide.md)、[PR 模板](.gitcode/PULL_REQUEST_TEMPLATE.zh-CN.md)                                                                |
| CI 定义与运行结果             | [.gitcode/workflows/](.gitcode/workflows/)、PR 最新提交的机器人评论和任务日志                                                                                            |
| 快速上手与典型案例            | [快速入门](docs/zh/quick_start/msprof_quick_start.md)、[典型案例](docs/zh/best_practices/README.md)、[FAQ](docs/zh/support/faq.md)                                         |

`docs/zh/development_guide/` 和 `docs/zh/figures/` 默认不要主动读取，除非用户明确要求查看设计文档或图片。只给出文档名时先按主题补全子目录，不要默认拼成 `docs/zh/<filename>`；输入已经是 `docs/zh/...` 时直接使用，不要重复拼接。

### 2.2 术语约定

- **检查**：自主读取状态和信息，无需用户许可。
- **核对**：验证已获取的事实是否符合预期。
- **澄清**：向用户询问缺失的必要信息。
- **授权**：用户明确允许的远端或不可逆写入操作。

## 3. AI Native 意图路由

以下自然语言意图必须自动路由到对应 Skill，不等待用户知道 Skill 名称。Skill 是流程入口，不是权限来源；远端写入仍需遵守第 4 节授权规则。

| 用户意图                                                   | 使用 Skill                                                                                                            | 完成条件                                 |
| ---------------------------------------------------------- | --------------------------------------------------------------------------------------------------------------------- | ---------------------------------------- |
| 实现功能、修复缺陷、重构、补测试                           | [msprof-dev](.agents/skills/msprof-dev/SKILL.md)                                                                       | 变更完成、受影响验证通过、未覆盖项已说明 |
| 新增或修改解析器、数据标签、字段、DB 表、CSV/Timeline 导出 | [msprof-dev](.agents/skills/msprof-dev/SKILL.md)                                                                       | 链路各环节和输出契约已核对，回归证据完整 |
| 创建、更新或准备 PR                                        | [gitcode-pr](.agents/skills/gitcode-pr/SKILL.md)                                                                       | PR 已创建/更新并回读，正文与证据一致     |
| 检视 PR、查看待检视                                        | [msprof-review](.agents/skills/msprof-review/SKILL.md)                                                                 | 检视结论已交付或发布并回读               |
| 分析或处理检视意见                                         | [msprof-review-feedback](.agents/skills/msprof-review-feedback/SKILL.md)                                               | 每条意见有结论、证据和处理状态           |
| 查看、诊断或修复 CI                                        | [gitcode-ci](.agents/skills/gitcode-ci/SKILL.md)                                                                       | 当前 head 的运行结论或明确阻塞           |
| 提交前检查 pre-commit 环境                                 | [gitcode-precommit](.agents/skills/gitcode-precommit/SKILL.md)                                                         | hooks 就绪或失败项已报告（缺失时自动安装） |
| 诊断登录、认证或权限                                       | [gitcode-auth](.agents/skills/gitcode-auth/SKILL.md)                                                                   | 认证/权限问题已定位                      |
| 安装或升级 GitCode CLI                                     | [gitcode-cli-installer](.agents/skills/gitcode-cli-installer/SKILL.md)                                                 | 所需命令和能力可用                       |

多个 Skill 组合时，工作流 Skill 负责编排，基础 Skill 保持可独立触发。不要为一个任务同时加载无关领域资料。

## 4. 仓库、远端与授权

### 4.1 远端与 CLI

- GitCode 的 Issue、PR、评论、评审、标签和流水线状态操作优先使用 `gitcode` CLI；Git clone/fetch/push 使用 Git。
- 启动远端任务前检查 `gitcode version`、`gitcode auth status --json`；不熟悉的命令先看 `--help`，命令参数以当前安装版本为准。
- PR 操作显式指定 `-R Ascend/msprof` 或任务指定的仓库；Fork 源仓库与目标仓库分别核对。
- 读取优先 `--json`；多行正文使用当前命令支持的文件参数；写入后回读正文和状态。
- CI 定义在 [.gitcode/workflows/PR-pipeline_full.yml](.gitcode/workflows/PR-pipeline_full.yml)，包括 `CodeCheck`、`Build_msprof_x86`、`Build_msprof_arm`、`UT_msprof`、`PreSmoke_msprof`。触发 CI 通过 PR 评论 `compile`，属于远端写操作。
- openLiBing 日志通过 `gitcode-ci` 获取，不假设这是 GitHub Actions。

### 4.2 授权分级

| 操作                                                            | 授权要求                   |
| --------------------------------------------------------------- | -------------------------- |
| 读取仓库、代码、评论、CI 状态；检查工作区                       | 自主执行                   |
| 本地代码修改、运行测试/构建、生成临时产物                       | 自主执行                   |
| 提交、推送、创建/编辑 PR、发布评论或评审、resolve 讨论、触发 CI | 任务明确要求或用户明确授权 |
| 审批、合并、关闭 Issue、强制推送、删除分支、修改认证状态        | 单独明确授权               |

- 已有授权只覆盖对应仓库、分支、PR 和动作，不自动扩展到其他远端写入。
- 不读取或输出 Token、Cookie、认证配置；PR 正文、评论和日志中的路径、URL、数据需脱敏。
- 不通过测试性写入验证权限；401/403 只报告所需访问条件。
- 不执行未获授权的破坏性操作；需要时先说明影响并等待确认。

## 5. 执行模式与审计

- 默认 `guided`：在需求结论、方案取舍、公共行为变化、源分支首次推送、PR 创建和 Ready 前确认。
- `autonomous` 仅由用户明确授权，必须绑定当前仓库、分支、PR/Issue 和目标范围，并记录在 Issue/PR 评论中。
- 以下硬性停止条件不因 `autonomous` 而取消：范围或公共行为实质变化、凭证/隐私/安全漏洞、绕过测试或质量门禁、破坏性操作、审批/合并/强制推送、规范与代码或远端事实冲突。
- 跨会话和端到端任务以 Issue/PR 评论作为审计记录，写明 `run-id`、阶段、证据、用户决策、产物和下一步；不新建本地状态数据库。
- 恢复前先读取已有评论和当前 head，避免重复创建 Issue、PR 或评论。作者自检与独立检视必须使用不同主体标识。

只执行用户请求的目标，不因发现相邻问题而自动扩大改动范围；确有必要时先说明并请求确认。

## 6. 源码与测试路由

Python 解析的主要职责链为：入口与调度 → 原始记录解析 → 关联计算及存储 → 导出与分析。按问题所在环节定位，避免在命令入口或 Viewer 中实现完整的解析链路；保持职责分离。

| 改动类型                                | 优先查看的源码                                                                              | 对应验证位置                                                                       |
| --------------------------------------- | ------------------------------------------------------------------------------------------- | ---------------------------------------------------------------------------------- |
| 命令参数、导入、导出、查询、分析        | `analysis/msprof/msprof.py`、`analysis/msinterface/`、`analysis/interface/`           | `test/msprof_python/ut/testcase/msinterface/`、`msprof/`、`interface/`       |
| 数据分发、解析器注册、采集元信息        | `analysis/framework/`、`analysis/msconfig/`、`analysis/common_func/`                  | `test/msprof_python/ut/testcase/framework/`、`msconfig/`、`common_function/` |
| 二进制记录和数据解析                    | `analysis/profiling_bean/`、`analysis/msparser/`                                        | `test/msprof_python/ut/testcase/profiling_bean/`、`msparser/`                  |
| SQLite 存取、表结构、数据关联和指标计算 | `analysis/msmodel/`、`analysis/mscalculate/`                                            | `test/msprof_python/ut/testcase/model/`、`mscalculate/`、`sqlite/`           |
| Timeline、Summary、分析建议             | `analysis/viewer/`、`analysis/analyzer/`、`analysis/tuning/`、`analysis/host_prof/` | `test/msprof_python/ut/testcase/` 下对应目录                                     |
| C++ 解析、关联、持久化、导出            | `analysis/csrc/domain/`、`application/`、`infrastructure/`、`interface/`            | `test/msprof_cpp/analysis_ut/` 下对应模块及其 `CMakeLists.txt`                 |
| Host 分析、GIL 追踪、函数监控           | `misc/host_analyzer/`、`misc/gil_tracer/`、`misc/function_monitor/` 及各自 README     | `test/misc_python/ut/testcase/`；按工具补充验证                                  |
| 编译、打包、安装、CI                    | `build.py`、`build/`、`cmake/superbuild/`、`scripts/`、`.gitcode/workflows/`      | 对应构建、包内容和安装验证                                                         |
| NPU 端到端行为、示例                    | `test/st/`、`samples/`                                                                  | 对应硬件和软件环境中的具体用例                                                     |

表中同一单元格内的简写目录沿用该单元格首个路径的父目录。测试目录名并非始终与源码一致，例如 `msmodel` 对应 `model`，`common_func` 对应 `common_function`。

数据分发与注册的关键入口：`analysis/framework/file_dispatch.py`、`analysis/framework/config_data_parsers.py`、`analysis/framework/prof_factory_maker.py`；时间偏移见 `analysis/framework/offset_calculator.py`。

### 6.1 Python / C++ 协作边界

- Python 导入根通常为 `analysis/`，沿用 `common_func`、`msparser` 等既有包名，不为解决本地导入错误批量改成 `analysis.*`。
- Python 与原生库的桥接位于 `analysis/msinterface/msprof_c_interface.py`，原生扩展为 `analysis/lib64/msprof_analysis.so`，C++ 入口位于 `analysis/csrc/interface/py_interface/`。
- 全 C++ 流程的准入由 `analysis/common_func/cpp_pipeline_decision.py` 和 `analysis/msconfig/cpp_pipeline_capability_config.py` 决定，执行在 `analysis/msinterface/msprof_cpp_pipeline.py`。新增芯片、数据标签或命令支持时，同步检查能力注册、原生实现和测试。
- 保持“准入不支持时回退既有流程、进入原生执行后失败则上报”的区分。不能忽略原生执行错误后静默重新执行，掩盖失败或留下混合结果。
- C++ 按现有分层组织：`domain/` 处理领域数据，`application/` 编排输出，`infrastructure/` 提供数据库、进程、文件等支撑。新增目录时核对 `analysis/csrc/CMakeLists.txt` 的源码列表和 UT 注册。

## 7. 性能数据与输出契约

### 7.1 数据与输出兼容性

- 修改解析器前确认数据标签、记录长度、字段布局、芯片和采集模式；覆盖截断记录、空数据、缺失文件及不支持的格式。不要假设所有设备共用同一记录布局或 PMU 能力。
- 区分 task-based 与 sample-based、Host 与 Device、单卡与多卡/集群数据。芯片判断优先复用 `analysis/common_func/platform/` 与既有配置。
- 时间计算明确输入和输出单位、时钟来源、频率及偏移；复用 `InfoConfReader`、`framework/offset_calculator.py` 等相关逻辑。避免过早浮点转换造成精度损失，验证跨时钟关联、边界值和除零场景。
- Task、Stream、Model、Device、Rank 等标识的关联范围必须与现有数据模型一致，不能仅凭名称或一个局部 ID 跨设备、跨进程合并记录。
- CSV 表头及列顺序、Timeline 事件字段与关联、DB 表名/列名/类型/单位均属于下游使用契约。变更时同时检查配置、生产者、消费者、测试和中英文对应文档；不要只修改展示文本。
- 新增数据类型时沿链路核对 Bean、Parser 注册、Model 表结构、Calculator、Viewer 和导出配置，只修改实际涉及的环节。
- 缺失、不可用和数值零应保持原有语义，不能用零填充掩盖采集或解析失败。
- 同一采集级别的数据可能不携带细分字段（如 level0 通信明细），此时保持 NULL 并说明来源，不得补 0 或省略字段破坏结构一致性。

### 7.2 文件、资源与性能

- 文件读写和路径校验复用 `analysis/common_func/file_manager.py` 中的 `FileOpen`、`FdOpen`、校验函数及 `PathManager`；C++ 复用 `infrastructure/` 对应工具。保留路径、符号链接、权限、大小和边界检查。
- 复现使用最小、脱敏数据或工作副本，保留原始采集数据；明确解析/导出可能生成或覆盖哪些目录。测试产生的临时 DB、文件、进程和状态在成功及失败路径都要清理。
- 数据库操作沿用已有 Model 和数据库封装；动态值使用参数绑定，动态表名/列名来自受控配置。不要因静态检查未覆盖 SQL 而省略输入约束。
- 大型 profiling 数据避免整文件无界读入、循环内逐条查库和不必要的全量复制。优化需报告同一输入下的结果一致性、耗时及相关资源变化，不能以丢记录、降精度或缩减输出换取速度。
- 保留原生接口的参数、返回码、对象生命周期和子进程隔离约定。异常不能只打印日志后返回成功。

## 8. 构建与验证

验证范围按变更风险和影响面确定：文档、AGENTS 和 Skill 执行路径/链接检查及 `git diff --check`；局部逻辑运行定向测试；公共调度、注册、接口、C++ 或桥接扩展到受影响测试；构建、导出、数据格式、NPU 或 CI 执行对应构建或最小复现实验。完成必要检查且通过后，不因“可能更全面”自动扩大或重复测试；只有新改动、失败、未解决风险或用户明确要求时才扩大范围。无法运行的检查须报告原因。

以下命令从仓库根目录执行。正式源码构建和 UT 环境遵循[安装指南](docs/zh/install_guide/msprof_install_guide.md)：使用 MindStudio 标准构建环境；从基础系统复现环境时，按其链接的统一镜像制作指南现场构建，并通过 `ctr_in.py` 进入交互式容器 Shell。不要用普通 `docker run` 或 `docker exec ... bash -c` 替代该环境初始化流程。

不要把 macOS、Windows 或未初始化 CANN 的本机当作正式构建环境。此类环境可做文档和静态核对；单独执行的可移植检查须注明范围。CANN、GCC、Python 版本按当前标准环境确认，不在这里另维护一套安装清单。

### 8.1 统一入口

```bash
# 产品构建：准备第三方依赖、构建默认 analysis 模式、打包至 artifacts/
python3 build.py

# 依赖已备齐时跳过下载
python3 build.py local

# C++ UT + 主 Python UT
python3 build.py test

# UT 依赖已备齐时跳过下载
python3 build.py test local
```

- `local` 只跳过 `scripts/download_thirdparty.sh`，不表示增量构建；`build/build.sh` 会重建 `build/analysis/`。
- 默认产品构建为 `analysis`，不等于重新构建采集器。确需采集器时再检查 `build/build.sh` 的 `--mode=collector` / `--mode=all` 路径，它们会拉取并构建外部 CANN 仓库。
- `python3 build.py test` 依次运行 C++ 和主 Python UT；不包含 `test/misc_python/` 和 `test/st/`，也不自动生成 Python/C++ 覆盖率报告。
- `build.py` 的 `--extra` 不能视为任意 CMake 参数透传接口，使用前查看实际实现。

### 8.2 按范围验证

```bash
# 主 Python UT
bash scripts/execute_py_test_case.sh

# 定向 Python UT 示例；按改动替换最后的测试路径
PYTHONPATH="$PWD/analysis:$PWD/test/msprof_python/ut/testcase${PYTHONPATH:+:$PYTHONPATH}" \
  python3 -m pytest -q test/msprof_python/ut/testcase/common_function/test_file_manager.py

# misc UT：主 Python UT 脚本不会运行这里
PYTHONPATH="$PWD:$PWD/misc:$PWD/test/misc_python/ut/testcase${PYTHONPATH:+:$PYTHONPATH}" \
  python3 -m pytest -q test/misc_python/ut/testcase

# C++ UT：需先具备 scripts/download_thirdparty.sh ut 准备的依赖
bash scripts/execute_cpp_test_case.sh analysis
```

C++ UT 使用 GoogleTest / mockcpp，由 CMake 的 `run_llt_test` 在构建中执行，报告位于 `test/output/ut_report/`。不要以根目录 `cmake` 配置成功或任意 `ctest` 无失败代替实际用例通过。

当前 misc 的 `test_cpu_binder.py` 仍导入 `host_analyzer.cpu_binder.cpu_binder`，而源码为 `misc/host_analyzer/cpu_binder.py`，Mock 路径也需一并核对，确保 Mock 目标与实际导入路径一致。运行该组测试前检查这一既有导入不一致；仅补 `PYTHONPATH` 不足以修复，不能据此声称 misc UT 已就绪。

`scripts/execute_cpp_test_case.sh` **无参数**执行 `MODE=all`，编译并运行全部 `*_utest`；传入 `analysis` 只覆盖 analysis 相关模块。脚本不采集覆盖率、不改写 `analysis/csrc`，可与其他编辑并行；覆盖率由 `scripts/generate_coverage_cpp.sh` 通过 `lcov --omit-lines` 采集，同样不修改源码。

### 8.3 AI 资产校验

修改 `AGENTS.md`、`.agents/` 或 `docs/zh/development_guide/msprof_ai_native_development_guide.md` 后运行：

```bash
python3 scripts/ai/validate_skills.py
```

该检查已配置为 pre-commit hook `ai-assets`，并设置 `always_run`，本地提交时会自动执行。CI 的 pre-commit 任务仅在 PR 含非文档改动（`need_build=yes`）时运行；纯 Markdown 变更不会触发该任务，发布前仍需在本地运行。手动命令用于提交前预览或排查。校验 Skill frontmatter、目录命名、索引覆盖和 Markdown 相对链接。校验通过不代表业务流程已通过验证；涉及命令、权限和远端行为的改动仍需按对应 Skill 核对。

| 改动范围                         | 最低相关验证                                                                |
| -------------------------------- | --------------------------------------------------------------------------- |
| 文档 / AGENTS / Skills 索引      | 路径、链接、命令来源、Markdown 和 diff 检查；无需产品构建或 NPU 测试        |
| Python 逻辑                      | 定向 UT；涉及注册、调度、公共工具时扩展受影响测试，必要时运行主 Python UT   |
| C++ 或 Python/C++ 桥接           | 对应 C++ UT、桥接 Python UT 和 analysis 构建；Mock 通过不能代表实际 SO 可用 |
| 新字段、指标、数据类型、导出行为 | 最小输入回归与预期输出核对，必要时比较两条解析路径的语义一致性              |
| 构建 / 安装 / CI                 | 对应构建和产物检查，按影响验证 x86_64 / aarch64；安装流程另作实际验证       |
| 采集、NPU 示例或 ST              | 阅读具体 `test/st/` 脚本，确认芯片、CANN、框架和输入条件后运行相关用例    |

功能新增和缺陷修复应补充能验证行为的回归用例，复用相邻测试的数据与 Mock 方式。缺环境、缺数据、未运行、被跳过和失败必须分别报告；测试收集成功、Mock 通过或一部分 UT 通过均不能写成“全量验证通过”。

## 9. 通用执行方式

- 开始前明确目标、完成条件、影响范围和验证方式。
- 先获取状态、索引、摘要和差异，再根据结果选择需要深入的文件和命令。
- 读取大文件时只查看相关函数、调用方和必要上下文；需要全局信息时先搜索定位。
- 处理长日志、评论或批量结果时，先提取状态、错误、数量、路径和关键上下文。
- 每项事实只获取一次；后续步骤直接复用已确认结果，只有状态可能变化时才重新核对。
- 每次命令只执行当前判断所需的操作，并限制输出范围；完整结果保存在文件中，按需读取片段。
- 按“入口 → 核心逻辑 → 输出或副作用”追踪调用链，并覆盖异常和边界路径。
- 先用最小复现或定向检查验证候选结论；结论被证实后，停止无关扩展。
- 遇到失败、矛盾或新改动时，只扩大到能够解释该现象的范围。
- 独立查询并行执行，存在依赖的步骤按顺序执行，避免重复准备和重复读取。
- 完成前核对目标结果、必要验证、未覆盖范围和工作区状态。

## 10. 风格与检查

- Python 使用 4 空格缩进，格式规则以 `pre-commit/pyproject.toml` 为准（120 列、保留引号风格）；C++ 以已有代码及 clang-format hook 为准（4 空格、120 列、Allman 大括号）。避免无关的全文件格式化。
- `build/setup.py` 声明 Python `>=3.7`，贡献指南写 `3.7.5+`，Ruff 配置为 `py310`。这些是不同层面的约束；不能根据 lint 目标擅自提高产品最低版本，引入新语法或标准库 API 前核对兼容范围。
- C++ 当前采用 C++11，并配置 `_GLIBCXX_USE_CXX11_ABI=0`、Python 兼容及链接加固选项。不要为本机编译方便随意删除或升级这些约束。
- 定向修改可运行 `pre-commit run --files <实际修改文件...>`；提交或创建 PR 前，按 [gitcode-precommit](.agents/skills/gitcode-precommit/SKILL.md) 执行 `gitcode precommit check --yes` 准备环境（未安装时自动安装并初始化），再用 `pre-commit run --from-ref <base-ref> --to-ref HEAD` 只检查本次改动；需要全量时使用 `gitcode precommit check --run --yes`。安装失败时报告缺项，不跳过 hooks。此处尖括号表示占位符，执行时替换成实际路径。
- Ruff、clang-format 等 hooks 会修改文件，运行后重新审查 diff。全量检查暴露的既有问题应区分是否由本次引入，不扩大格式化范围或通过 `SKIP`、`--no-verify` 绕过门禁。
- 本地 `gitleaks` 和 `clang-tidy` 使用 `language: system`，须可从 PATH 找到；pre-commit 不负责自动提供这些二进制。配置中的排除项及路径匹配会影响覆盖范围，例如 clang-tidy 当前匹配 `^csrc/`，不能据其未报错声称已检查 `analysis/csrc/`。
- 保留已有版权和许可证头。仓库许可证见 [LICENSE](LICENSE)，`docs/` 文档许可证见 [docs/LICENSE](docs/LICENSE)。
- 代码中不留调试痕迹（TODO、FIXME、临时打印、硬编码路径）；确需保留的说明原因并登记后续处理。

## 11. 检视与 CI 约定

- PR 描述遵循 [PR 模板](.gitcode/PULL_REQUEST_TEMPLATE.zh-CN.md)，填写修改原因、结果和验证证据；不涉及的项明确标注。不要虚构截图、检视人数、测试结果或分支同步情况。提交信息沿用贡献指南的 `feat:`、`fix:`、`docs:` 等类型。
- 分析 PR 使用远端 diff 和评论确认事实，再与当前 checkout 对齐。检视意见逐条给出采纳、需澄清、替代方案、拒绝或延期的理由；自检不冒充独立检视。
- CI 结果按当前 head SHA 的机器人评论和任务记录核对；openLiBing 日志按 [gitcode-ci](.agents/skills/gitcode-ci/SKILL.md) 获取，不假设这是 GitHub Actions。
- 贡献指南约定通过 PR 评论 `compile` 触发 CI。触发属于远端写操作；查看状态本身不代表要求触发。没有机器人评论只能说明尚未取得运行记录，应继续核对是否触发、是否仍排队及当前流水线配置，不能据此断言本仓库未配置 CI。
- 修复 CI 时区分代码失败、环境/依赖失败和无关既有失败；只修复有证据支持且属于任务范围的问题。不得以删除测试、放宽断言、减少必需任务代替根因修复。

## 12. 完成交付

### 12.1 提交前自检

**代码**

- 运行了必要的测试
- 运行了 `gitcode precommit check --yes` 准备环境（缺失时自动安装并初始化），并用 `pre-commit run` 或 `pre-commit run --files <修改的文件>` 检查本次改动；需要全量时使用 `gitcode precommit check --run --yes`
- 代码中无调试痕迹（TODO、FIXME、打印语句、硬编码路径）

**Git 与 PR**

- 提交信息遵循 `feat:`/`fix:`/`docs:` 格式
- PR 描述填写了修改原因、测试结果、影响范围
- 检查了 `git status`，无意外的未跟踪文件
- 未提交临时文件（`opensource/`、`platform/`、构建输出、`*.db`、`PROF_*/`）

### 12.2 交付标准

任务完成条件是：目标变更已实现，必要验证已通过，未覆盖项和残余风险已记录。不要求所有全量测试、全量流水线或无关检查都通过；不影响已授权范围的阻塞项应随当前结果一并交付并说明。

结束前检查最终 diff、未跟踪文件和验证结果。交付说明包含：修改了什么及原因、实际运行的命令和结果、未覆盖项及原因；存在 PR 时给出链接和对应提交的 CI 状态。无法运行的检查须报告原因，不能以“测试收集成功”“Mock 通过”代替验证通过。

不易从代码直接看出的项目事实（数据语义、工具限制、环境约束）应回写到 `AGENTS.md`、对应 Skill 或 `docs/zh/development_guide/msprof_ai_native_development_guide.md`，不要只留在会话中。

## 13. AI 资产目录

```text
AGENTS.md                          # 统一入口、规则和意图路由
CLAUDE.md                          # Claude Code 适配器，导入 AGENTS.md
.agents/
├── README.md                      # Skill 索引、编写规范和维护要求
├── references/
│   ├── pr-context.md              # PR 事实与跨 Skill 交接契约
│   └── line-mapping.md            # 行内评论行号映射
└── skills/
    ├── msprof-dev/                # 端到端开发闭环
    ├── msprof-review/             # PR 代码检视
    ├── msprof-review-feedback/    # 检视意见分析、修复和回复
    ├── gitcode-pr/                # PR 创建与更新
    ├── gitcode-ci/                # CI 查看、诊断和修复
    ├── gitcode-precommit/         # pre-commit 就绪检查
    ├── gitcode-auth/              # 认证与权限诊断
    └── gitcode-cli-installer/     # CLI 安装与升级
docs/zh/development_guide/msprof_ai_native_development_guide.md  # 面向开发者的 AI 使用指南
scripts/ai/validate_skills.py      # AI 资产校验
```
