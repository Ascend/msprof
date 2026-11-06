# Claude Code Adapter

本文件是 Claude Code 的轻量适配器。Claude Code 默认优先读取 `CLAUDE.md`，因此通过 `@AGENTS.md` 导入共享规则，避免双份维护。

@AGENTS.md

## Skill 加载

共享 Skill 位于 `.agents/skills/`。Claude Code 默认从 `.claude/skills/` 发现 Skill，首次使用时创建符号链接，不复制内容：

```bash
mkdir -p .claude
ln -s ../.agents/skills .claude/skills
```

Windows 可在仓库根目录的 cmd 中使用目录联接；junction 记录的是绝对路径，移动或重命名工作副本后需重新创建：

```bat
mklink /J .claude\skills .agents\skills
```

需要相对目标时，可在支持开发者模式的 PowerShell 中使用：

```powershell
New-Item -ItemType SymbolicLink -Path .claude\skills -Target ..\.agents\skills
```

Cursor、Codex、OpenCode 默认读取 `AGENTS.md` 并自动发现 `.agents/skills/`，无需本适配器。规则冲突时以 `AGENTS.md` 为准。
