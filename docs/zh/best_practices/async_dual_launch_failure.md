# 异步双发未生效问题调优指导实例

## 场景说明

异步双发也称异步调度，目标是在模型推理服务化过程中，将CPU侧调度、Host侧任务下发、NPU侧模型执行以及通信过程尽量重叠，减少同步等待和调度空洞。该能力通常需要环境变量、流水优化、启动参数、硬件软件版本和通信配置共同满足条件。如果配置不完整或存在冲突，服务可能能够正常启动，但异步调度未实际触发，最终表现为吞吐提升不明显、Decode时延不降或CPU/NPU流水没有被掩盖。

本文以异步双发未生效为例，介绍如何使用msServiceProfiler采集BatchSchedule、ModelExecute和Communication数据，判断异步调度是否实际生效，并给出环境变量、流水优化、启动参数、兼容性和配置冲突方向的排查方案。

## 问题现象

**典型表现**

- 设置异步调度相关环境变量后，吞吐、首Token时延或Decode时延没有明显改善。
- Timeline中BatchSchedule、Host下发、ModelExecute仍呈现严格串行关系，未出现调度与执行重叠。
- CPU侧调度线程等待NPU执行完成后才继续提交下一轮任务，流水空洞明显。
- Decode_Generate_Speed_Latency_curve未下降，Request_Latency_curve的P90/P99仍偏高。
- 单机PD分离或多卡场景中，通信等待掩盖失败，Communication域耗时持续暴露在关键路径上。
- 服务启动日志未出现异步调度启用信息，或启动参数、环境变量未被框架识别。

**影响范围**

| 影响项 | 表现 |
| --- | --- |
| 吞吐 | CPU调度与NPU执行无法重叠，tokens/s提升不明显。 |
| Decode时延 | Decode阶段仍受同步下发和通信等待影响。 |
| 端到端时延 | Request Latency P90/P99难以下降。 |
| 资源利用率 | NPU执行间存在空洞，CPU/NPU流水不连续。 |
| PD分离性能 | KV Cache传输和通算融合未充分掩盖，跨阶段等待增加。 |

## 数据采集

### 功能说明

使用msServiceProfiler采集BatchSchedule、ModelExecute、Communication和Request域数据，通过Timeline、span_info、batch.csv、forward.csv、pd_split_communication.csv和可视化曲线，判断异步调度是否生效。

### 注意事项

- 采集前需记录服务启动命令、环境变量、MindIE版本、CANN版本、硬件型号和部署形态。
- 建议分别采集“未开启异步调度”和“开启异步调度”两组数据，使用相同压测流量做对比。
- 若需要进一步分析Host与Device之间的任务下发耗时，可开启acl任务耗时采集，但需评估额外开销。
- 多卡或PD分离场景需同时采集Communication域，否则无法判断通信等待是否被异步调度掩盖。

### 配置示例

创建ms_service_profiler_config.json，采集调度、执行、通信和请求数据。

```json
{
    "enable": 1,
    "prof_dir": "${HOME}/.ms_server_profiler",
    "profiler_level": "INFO",
    "domain": "Request;BatchSchedule;ModelExecute;Communication",
    "acl_task_time": 1,
    "acl_prof_task_time_level": "L0"
}
```

设置采集配置路径。

```bash
export SERVICE_PROF_CONFIG_PATH=/path/to/ms_service_profiler_config.json
```

解析时导出Span数据，便于观察BatchSchedule和forward是否重叠。

```bash
ms_service_profiler_parse --input-path ${PROF_DIR} --output-path ${OUTPUT_DIR} --span
```

## 生效判断

### 预期生效特征

异步双发生效后，通常可以观察到以下特征：

- Timeline中下一轮BatchSchedule或Host下发与上一轮ModelExecute存在时间重叠。
- CPU侧调度Span不再完全等待NPU侧forward结束后才开始下一轮。
- Decode阶段连续性增强，forward.csv中相邻Decode执行间隔缩短。
- Communication域耗时被部分掩盖，不再完整暴露在Request关键路径中。
- Decode_Generate_Speed_Latency_curve、Request_Latency_curve或吞吐指标有可观测改善。

### 未生效特征

若异步双发未生效，通常表现为：

