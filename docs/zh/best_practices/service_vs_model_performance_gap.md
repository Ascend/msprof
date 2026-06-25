# 服务化与纯模型性能差异过大问题调优指导实例

## 场景说明

在大模型推理调优中，通常会先使用纯模型方式获取模型在固定输入输出长度、固定batch size下的性能基准，再将模型部署到服务化框架中承接真实请求。若服务化场景的吞吐、首Token时延或Decode时延与纯模型基准差异过大，常见原因包括服务化参数未对齐、资源分配不足、通信配置异常、负载不均或序列长度配置过大。

本文以服务化推理性能低于纯模型基准为例，介绍如何使用msServiceProfiler采集服务化链路数据，定位性能差异来源，并给出参数、资源、通信和负载均衡方向的调优方案。

## 问题现象

**典型表现**

- 纯模型测试中吞吐正常，例如输入/输出长度为256/256、bs=128时可达到1132.24 tokens/s，但服务化部署后吞吐明显下降。
- 服务化场景中Batch_Size_curve长期低于maxBatchSize或maxPrefillBatchSize配置上限。
- Prefill_Generate_Speed_Latency_curve、Decode_Generate_Speed_Latency_curve相较纯模型基准明显变差。
- First_Token_Latency_curve或Request_Latency_curve的P90/P99偏高，且随并发增加快速放大。
- 启动阶段出现HCCL通信超时、rank连接失败、模型加载卡死或服务进程被杀。
- MoE模型多卡部署时，部分Device耗时明显偏高，moe_analysis或专家热点图呈现负载不均。

**影响范围**

| 影响项 | 表现 |
| --- | --- |
| 吞吐 | 服务化tokens/s明显低于纯模型基准。 |
| 首Token时延 | Prefill阶段排队、组batch或执行耗时增加。 |
| Decode时延 | 通信等待、通算未融合或负载不均导致单步耗时升高。 |
| 稳定性 | 内存不足、权限不足或通信超时导致服务启动失败。 |
| 资源利用率 | batch未打满、KVCache配置过大或热点专家集中导致Device利用率下降。 |

## 数据采集

### 功能说明

使用msServiceProfiler采集服务化推理过程中的Request、BatchSchedule、ModelExecute、Communication、KVCache和专家负载相关数据。通过batch.csv、request.csv、forward.csv、BatchSchedule.csv以及可视化曲线，对比纯模型基准和服务化链路中的差异。

### 注意事项

- 调优前需先完成纯模型基准测试，明确在固定输入输出长度和固定batch size下的最大性能。
- 服务化压测应尽量复用纯模型基准的输入输出长度、并发规模和模型配置，避免基准不可比。
- 多机多卡场景采集前需确认各节点时间同步，避免Timeline和负载均衡图出现时间偏差。
- 若需要分析通信、任务下发或算子执行耗时，可开启acl任务耗时相关采集，但需评估额外开销。
- 专家热点信息建议单独配置eplb_observe domain域，避免采集数据过大。

### 配置示例

创建ms_service_profiler_config.json，按需采集Request、BatchSchedule、ModelExecute、Communication和KVCache域。

```json
{
    "enable": 1,
    "prof_dir": "${HOME}/.ms_server_profiler",
    "profiler_level": "INFO",
    "domain": "Request;BatchSchedule;ModelExecute;Communication;KVCache",
    "acl_task_time": 1,
    "acl_prof_task_time_level": "L0"
}
```

若需要采集MoE专家热点信息，可单独开启eplb_observe域，并配置MindIE专家热点采集环境变量。

```json
{
    "enable": 1,
    "prof_dir": "${HOME}/.ms_server_profiler",
    "profiler_level": "INFO",
    "domain": "eplb_observe"
}
```

启动服务前设置采集配置路径。

```bash
export SERVICE_PROF_CONFIG_PATH=/path/to/ms_service_profiler_config.json
```

## 基准对齐

服务化性能低于纯模型基准时，应先确认两类测试是否具备可比性。

| 检查项 | 说明 |
| --- | --- |
| 输入输出长度 | 纯模型与服务化压测的输入、输出长度应一致，例如256/256。 |
| batch与并发 | 纯模型bs应与服务化maxBatchSize、maxPrefillBatchSize、concurrency建立对应关系。 |
| 模型权重 | 模型版本、量化方式、并行策略和rank配置应一致。 |
| 精度与算子 | 纯模型和服务化使用的精度、算子库和图优化开关应一致。 |
| 硬件资源 | NPU数量、机器数量、CPU和内存资源应一致或可换算。 |

