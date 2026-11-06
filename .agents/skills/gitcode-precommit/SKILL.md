---
name: gitcode-precommit
description: 使用 GitCode CLI 的 `precommit check` 检查并准备 msprof 仓库的 pre-commit 环境；提交或创建 PR 前检测到未安装时自动安装、初始化并运行 hooks。需要 GitCode CLI 0.6.0 及以上。
metadata:
  version: 1.0.0
  repository: Ascend/msprof
---
# gitcode-precommit

在提交代码或创建 PR 前，确认 msprof 已配置 pre-commit 且本地可运行全部 hooks。需要 GitCode CLI 0.6.0 及以上（`precommit` 命令起始版本），可用 `gitcode version` 或 `gitcode help precommit check` 确认。

## 使用

从目标 Git 仓库根目录运行；Linux/macOS 可用 `gc`，Windows PowerShell 用 `gitcode`。

```bash
# 准备环境：检测到未安装时自动安装并初始化，不运行 hooks
gitcode precommit check --yes

# 只检查已暂存文件（pre-commit 默认范围）
pre-commit run

# 只检查指定文件
pre-commit run --files <文件...>

# 只检查相对 base 的改动
pre-commit run --from-ref <base-ref> --to-ref HEAD

# 全仓库运行：--run 没有增量参数，会对全部文件执行
gitcode precommit check --run --yes

# 只诊断，不安装或修改环境
gitcode precommit check --no-install --json
```

- 提交或创建 PR 前先执行 `gitcode precommit check --yes` 完成环境准备；检测到 `tool_installed: false` 或 `reason: tool_missing` 时直接自动安装和初始化，不要求用户预装。该授权只覆盖本地 pre-commit 环境准备，不扩展到提交、推送、PR 写入、评论或触发 CI。
- `gitcode precommit check --run` 没有增量参数，会对仓库全部文件运行；只检查修改文件时使用 `pre-commit run`（已暂存文件）、`pre-commit run --files` 或 `pre-commit run --from-ref <base-ref> --to-ref HEAD`。
- `always_run: true` 的 hook（如 `ai-assets`）在增量模式下仍会运行。
- 只检查状态用 `--no-install`；`--no-install` 与 `--yes` 互斥。
- 命令检测 `.pre-commit-config.yaml|.yml`、`pre-commit` 工具以及 pre-commit/pre-push hooks；`--run` 运行两个 stage 的全部文件检查。
- 解析 `--json` 结果：`ok` 需结合 `config_found`，`ok: true, config_found: false` 只表示无配置、跳过检查；`run_result: failed` 时检查 `run_output`；`actions_taken` 记录已执行的安装或初始化；未就绪时读取 `reason` 和 `pre_commit_hook_installed`/`pre_push_hook_installed`。
- 退出码：`0` 就绪或无配置；`1` 未就绪、检查失败、非 Git 仓库或非交互环境拒绝修改；`2` 用法错误。
- 自动安装依次尝试 `pipx`、`python3 -m pip install --user`、`python -m pip install --user`，Windows 另试 `py` 启动器；无 Python 工具时输出手动安装说明。
- 安装或初始化失败时报告 `reason`、已尝试的方式和手动安装命令，停止提交流程；不得改用 `--no-install` 跳过 hooks 后继续。
- `--run` 可能改写文件内容，运行后检查 `git diff` 再提交。

## msprof 适配

- 配置位于仓库根 `.pre-commit-config.yaml`，包含基础文件检查、Ruff、codespell、pylint、Bandit、typos、clang-format、clang-tidy、本地 gitleaks 和 `ai-assets`。
- `clang-tidy` 依赖本地 clang 工具链，`gitleaks` 依赖可执行文件；缺失时记录具体失败原因。
- pre-commit 通过不等于 UT、构建或 NPU 冒烟通过，按 [AGENTS.md](../../../AGENTS.md) 第 8 节另行验证。
- 本 Skill 只负责检查和准备本地环境，`--json` 结果交给调用方决定后续动作。

## 规则

- 只在目标仓库根目录运行；安装失败报告具体缺项，不绕过 hooks。
- 用户只要求检查状态时使用 `--no-install`；需要运行 hooks 时按任务范围准备环境。
