# msProf AI Native Development Guide

This guide helps developers use AI agents for requirements analysis, development, verification, PR review, and CI closure in msProf. The mandatory rules for AI agents are provided in [AGENTS.md](https://gitcode.com/Ascend/msprof/blob/master/AGENTS.md). For the Skill index and authoring guidelines, see [.agents/README.md](https://gitcode.com/Ascend/msprof/blob/master/.agents/README.md).

## 1. First Use

### 1.1 Prerequisites

| Item | Requirement |
| --- | --- |
| GitCode CLI | The current baseline is 0.15.0. Use `gitcode-cli-installer` if the CLI is unavailable. |
| Authentication | `gitcode auth status --json` can be read. Use `gitcode-auth` if you are not logged in or permissions are insufficient. |
| Python | 3.7+ |
| Git | `git worktree` must be supported. |
| Network | `gitcode.com` must be accessible. When using a network proxy, ensure that the CLI and Git use the same proxy configuration. |
| Client | Cursor, Codex, and OpenCode read `AGENTS.md`. Claude Code reads `CLAUDE.md` (which imports `AGENTS.md`). For other clients, refer to their official documentation. |

Create a GitCode personal access token (PAT) from "Profile > Personal Settings > Access Tokens > Create Access Token", and grant only the minimum permissions required for the task. Do not paste the token into conversations, command arguments, or committed files.

### 1.2 Startup Checks

Run the following commands in the repository root:

```bash
git status --short --branch
gitcode version
gitcode auth status --json
python3 scripts/ai/validate_skills.py
```

Expected result: the worktree status is clear, the CLI is authenticated, and the AI asset validation passes. If any check fails, resolve the corresponding issue first. Do not continue while a Skill is missing or authentication is unavailable.

### 1.3 First Task

1. Start the AI client in the repository root and confirm that it has read `AGENTS.md`. Claude Code reads `CLAUDE.md`, which imports `AGENTS.md`.
2. Describe the objective directly. You do not need to remember Skill names. The agent loads the corresponding Skill by intent routing.
3. Continue after confirmation at key points such as the solution, public behavior changes, the first push, and PR creation.
4. Before delivery, ask the agent to provide the commands actually executed, their results, and any uncovered items, rather than conclusions only.

> Command examples use bash. On Windows PowerShell, use equivalent commands and forward-slash paths. `python3` is usually written as `python` or `py -3` on Windows.

## 2. Common Scenarios

| Objective | Example Request | Skill Loaded by the Agent |
| --- | --- | --- |
| Implement a feature | "Implement feature xxx. Provide the solution before changing code." | `msprof-dev` |
| Fix a defect | "Fix the crash in xxx with level0 data." | `msprof-dev` |
| Change parsing or export | "Add parsing for field xxx and update both CSV and DB outputs." | `msprof-dev` |
| Create a PR | "Submit the current branch to Ascend/msprof and create a Draft PR." | `gitcode-pr` |
| Review a PR | "Review PR 123, focusing on the parsing pipeline and output contract." | `msprof-review` |
| Address review comments | "Analyze the review comments on PR 123 and apply the acceptable changes first." | `msprof-review-feedback` |
| Check CI | "Check the CI status of the current head of PR 123." | `gitcode-ci` |
| Fix CI | "Fix CI for PR 123 until all checks pass or a clear blocker is identified." | `gitcode-ci` |

## 3. What AI Can Do

- Locate responsibility chains and impact scope by reading implementations, callers, configurations, and tests.
- Design solutions, decompose tasks, implement code, and add regression tests.
- Run targeted tests, builds, parsing/export verification, and AI asset validation.
- Read PRs, diffs, comments, and CI results. Generate review reports, replies, and review-comment resolutions.
- Prepare commit messages and PR descriptions, and provide verification evidence and residual risks.

Security check: the `gitleaks` hook scans for secrets during commit. The PR template includes security self-check items. When specialized scanning for dependencies, injections, or insecure TLS is required, use the team security scanning process or the corresponding security review tool.

AI cannot replace:

- Your business judgment and scope decisions.
- An independent reviewer. Self-review by the author is not equivalent to approval.
- Unauthorized commits, pushes, comments, `resolve` operations, CI triggers, approvals, or merges.
- Real verification when the environment or data is unavailable.

## 4. Execution Modes and Authorization

The default mode is `guided`: the agent confirms before requirements conclusions, solution tradeoffs, public behavior changes, the first push, PR creation, and remote writes.

You can explicitly authorize `autonomous` mode, but the authorization must specify the current repository, branch, PR/Issue, and target scope, and must be recorded in an Issue or PR comment. The following mandatory stop conditions are not waived by authorization:

- A material change in scope or public behavior.
- Credentials, privacy, or security vulnerabilities.
- Bypassing tests or quality gates.
- Destructive operations, force pushes, approvals, or merges.
- Conflicts among specifications, code, and remote facts.

For authorization levels and remote operation requirements, see Sections 4 and 5 of [AGENTS.md](https://gitcode.com/Ascend/msprof/blob/master/AGENTS.md).

## 5. Verification Requirements

The agent must report the checks actually executed and distinguish among:

- Passed: the command completed successfully and the result met expectations.
- Not run: the environment, data, or permissions were unavailable. State the reason.
- Skipped: the check was explicitly inapplicable. State the reason.
- Failed: provide the error message, impact, and whether delivery is blocked.

"Test collection succeeded", "Mock tests passed", and "some unit tests passed" must not be described as full verification success. For the verification matrix, see Section 8 of [AGENTS.md](https://gitcode.com/Ascend/msprof/blob/master/AGENTS.md).

## 6. AI Asset Maintenance

After modifying `AGENTS.md`, `.agents/`, or this directory:

```bash
python3 scripts/ai/validate_skills.py
git diff --check
```

These checks are configured as the `ai-assets` pre-commit hook and run automatically during local commits. The CI pre-commit job runs only when a PR contains non-documentation changes. Markdown-only changes do not trigger it. Use the manual commands to preview checks before committing or to troubleshoot failures.

- When adding, deleting, or renaming a Skill, update the [.agents/README.md](https://gitcode.com/Ascend/msprof/blob/master/.agents/README.md) index and the `AGENTS.md` intent routing accordingly.
- Store shared PR facts and line-number rules under `.agents/references/`. Do not duplicate them across multiple Skills.
- Write project facts that are not readily visible from the code back to `AGENTS.md`, the corresponding Skill, or this directory. Do not leave them only in the conversation.
- Do not commit tokens, private collected data, machine paths, or temporary artifacts.

## 7. Related Links

- [Repository AI Entry Point](https://gitcode.com/Ascend/msprof/blob/master/AGENTS.md)
- [Skill Index and Authoring Guidelines](https://gitcode.com/Ascend/msprof/blob/master/.agents/README.md)
- [Contributing Guide](../contributing/contributing_guide.md)
- [Parsing Instructions](../user_guide/msprof_parsing_instruct.md)
- [Deliverable Field Reference](../user_guide/profile_data_file_references.md)
- [DB Structure Reference](../user_guide/profile_data_file_references_db.md)
