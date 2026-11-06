---
name: gitcode-cli-installer
description: 安装或升级 GitCode CLI，诊断命令缺失和版本能力不足。认证与权限故障使用 gitcode-auth；业务 Skill 仅在工具确实缺失或不兼容时依赖本 Skill。
metadata:
  version: 1.0.0
  repository: Ascend/msprof
---

# GitCode CLI 安装与升级

1. 检查 `gitcode version` 与所需子命令的 `--help`。已经可用则返回原任务，不重复安装。Windows 使用 `gitcode` 全称，避免 PowerShell 的 `gc` 别名冲突。
2. 需要 npm 安装时先检查 Node.js 18 及以上和 npm；按当前安装方式升级，避免覆盖同名的其他工具。工具版本以当前任务实际需要的命令能力为准，不因本 Skill 旧版本记录的固定版本号强制升级。
3. 使用 npm 安装：`npm install -g @gitcode-cli/cli`。默认沿用本机 npm registry；公共 registry 或公司镜像按网络环境选择，不为一次安装持久改写用户配置。安装后重新检查版本和所需命令。
4. 原任务需要远端访问时检查认证状态；登录或权限处理只使用 [认证 Skill](../gitcode-auth/SKILL.md)，沿用已有授权，不在安装流程中重复维护认证规则。

安装完成条件是命令及所需能力可用。登录和写权限不是安装完成的必要条件；按原任务需要分别核验。报告实际安装方式、版本、检查结果与尚缺环境。
