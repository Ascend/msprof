---
name: gitcode-pr
description: 为 msprof 创建或更新 GitCode PR，准备正文、选择 Fork 与目标分支、执行写入并回读结果。适用于创建 PR、更新描述、转 Ready 和提交前检查；PR 检视与意见处理使用 msprof-review 和 msprof-review-feedback。
metadata:
  version: 1.0.0
  repository: Ascend/msprof
---

# GitCode PR 创建与更新

只负责 PR 的创建、更新和准备。代码检视和检视意见处理分别使用 [msprof-review](../msprof-review/SKILL.md) 与 [msprof-review-feedback](../msprof-review-feedback/SKILL.md)。

## 适用场景

- “帮我创建 PR”“提交到 Ascend/msprof”
- “更新 PR 123 的描述/标题”
- “把 PR 转 Ready”“检查能不能提 PR”
- 开发完成后需要把本地分支交付到远端

## 前置检查

```bash
git status --short --branch
git remote -v
git log --oneline --decorate -5
git diff --stat <base-ref>...HEAD
```

- CLI 版本、认证状态和 PR 事实按 [pr-context.md](../../references/pr-context.md) 获取，不重复检查。
- 确认当前分支不是 `master`，且工作区没有未提交的非预期改动。
- 确认 source remote、目标仓库、base 分支和 head 分支。目标默认 `Ascend/msprof` + `master`；用户指定时以用户为准。Fork 源仓库从实际 remote 和 PR 元数据识别，不能假定 `origin` 是个人 Fork。
- 提交或创建 PR 前先运行 `gitcode precommit check --yes` 安装并初始化 hooks（检测到未安装时自动安装），再用 `pre-commit run --from-ref <base-ref> --to-ref HEAD` 只检查本次改动；需要全仓库检查时使用 `gitcode precommit check --run --yes`。只诊断时使用 `--no-install --json`。
- 若验证后工作区又有改动，或验证不在本次会话完成，提交前复验受影响的最小门禁；否则直接引用已有命令和结果，不重复全量验证。

## 准备 PR 正文

按 [PR 模板](../../../.gitcode/PULL_REQUEST_TEMPLATE.zh-CN.md) 逐项填写，不涉及的项标注“不涉及”，并补充模板未覆盖的残余风险。正文写入文件后使用 `--body-file`，避免多行转义；不要把 Token、私有数据、机器路径或未经授权的采集数据写入 PR。

## 创建 PR

```bash
# 标准 PR
gitcode pr create -R Ascend/msprof --base master --head feature-branch \
  --title "feat: summary" --body-file pr.md --json

# 草稿 PR
gitcode pr create -R Ascend/msprof --title "fix: summary" --body-file pr.md --draft --json

# Fork PR
gitcode pr create -R Ascend/msprof --fork <fork-owner>/<fork-repo> --head feature-branch \
  --base master --title "feat: summary" --body-file pr.md --json
```

创建前展示仓库、base、head、标题和正文摘要。创建后读取返回的 PR 编号/URL，并回读正文确认写入成功。

## 更新已有 PR

```bash
gitcode pr edit 123 -R Ascend/msprof --body-file pr.md --json
gitcode pr edit 123 -R Ascend/msprof --title "new title" --json
gitcode pr ready 123 -R Ascend/msprof --ready --yes --json
```

- 更新前确认 PR 当前 head 与本地分支一致；不一致时先说明，不直接覆盖。
- 只有 CI 和作者自检满足仓库要求后才转 Ready。
- 更新后回读正文、标题和状态。

## 交接

检视、反馈、CI、提交前 hooks 分别使用 [msprof-review](../msprof-review/SKILL.md)、[msprof-review-feedback](../msprof-review-feedback/SKILL.md)、[gitcode-ci](../gitcode-ci/SKILL.md)、[gitcode-precommit](../gitcode-precommit/SKILL.md)；PR 事实获取见 [pr-context.md](../../references/pr-context.md)。

## 授权与安全

- 创建、编辑和转 Ready 属于远端写入，按 [AGENTS.md](../../../AGENTS.md) 第 4 节确认授权；只准备正文时先报告待执行内容。
- 不虚构 CI、检视、测试或分支同步状态。

## 完成标准

- PR 已创建或更新，并回读成功。
- 正文按 PR 模板填写，并补充残余风险。
- 目标仓库、base、head 和 source remote 与事实一致。
- 后续动作（检视、CI、反馈）已明确交接给对应 Skill，未授权动作保持待执行。