- BatchSchedule.csv中的调度开始时间总是在上一轮forward结束之后。
- forward.csv中相邻执行之间存在明显空洞。
- Communication耗时与ModelExecute串行排列，未与计算阶段重叠。
- 开关打开前后Batch_Size_curve相似，但吞吐和Decode时延几乎无变化。
- 服务日志中未打印异步调度启用、流水队列启用或相关启动参数生效信息。

## 定位方法

1. 检查环境变量是否在服务启动前生效。
   - 确认启动脚本中已配置`MINDIE_ASYNC_SCHEDULING_ENABLE=1`。
   - 确认变量设置在服务进程启动之前，而不是启动后在交互终端中临时设置。
   - 容器部署时需确认环境变量传入容器内部，并可被服务进程读取。

2. 检查流水优化是否同时开启。
   - 异步调度通常需要配合`TASK_QUEUE_ENABLE=2`使用。
   - 若只开启异步调度，不开启流水优化，CPU/NPU重叠可能无法达到预期。
   - 通过Timeline观察Host下发和Device执行是否形成连续流水。

3. 检查启动参数是否显式启用。
   - 部分框架不只依赖环境变量，还要求启动命令包含异步调度参数。
   - 在vLLM-Ascend等场景中，需确认是否传入`--async-scheduling`。
   - 若框架启动脚本封装了参数，需要检查最终生效的真实启动命令。

4. 检查硬件和软件版本。
   - 确认硬件型号支持异步调度特性，例如目标昇腾卡型是否在支持范围内。
   - 确认CANN、MindIE和推理框架版本支持当前异步调度能力。
   - 对已知存在兼容性问题的旧版本，建议升级后重新验证。

## 原因分析与解决方案

### 环境变量未正确配置

**原因**

异步调度的核心开关未设置、设置值错误、设置时机晚于服务启动，或容器/启动脚本未继承该变量，都会导致异步双发未生效。

**解决方案**

在服务启动前配置环境变量。

```bash
export MINDIE_ASYNC_SCHEDULING_ENABLE=1
```

容器部署时，将变量写入容器启动命令或服务启动脚本中，并在服务日志中确认变量已被读取。若使用systemd、Kubernetes或平台化拉起方式，需要检查最终进入服务进程的环境变量，而不是只检查当前Shell。

### 未开启流水优化

**原因**

异步调度需要与流水队列能力配合，才能将任务下发、调度和执行重叠。若`TASK_QUEUE_ENABLE`未设置为推荐值，可能仍呈现同步串行提交。

**解决方案**

在启动前开启流水优化。

```bash
export TASK_QUEUE_ENABLE=2
```

开启后重新采集Timeline。若BatchSchedule和ModelExecute仍完全串行，继续检查启动参数、版本和冲突配置。

### 启动参数未启用

**原因**

部分框架需要在启动命令中显式打开异步调度。只配置环境变量时，框架层可能不会进入异步调度分支。

**解决方案**

检查服务启动命令。以需要显式参数的框架为例，启动时增加异步调度参数。

```bash
--async-scheduling
```

如果使用脚本封装启动命令，需确认该参数没有被配置模板覆盖或过滤。建议在日志中打印最终启动参数，便于复盘。

### 硬件或软件版本不兼容

**原因**

异步调度能力依赖硬件、CANN、MindIE和框架版本。旧版本可能不支持该能力，或存在异步调度与通信、内存管理相关的兼容性问题。

**解决方案**

- 确认硬件型号支持异步调度，例如目标环境是否为支持该特性的昇腾卡型。
- 确认CANN版本、MindIE版本和推理框架版本满足异步调度要求。
- 对已知存在兼容性问题的旧版本，升级到支持异步调度的稳定版本后重新验证。
- 升级前后分别采集msServiceProfiler数据，使用相同压测条件对比Decode时延和Timeline重叠情况。

### 通信或内存配置冲突

**原因**

部分通信或内存相关环境变量会改变执行图、通信路径或任务提交方式，可能导致异步调度未触发，或异步收益被通信同步等待抵消。

**解决方案**

排查以下配置是否与当前异步调度场景冲突。

```bash
unset HCCL_OP_EXPANSION_MODE
unset ATB_LLM_HCCL_ENABLE
```

单机PD分离场景中，建议开启通算融合。

```bash
export ATB_LLM_LCOC_ENABLE=1
```

多卡场景需确认HCCL/LCCL配置正确，避免通信初始化、rank配置或网络问题导致异步调度收益无法体现。

### Prefix Cache、LCCL等协同特性缺失