若基准未对齐，不应直接判断服务化框架存在瓶颈。建议先记录纯模型最大性能，再在服务化侧逐项打开调优开关，确认每一步收益。

## 定位方法

1. 查看Batch_Size_curve和batch.csv。
   - 若prefill_batch_size或decode_batch_size长期偏小，说明服务化组batch未打满，需检查concurrency、maxPrefillBatchSize、maxBatchSize、请求分布和supportSelectBatch。
   - 若batch size已接近上限但吞吐仍低，需继续检查ModelExecute、Communication和Device利用率。

2. 查看Request_Status_curve。
   - 若waiting队列持续堆积，说明服务入口、组batch或执行阶段消费能力不足。
   - 若running队列较少但吞吐低，说明调度策略可能过于保守，或服务化参数限制了有效batch。

3. 查看Prefill和Decode相关曲线。
   - Prefill_Generate_Speed_Latency偏高，优先检查maxPrefillBatchSize、maxSeqLen、KVCache预留和输入长度分布。
   - Decode_Generate_Speed_Latency偏高，优先检查通信配置、LCCL、通算融合、MoE负载均衡和跨卡同步等待。

4. 查看Kvcache_usage_percent_curve。
   - 若KVCache使用率长期较低但maxSeqLen配置很大，说明预留过多，可能限制batch和并发。
   - 若KVCache使用率接近上限，说明并发或序列长度已触及内存瓶颈，需降低maxSeqLen或增加资源。

## 原因分析与解决方案

### 参数配置与纯模型基准不一致

**原因**

服务化场景中的maxPrefillBatchSize、maxBatchSize、concurrency、prefillBatchSize等参数若未按实际负载配置，可能导致组batch不足、队列调度保守或Prefill/Decode资源分配不合理，最终表现为吞吐低于纯模型。

**解决方案**

- 以纯模型基准为目标，建立bs与服务化并发、maxBatchSize的对应关系。
- 根据请求长度分布调整maxPrefillBatchSize和prefillBatchSize，避免Prefill阶段batch过小。
- 根据压测并发逐步提高concurrency，观察Batch_Size_curve是否接近配置上限。
- 吞吐优先场景建议开启supportSelectBatch，使调度优先选择更有利于吞吐的batch组合。
- 分别记录调优前后的Batch_Size_curve、Prefill_Generate_Speed_Latency和Request_Latency，避免只看单次吞吐结果。

### 资源分配与内存不足

**原因**

服务化部署相比纯模型通常需要额外的框架进程、队列、KVCache、通信缓存和监控组件。若机器内存不足，可能出现模型加载卡死、进程被杀或首轮请求时延异常。

**解决方案**

- 启动前确认可用内存满足基本要求，建议free_mem不低于`(权重大小 / 机器数) * 1.3`。
- 在测试环境中可释放系统缓存后再启动服务。

```bash
sync
echo 3 > /proc/sys/vm/drop_caches
```

- 容器化部署时确认特权模式和路径权限，避免模型、rank table、共享内存或设备文件访问失败。

```bash
docker run --privileged=true ...
```

- 若KVCache使用率长期偏低且内存占用高，检查maxSeqLen是否远大于真实数据集最大序列长度。
- 若服务进程被杀，优先检查系统日志、容器内存限制和权重加载阶段内存峰值。

### 通信与环境变量配置不足

**原因**

多机多卡服务化部署依赖HCCL或LCCL通信。若主从节点环境变量不一致、rank table路径错误、容器IP设置错误或通信超时配置过小，可能导致启动失败、Decode等待增加或跨卡同步耗时偏高。

**解决方案**

在主节点和从节点分别设置本机IP、rank table和确定性通信相关环境变量。

```bash
export MIES_CONTAINER_IP=本机IP
export RANKTABLEFILE=/path/to/rank_table.json
export HCCL_DETERMINISTIC=true
```

大规模部署或网络初始化较慢时，可增大通信连接超时时间，并确认WORLD_SIZE与实际rank数量一致。

