# msprof AI Native 资产

本目录存放 `Ascend/msprof` 的 Agent Skills 和共享契约，供支持 `AGENTS.md` 与 Agent Skills 约定的 AI 编程客户端使用。仓库级规则和意图路由由 [AGENTS.md](../AGENTS.md) 负责；本文件只说明资产结构、Skill 索引和编写规范。

> AI 代理必读：先读 [AGENTS.md](../AGENTS.md)，再按任务加载对应 Skill。

## 客户端加载

| 客户端 | 项目规则 | Skills |
| --- | --- | --- |
| Cursor | 读取根目录和子目录的 `AGENTS.md` | 发现 `.agents/skills/` |
| Codex | 读取 `AGENTS.md` | 发现 `.agents/skills/` |
| OpenCode | 读取 `AGENTS.md` | 发现 `.agents/skills/` |
| Claude Code | 默认读取 `CLAUDE.md`；本仓库通过 `@AGENTS.md` 导入 | 需将 `.claude/skills` 指向 `.agents/skills/` |
| 其他客户端 | 以客户端文档为准 | 以客户端文档为准 |

参考：

- [Cursor Rules](https://cursor.com/docs/context/rules)、[Cursor Agent Skills](https://cursor.com/docs/skills)
- [Codex AGENTS.md](https://developers.openai.com/codex/guides/agents-md)、[Codex Skills](https://developers.openai.com/codex/skills)
- [OpenCode Instructions](https://opencode.ai/v2/docs/instructions/)、[OpenCode Skills](https://opencode.ai/v2/docs/skills/)
- [Claude Code Memory](https://code.claude.com/docs/en/memory#agents-md)

当前基线：GitCode CLI 0.15.0、Python 3.7+。其他版本先核对 `--help`；客户端行为以其文档为准。命令示例以 Linux bash 为准，Windows PowerShell 使用等价命令和正斜杠路径。

## 1. 目录结构

```text
.agents/
├── README.md
├── references/
│   ├── pr-context.md          # PR 事实与跨 Skill 交接契约
│   └── line-mapping.md        # 行内评论行号映射
└── skills/
    ├── msprof-dev/
    ├── msprof-review/
    ├── msprof-review-feedback/
    ├── gitcode-pr/
    ├── gitcode-ci/
    ├── gitcode-precommit/
    ├── gitcode-auth/
    └── gitcode-cli-installer/
```

命名约定：`msprof-*` 表示仓库工作流 Skill；`gitcode-*` 表示可复用的 GitCode CLI 操作能力。一个 Skill 只承担一个连贯目标，重叠流程优先合并，不复制同一事实。

## 2. Skill 索引

### 2.1 开发

| Skill | 触发场景 | 主要产出 |
| --- | --- | --- |
| [`msprof-dev`](skills/msprof-dev/SKILL.md) | “实现 xxx”“修复 xxx”“重构”“补测试”“按方案完成修改并验证” | 代码变更、回归测试、验证证据、提交/PR 交接 |

### 2.2 PR、CI 与检视

| Skill | 触发场景 | 主要产出 |
| --- | --- | --- |
| [`gitcode-pr`](skills/gitcode-pr/SKILL.md) | “创建 PR”“更新 PR 描述”“转 Ready”“准备提 PR” | 创建/更新的 PR 及回读结果 |
| [`msprof-review`](skills/msprof-review/SKILL.md) | “检视 PR 123”“交互检视”“快速检视”“查看待检视” | 本地检视报告、逐条行内评论 |
| [`msprof-review-feedback`](skills/msprof-review-feedback/SKILL.md) | “处理检视意见”“判断评论是否合理”“按意见修改” | 每条意见的结论、修复、回复和 `resolve` 状态 |
| [`gitcode-ci`](skills/gitcode-ci/SKILL.md) | “查看 CI”“分析失败日志”“修复流水线”“持续监控直至全部通过” | 当前 head 的 CI 结论或修复闭环 |

### 2.3 基础能力

| Skill | 触发场景 | 主要产出 |
| --- | --- | --- |
| [`gitcode-precommit`](skills/gitcode-precommit/SKILL.md) | “检查 pre-commit”“提交前检查 hooks” | hooks 就绪；缺失时自动安装并初始化 |
| [`gitcode-auth`](skills/gitcode-auth/SKILL.md) | “`gitcode` 登录失败”“没有权限”“认证状态” | 认证/权限问题定位，不输出 Token |
| [`gitcode-cli-installer`](skills/gitcode-cli-installer/SKILL.md) | “安装 `gitcode`”“升级 GitCode CLI” | 所需命令和能力可用 |

## 3. 共享契约

| 文档 | 使用方 | 作用 |
| --- | --- | --- |
| [`references/pr-context.md`](references/pr-context.md) | `gitcode-pr`、`msprof-review`、`msprof-review-feedback`、`gitcode-ci` | 固定 PR/head、获取权威 diff 与评论、保护工作区、跨 Skill 交接 |
| [`references/line-mapping.md`](references/line-mapping.md) | `msprof-review`、`msprof-review-feedback` | 将 diff 行号映射到新版本文件行号，避免行内评论错位 |

共享契约只维护一份。新增 PR 相关 Skill 时引用这两个文件，不复制规则。

## 4. Skill 编写规范

### 4.1 目录与文件

```text
<skill-name>/
├── SKILL.md       # 必需：定位、触发场景、流程、授权边界、完成标准
├── references/    # 可选：较长的背景、规则和查表
├── scripts/       # 可选：可重复执行的确定性逻辑
└── assets/        # 可选：模板、样例和静态资源
```

- 目录和 `name` 使用小写 kebab-case，且必须一致。
- `SKILL.md` 只放入口和必要流程；长材料放 `references/`，确定性逻辑放 `scripts/`。
- 示例命令必须能在本仓库执行，或明确标注环境前提。
- 使用仓库相对路径；不写机器绝对路径、Token、私有数据或不可复现的临时文件。

### 4.2 Frontmatter

```yaml
---
name: skill-name
description: 做什么，以及何时使用；包含用户可能说出的触发词。
metadata:
  version: 1.0.0
  repository: Ascend/msprof
---
```

- `description` 同时写清“做什么”和“何时使用”，不写内部实现细节。
- 首次合入前所有 Skill 统一使用 `1.0.0`，不因修改递增；合入后修改行为、流程或边界时再递增 `metadata.version`。

### 4.3 内容要求

每个 Skill 至少包含：

1. **适用场景**：自然语言触发词，以及不适用时应该转到哪个 Skill。
2. **前置条件**：需要的仓库状态、CLI、数据、环境和授权。
3. **执行流程**：可复现的步骤，区分只读检查和远端写入。
4. **授权与安全**：哪些操作自主执行，哪些必须获得明确授权。
5. **验证与完成标准**：运行什么、如何判断完成、未覆盖项如何报告。

### 4.4 维护规则

- 新增、删除或重命名 Skill 时同步更新本文件索引和 `AGENTS.md` 的意图路由。
- 删除 Skill 前先搜索仓库内引用；共享契约移到 `references/`，不要让其他 Skill 指向不存在的路径。
- 已配置 pre-commit hook `ai-assets`（`always_run`），本地提交时会自动校验。CI 的 pre-commit 任务仅在 PR 含非文档改动时运行，纯 Markdown 变更不会触发；修改后也可手动运行：

  ```bash
  python3 scripts/ai/validate_skills.py
  ```

- 校验只覆盖 frontmatter、命名、索引和链接；流程、命令和权限仍需人工核对。
- 不易从代码直接看出的项目事实回写到对应 Skill；不要在多个 Skill 中重复维护同一规则。
- PR 事实、工作区隔离、回读和行号映射规则只在 `references/` 维护，授权边界以 `AGENTS.md` 第 4 节为准；Skill 只写本域增量，不复制命令块。
- 流程规则写成可观察的门禁和停止条件（覆盖项闭合、证据充分、无新证据），不用命令条数或时间上限替代必要检查。
