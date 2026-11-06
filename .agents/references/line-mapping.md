# 行内评论定位

发布行内评论或定位已有评论时读取。共享给 `msprof-review` 和 `msprof-review-feedback`。

当前 CLI 的 `pr comment --position` 是 PR **新版本文件的一基行号**，不是 unified diff 的偏移，也不是任意 checkout 的行号。使用前核对 `gitcode pr comment --help`。远端评论字段可能缺失或来自旧 head，必须结合原始 diff、评论内容和对应版本核实。

对 `@@ -old_start,old_count +new_start,new_count @@` 分别维护旧、新行号：上下文推进两者，新增行只推进新行号，删除行只推进旧行号。每个 hunk 重新初始化；文件头和“文件末尾无换行”标记不计数。删除行没有新版本锚点，不发布行内评论，记录到本地报告和检视记录表（ledger）。

先固定 head 与文件路径，再确认路径在 `pulls/<PR>/files` 中，并计算行号、读取同一 head 的内容二次核对。不得通过偏移反复试探或估计行号。已有评论按问题实质去重，避免重复发布同一问题。无法可靠映射时明确说明并保留在本地报告，不改为发布总体评论。

```bash
gitcode pr comment <PR> -R <repo> --path <file> --position <new-file-line> \
  --body-file <finding.md> --json
```