```bash
export HCCL_CONNECT_TIMEOUT=7200
export WORLD_SIZE=32
```

Decode时延偏高时，建议评估启用LCCL通信库和通算融合。

```bash
export ATB_LLM_LCOC_ENABLE=1
```

验证时重点对比Communication域Span、Decode_Generate_Speed_Latency和Request_Latency的P90/P99。

### MoE负载不均

**原因**

MoE模型中，不同专家的访问热度可能差异很大。如果热点专家集中部署在少数Device或rank上，会导致部分快卡等待慢卡，服务化吞吐低于纯模型理想基准。

**解决方案**

- 使用msServiceProfiler采集eplb_observe域，观察专家热点和负载不均曲线。
- 使用msit elb工具生成专家部署表expert_map_file。
- 在config.json中配置专家负载均衡参数，例如设置`"level": 1`启用静态负载均衡。
- 调整后重新采集moe_analysis.csv和专家负载不均折线图，确认不同Device耗时差距缩小。

### 序列长度配置过大

**原因**

maxSeqLen若按极端上限配置，例如设置为10000，但实际数据集最大长度只有698，会导致KVCache和内存预留过大，限制可用batch和并发，进而降低服务化吞吐。

**解决方案**

- 统计线上或压测数据集的真实输入输出长度分布，使用P99或实际最大值作为maxSeqLen配置依据。
- 将maxSeqLen从过大的保守值调整为贴近真实负载的值，例如从10000调整为698。
- 调整后观察Kvcache_usage_percent_curve、Batch_Size_curve和服务吞吐是否改善。
- 若业务存在少量超长请求，可对超长请求单独路由或降级处理，避免拖累主服务实例。

## 优化验证

每次只调整一个方向，并在相同压测条件下重新采集数据。建议按以下顺序验证。

| 步骤 | 验证目标 | 观察指标 |
| --- | --- | --- |
| 纯模型基准 | 确认模型理论上限 | tokens/s、bs、输入输出长度 |
| 参数调优 | 确认服务化batch是否打满 | Batch_Size_curve、batch.csv |
| 内存调优 | 确认资源是否足够 | Kvcache_usage_percent、进程RSS、系统free_mem |
| 通信调优 | 确认Decode等待是否下降 | Communication Span、Decode_Generate_Speed_Latency |
| 负载均衡 | 确认快慢卡是否收敛 | moe_analysis.csv、专家热点图 |
| 端到端验证 | 确认用户侧收益 | First_Token_Latency、Request_Latency、吞吐 |

优化有效时，通常会看到以下结果：

- Batch_Size_curve更接近maxBatchSize或maxPrefillBatchSize配置上限。
- Prefill和Decode阶段token平均时延下降。
- Request_Latency_curve的avg、P90和P99下降。
- KVCache使用率更贴近真实负载，内存浪费减少。
- MoE场景中不同Device耗时差距缩小，快慢卡现象减弱。
- 服务化吞吐逐步接近纯模型基准。

## 推荐处理策略

| 问题类型 | 推荐方案 |
| --- | --- |
| 服务化batch长期偏小 | 调整concurrency、maxBatchSize、maxPrefillBatchSize，开启supportSelectBatch。 |
| Prefill时延偏高 | 调整prefillBatchSize、maxSeqLen，检查KVCache预留和输入长度分布。 |
| Decode时延偏高 | 优化HCCL/LCCL配置，开启通算融合，检查跨卡通信等待。 |
| 启动失败或进程被杀 | 检查free_mem、容器特权模式、路径权限和内存限制。 |
| 多卡负载不均 | 使用msit elb生成expert_map_file，启用静态或动态专家负载均衡。 |
| 与纯模型差异仍大 | 回到基准对齐，确认模型版本、并行策略、输入输出长度和硬件资源一致。 |

## 总结

服务化与纯模型性能差异过大的核心在于基准对齐、参数配置、资源分配和通信优化。调优时应先用纯模型测试获取最大性能基准，再通过msServiceProfiler拆解服务化链路，重点观察BatchSchedule、Request、KVCache、Communication和MoE负载均衡数据。若业务追求吞吐，应优先打满batch并开启吞吐优先策略；若Decode尾时延偏高，应优先检查通信库、通算融合和专家负载均衡；若资源不足，应先解决内存、容器权限和序列长度配置问题，再继续做服务化参数优化。
