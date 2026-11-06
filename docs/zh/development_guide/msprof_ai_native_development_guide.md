# msProf AI Native 开发指南

本指南帮助开发者使用 AI 代理参与 msProf 的需求分析、开发、验证、PR 检视和 CI 闭环。AI 代理的强制规则以 [AGENTS.md](../../../AGENTS.md) 为入口；Skill 索引和编写规范见 [.agents/README.md](../../../.agents/README.md)。

## 1. 首次使用

### 1.1 前置条件

| 项目 | 要求 |
| --- | --- |
| GitCode CLI | 当前基线为 0.15.0；CLI 缺失时使用 `gitcode-cli-installer` |
| 认证 | `gitcode auth status --json` 可读；未登录或权限不足使用 `gitcode-auth` |
| Python | 3.7+ |
| Git | 支持 `git worktree` |
| 网络 | 可访问 `gitcode.com`；使用网络代理时，确认 CLI 和 Git 使用相同的代理配置 |
| 客户端 | Cursor、Codex、OpenCode 读取 `AGENTS.md`；Claude Code 读取 `CLAUDE.md`（已导入 `AGENTS.md`）；其他客户端以官方文档为准 |

GitCode 个人访问令牌（PAT）在“头像 > 个人设置 > 访问令牌 > 新建访问令牌”创建，只授予任务所需的最小权限。不要把 Token 粘贴到对话、命令参数或提交文件中。

### 1.2 启动检查

在仓库根目录运行：

```bash
git status --short --branch
gitcode version
gitcode auth status --json
python3 scripts/ai/validate_skills.py
```

预期：工作区状态明确、CLI 已认证、AI 资产校验通过。任一项失败时先处理对应问题，不在 Skill 缺失或认证不可用时继续任务。

### 1.3 第一个任务

1. 在仓库根目录启动 AI 客户端，确认它读取了 `AGENTS.md`；Claude Code 读取 `CLAUDE.md`，其中已导入 `AGENTS.md`。
2. 直接描述目标，不需要记住 Skill 名称；代理会按意图路由加载对应 Skill。
3. 在方案、公共行为变化、首次推送和 PR 创建等关键点确认后再继续。
4. 交付前要求代理给出实际执行的命令、结果和未覆盖项，而不是只给结论。

> 命令示例以 bash 为准；Windows PowerShell 使用等价命令和正斜杠路径。`python3` 在 Windows 上通常写作 `python` 或 `py -3`。

## 2. 常见场景

| 目标 | 示例说法 | 代理加载的 Skill |
| --- | --- | --- |
| 实现功能 | “实现 xxx 功能，先给方案再改代码” | `msprof-dev` |
| 修复缺陷 | “修复 xxx 在 level0 数据下崩溃的问题” | `msprof-dev` |
| 解析/导出变更 | “新增 xxx 字段解析，并更新 CSV 和 DB” | `msprof-dev` |
| 创建 PR | “把当前分支提交到 Ascend/msprof，创建 Draft PR” | `gitcode-pr` |
| 检视 PR | “检视 PR 123，重点看解析链路和输出契约” | `msprof-review` |
| 处理检视意见 | “分析 PR 123 的检视意见，可采纳的先修改” | `msprof-review-feedback` |
| 查看 CI | “查看 PR 123 当前 head 的 CI 状态” | `gitcode-ci` |
| 修复 CI | “修复 PR 123 的 CI，直到全部通过或明确阻塞” | `gitcode-ci` |

## 3. AI 能做什么

- 定位职责链和影响面，阅读实现、调用方、配置和测试。
- 设计方案、拆解任务、实现代码、补回归测试。
- 运行定向测试、构建、解析/导出验证和 AI 资产校验。
- 读取 PR、diff、评论和 CI，生成检视报告、回复和处理检视意见。
- 整理提交信息和 PR 正文，给出验证证据和残余风险。

安全自检：提交时 `gitleaks` hook 会扫描密钥；PR 模板包含安全自检项；需要依赖、注入或不安全 TLS 专项扫描时，使用团队安全扫描流程或对应安全审查工具。

AI 不能替代：

- 用户的业务判断和范围决策。
- 独立检视主体；作者自检不等同于审批。
- 未授权的提交、推送、评论、`resolve`、触发 CI、审批或合并。
- 缺少环境或数据时的真实验证。

## 4. 执行模式与授权

默认 `guided`：代理在需求结论、方案取舍、公共行为变化、首次推送、PR 创建和远端写入前确认。

用户可以明确授权 `autonomous`，但必须绑定当前仓库、分支、PR/Issue 和目标范围，并记录在 Issue/PR 评论中。以下硬性停止条件不因授权而取消：

- 范围或公共行为发生实质变化。
- 凭证、隐私或安全漏洞。
- 绕过测试或质量门禁。
- 破坏性操作、强制推送、审批或合并。
- 规范、代码和远端事实冲突。

授权分级和远端操作要求见 [AGENTS.md](../../../AGENTS.md) 第 4、5 节。

## 5. 验证要求

代理必须报告实际执行的检查，并区分：

- 通过：命令执行成功且结果符合预期。
- 未运行：缺少环境、数据或权限，说明原因。
- 跳过：明确不适用的检查及理由。
- 失败：错误信息、影响和是否阻塞交付。

“测试收集成功”“Mock 通过”“部分 UT 通过”均不得表述为全量验证通过。验证矩阵见 [AGENTS.md](../../../AGENTS.md) 第 8 节。

## 6. AI 资产维护

修改 `AGENTS.md`、`.agents/` 或本目录后：

```bash
python3 scripts/ai/validate_skills.py
git diff --check
```

该检查已配置为 pre-commit hook `ai-assets`，本地提交时会自动执行。CI 的 pre-commit 任务仅在 PR 含非文档改动时运行，纯 Markdown 变更不会触发；手动命令用于提交前预览或排查。

- 新增、删除或重命名 Skill 时同步更新 [.agents/README.md](../../../.agents/README.md) 索引和 `AGENTS.md` 意图路由。
- 共享的 PR 事实和行号规则放在 `.agents/references/`，不要复制到多个 Skill。
- 不易从代码直接看出的项目事实回写到 `AGENTS.md`、对应 Skill 或本目录，不要只留在会话中。
- 不提交 Token、私有采集数据、机器路径或临时产物。

## 7. 相关链接

- [仓库 AI 统一入口](../../../AGENTS.md)
- [Skill 索引与编写规范](../../../.agents/README.md)
- [贡献指南](../contributing/contributing_guide.md)
- [解析使用说明](../user_guide/msprof_parsing_instruct.md)
- [交付件字段参考](../user_guide/profile_data_file_references.md)
- [DB 结构参考](../user_guide/profile_data_file_references_db.md)