**原因**

异步调度只能减少调度和执行之间的等待，若Prefill重复计算、KV Cache传输慢或通信库未优化，整体性能仍可能无明显改善。

**解决方案**

- 单机PD分离场景中，结合Prefix Cache减少重复Prefill计算。
- 启用LCCL通信库，降低KV Cache传输和Decode通信开销。
- 对Communication域进行采集，确认通信耗时是否被计算阶段掩盖。
- 若通信仍暴露在关键路径中，优先优化通信配置，再评估异步调度收益。

## 插桩建议

若框架允许自定义插桩，可在异步调度关键阶段增加Span和Event，便于确认异步任务提交和回收是否发生。

```C++
auto submitSpan = PROF(INFO, SpanStart("AsyncSubmit"));

// CPU侧异步提交下一轮任务

PROF(submitSpan.SpanEnd());

PROF(INFO, Event("AsyncTaskQueued"));

auto waitSpan = PROF(INFO, SpanStart("AsyncWait"));

// 等待异步任务完成或回收结果

PROF(waitSpan.SpanEnd());
```

采集队列深度和异步命中次数。

```C++
PROF(INFO, Metric("asyncQueueDepth", asyncQueueDepth).MetricScope("scheduler", rankId).Launch());
PROF(INFO, MetricInc("asyncDispatchCount", 1).MetricScope("rank", rankId).Launch());
```

若`asyncDispatchCount`长时间为0，说明异步分支未进入；若`asyncQueueDepth`持续为0，说明任务没有形成有效流水。

## 优化验证

建议按以下顺序验证，每次只改变一个变量。

| 步骤 | 验证内容 | 观察指标 |
| --- | --- | --- |
| 基线采集 | 关闭异步调度，采集同步执行数据 | Timeline、BatchSchedule.csv、forward.csv |
| 开启异步变量 | 配置MINDIE_ASYNC_SCHEDULING_ENABLE=1 | 是否进入异步分支、Decode时延 |
| 开启流水优化 | 配置TASK_QUEUE_ENABLE=2 | BatchSchedule与ModelExecute是否重叠 |
| 增加启动参数 | 配置--async-scheduling | 框架日志、Timeline重叠 |
| 排除冲突配置 | 清理冲突通信或内存变量 | Communication耗时、Request Latency |
| 协同优化 | 开启LCCL、Prefix Cache或通算融合 | Decode时延、吞吐、PD通信耗时 |

优化有效时，通常会看到以下结果：

- BatchSchedule和ModelExecute在Timeline上出现重叠。
- forward.csv中相邻Decode执行间隔缩短。
- Communication域耗时被部分掩盖。
- Decode_Generate_Speed_Latency下降。
- Request_Latency的P90/P99下降。
- 吞吐提升，NPU执行空洞减少。

## 推荐处理策略

| 问题类型 | 推荐方案 |
| --- | --- |
| 异步开关未生效 | 启动前设置MINDIE_ASYNC_SCHEDULING_ENABLE=1，并确认服务进程可读取。 |
| 开关开启但无收益 | 同时设置TASK_QUEUE_ENABLE=2，检查Timeline是否出现流水重叠。 |
| 框架未进入异步分支 | 在启动命令中显式配置--async-scheduling，并检查最终启动参数。 |
| 旧版本兼容问题 | 升级CANN、MindIE或推理框架到支持异步调度的稳定版本。 |
| 通信配置抵消收益 | 检查HCCL/LCCL配置，清理冲突变量，必要时开启通算融合。 |
| 单机PD分离收益不足 | 配合ATB_LLM_LCOC_ENABLE=1、Prefix Cache和LCCL通信库。 |
| 多卡场景仍串行 | 检查rank配置、网络通信、Communication域耗时和跨卡同步点。 |

## 总结

异步双发未生效通常不是单一开关问题，而是环境变量、流水优化、框架启动参数、硬件软件版本和通信配置共同决定。定位时应先通过msServiceProfiler采集BatchSchedule、ModelExecute和Communication数据，判断调度、执行、通信是否在Timeline上形成重叠。优化时优先确认`MINDIE_ASYNC_SCHEDULING_ENABLE=1`、`TASK_QUEUE_ENABLE=2`和框架启动参数是否生效，再排查版本兼容、冲突环境变量、HCCL/LCCL通信配置、PD分离通算融合和Prefix Cache等协同能力。
