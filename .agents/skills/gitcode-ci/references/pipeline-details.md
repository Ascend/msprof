# 流水线接口与诊断细节

仅在定位日志接口、判断运行是否失效或处理 CodeCheck/SCA 时读取。

## 工作流程

1. 获取或复用 PR 评论，提取流水线总链接和任务表；结果必须与当前 PR/head 对齐。
2. 默认优先关注最新评论里的最新一轮流水线。
3. 若出现代码变更导致任务标签失效的通知，只分析该通知之后的新运行；没有新运行时报告未取得验证结果，不把旧运行当作当前结果。
4. 若评论区任务链接里没有 `jobRunId/stepRunId`，则优先用 pipeline detail 回填任务元数据。
5. 默认分析选中流水线里所有 `failed` 任务；没有 `failed` 时再看 `running` 任务。
6. 若需要日志，从任务详情链接中提取：
   `projectId`、`pipelineId`、`pipelineRunId`、`jobRunId`、`stepRunId`
7. 调用 openLiBing 接口：

```text
GET  https://www.openlibing.com/gateway/openlibing-cicd/project/pipeline/pipeline-run/detail
POST https://www.openlibing.com/gateway/openlibing-cicd/project/pipeline/exec-log
```

8. 按任务类型走不同接口：
   - `UT_*` / 构建：`/gateway/openlibing-cicd/project/pipeline/exec-log`
   - `CodeCheck`：`/gateway/openlibing-codecheck/ci-portal/v1/codecheck/event/task/issues/report`
     和 `/gateway/openlibing-codecheck/ci-portal/v1/event/codecheck/task`
   - `SCA`：`/gateway/openlibing-sca/open/person/scan/id`
     和 `/gateway/openlibing-sca/open/scan/scanIssue/query`
9. 对日志类任务分页获取日志，优先定位：
   - 第一条直接失败的测试
   - `short test summary info`
   - 与当前改动最相关的首个失败项
10. 输出前对日志和链接做基本脱敏，避免直接回显 Token、签名串、Cookie、口令类字段

## 输出要求

- 先总结任务状态：哪些阶段通过、哪些失败、哪些正在运行
- 再给失败任务日志摘要
- 若是 `CodeCheck`，输出规则名、文件、行号、问题说明
- 若是 `SCA`，输出命中文件、相似度、来源项目、风险说明
- 区分“直接相关失败”和“关联失败”
- 给出脱敏后的 openLiBing 任务链接，便于回看原始页面

## 注意事项

- `GET /project/pipeline/exec-log` 会返回“方法不支持”，必须用 `POST`
- `GET /project/pipeline/pipeline-run/detail` 可直接获取 `stages[].jobs[]` 结构
- openLiBing 前端页是 SPA，直接打开详情页无法获取日志正文，需要通过接口获取
- 当前 GitCode 机器人评论常见的是“任务名称/状态/日志/下载链接”表格，不再固定包含 stage 列
- 若评论区没有可推断的 `projectId`，任务详情回填可能不可用
- `compile` 评论是 openLiBing 流水线的触发条件，不是 push 后自动触发
- 若同一个 PR 有多轮 `compile`，优先分析最新评论里的最新流水线
- 标签只能辅助定位状态，须结合 run 和 head；本 Skill 只分析，不自动修复或触发。
- 若 PR 被关闭又重开，先确认当前有效 PR 编号再获取日志
