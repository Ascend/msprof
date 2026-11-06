---
name: gitcode-auth
description: 检查 GitCode CLI 登录状态，处理 msprof 开发中的登录、退出登录、凭据来源、Git 协议和权限问题。用户要求诊断 GitCode 认证或权限故障时使用。
metadata:
  version: 1.0.0
  repository: Ascend/msprof
---

# GitCode 认证与权限检查

适用于 msprof 开发中的 GitCode CLI 认证问题。先检查状态，再根据具体故障处理；安装或调用本 Skill 不代表需要重新登录。

## 命令入口

统一使用 `gitcode`。Linux/macOS 可使用等价的 `gc`；Windows PowerShell 中 `gc` 是 `Get-Content` 的别名，应使用 `gitcode`。

```bash
# 确认版本和当前命令支持的参数
gitcode version
gitcode auth --help

# 检查登录状态，不输出 Token
gitcode auth status --json
```

不熟悉的子命令先查看 `--help`。命令参数和返回结构以当前安装版本为准。

## msprof 诊断流程

1. 查看不含 Token 的认证状态，核对账号、主机、凭据来源及 Git 协议。
2. 涉及仓库访问时检查 Git remote。正式目标为 `Ascend/msprof`；个人 Fork 从实际 remote 识别，不能假定 `origin` 是个人仓库。若 remote URL 含凭据，展示前脱敏。
3. 区分未登录、凭据失效、网络故障、仓库不存在和权限不足。CLI 的认证失败退出码为 `4`，JSON 中为 `logged_in: false`；结合当前版本的实际输出判断，不能把所有非零退出码都视为凭据失效。
4. CLI 登录成功不代表拥有 `Ascend/msprof` 写权限。读取公开 PR 成功也不证明可推送、回复或解决讨论。根据具体操作的错误定位权限，不得通过测试性写入验证权限。
5. Git clone/fetch/push 与 GitCode CLI 的 API 认证分别诊断。使用 SSH remote 时可运行 `ssh -T git@gitcode.com` 检查 SSH 身份，并结合输出判断；它不证明特定仓库可写。
6. 状态正常后继续原任务，不重复登录或切换账号。PR 操作显式指定 `-R Ascend/msprof` 或用户指定的仓库；`auth` 子命令使用主机参数，不使用 `-R`。

## 登录

只有任务要求登录或修复认证时才改变登录状态。需要用户参与时，让用户在私人终端或浏览器完成，不要求将 Token 粘贴到对话中。

```bash
# 用户在私人、未录制的终端中完成交互登录
gitcode auth login

# 浏览器登录；先核对当前版本是否支持
gitcode auth login --web

# 登录后重新检查状态
gitcode auth status --json
```

没有个人访问令牌（PAT）时，在 GitCode“头像 > 个人设置 > 访问令牌 > 新建访问令牌”创建，按任务授予最小权限；官方说明见 [访问令牌（PAT）](https://docs.gitcode.com/docs/help/home/user_center/security_management/user_pat/)。Token 只在私人终端或平台密钥管理流程中使用，不粘贴到对话、命令参数或提交文件。

CI 和临时会话优先由平台密钥管理功能注入 `GC_TOKEN` 或 `GITCODE_TOKEN`，通常无需运行登录命令。非交互式持久登录可使用 `--with-token` 从标准输入读取，但输入必须来自已授权的密钥管理流程；不要通过明文命令、文件或聊天传递凭据。

`--git-protocol https` 或 `--git-protocol ssh` 用于选择登录时的 Git 协议。按现有 remote 和任务需要选择，不因排查 CLI 认证而擅自改写 remote 或 SSH 配置。

## 凭据来源

- 默认优先级为 `GC_TOKEN`、`GITCODE_TOKEN`、已保存的登录信息。
- 环境变量会覆盖已保存的登录信息。发现来源不符合预期时只报告变量名称或来源，不读取或输出变量值。
- `--hostname gitcode.com` 可指定认证主机；涉及企业实例时使用用户指定的主机，并通过当前帮助确认行为。
- 不读取本地认证配置，不执行 `auth token` 或 `auth status --show-token`；这些命令会输出完整 Token，不适合仓库内的代理诊断流程。

## 退出登录

仅在用户要求退出登录，或已有授权明确包含清除登录状态时执行。权限不足本身不需要退出登录。

```bash
# 交互式退出登录
gitcode auth logout

# 已获授权的非交互式退出登录
gitcode auth logout --yes
```

退出登录只删除已保存的凭据，不能清除环境变量。不要据此判定 CI 注入的认证也已失效。

## 结果说明

报告账号是否已登录、失败的具体操作、权限或环境问题及下一步处理方式。区分 CLI 认证、Git 认证和目标仓库权限；不把其中一项通过写成全部可用。回复评论、推送和修改权限仍以任务授权为准，不能因认证成功而自动执行。
