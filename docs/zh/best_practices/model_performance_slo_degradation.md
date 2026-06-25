# 模型性能劣化导致SLO劣化

## 问题背景

在服务化推理场景中，SLO（Service Level Objective，服务等级目标）是衡量服务质量的核心指标，通常包括首Token时延（TTFT）、Token生成速度（TPS）、请求端到端时延（E2E Latency）、吞吐量（Throughput）等。当模型执行侧出现性能劣化时，会直接导致SLO指标恶化，影响用户体验和业务SLA。模型性能劣化可能由多种因素引起：算子执行效率下降、模型计算图编译优化失效、NPU显存不足导致Swap、模型版本升级引入性能回退、CANN版本变更导致算子性能变化、动态shape场景下编译缓存未命中、MoE模型专家负载不均等。昇腾NPU上，模型从GPU迁移后可能因算子适配差异、图编译策略不同、内存管理机制差异等原因出现性能劣化，需要通过系统化的定位手段逐层排查。

## 问题来源

推理

## 问题现象

用户通常先看到推理服务SLO指标全面恶化：TTFT升高、TPS下降、P95/P99端到端时延升高、整体吞吐下降。在Grafana中按指标拆开后，常见现象是：

- `first_token_latency`指标P50/P90/P99均升高。
- `generate_token_speed`（Token生成速度）下降。
- `request_latency`（请求端到端时延）升高。
- `batch_size`可能正常但`during_time(ms)`（模型执行耗时）增加。
- NPU利用率可能正常甚至偏高（说明NPU在忙于执行低效算子）。

在`batch.csv`中，`name`为`modelExec`的行的`during_time(ms)`显著增加，而`batch_size`和`total_scheduled_tokens`与劣化前相近，说明相同计算量下模型执行耗时变长。在`forward.csv`中，`during_time(ms)`（forward执行时间）增加，`bubble_time(ms)`（空泡时间）可能正常，说明瓶颈在模型前向计算本身而非调度。在`request.csv`中，`execution_time(ms)`和`first_token_latency(ms)`均升高，而`queue_wait_time(ms)`可能正常，说明请求在队列中的等待时间未增加，瓶颈在执行阶段。

典型场景包括：模型版本升级后性能回退、CANN版本升级后算子性能变化、模型从GPU迁移到昇腾NPU后性能不达预期、MoE模型专家负载不均导致部分卡成为短板、动态shape场景下编译缓存未命中导致重复编译。

## 定位过程

### 步骤 1：先确认SLO劣化范围和劣化幅度

在Grafana中查看`first_token_latency`、`generate_token_speed`、`request_latency`等SLO指标的时序变化，确认劣化开始时间点和劣化幅度。对比劣化前后的`batch_size`、`total_scheduled_tokens`、NPU利用率等指标，判断劣化是否与负载变化相关。如果负载未变但SLO恶化，可初步判断为模型执行侧性能劣化。

### 步骤 2：用Profiler采集模型执行阶段数据

配置`ms_service_profiler_config.json`，设置`domain`为`"Schedule; ModelExecute; Request; KVCache"`，开启数据采集。如果怀疑算子级别性能问题，可设置`acl_task_time`为1或2，开启算子耗时采集（注意：开启算子采集会引入额外性能开销，建议在模型执行耗时异常时开启）。采集完成后执行解析：

```bash
python3 -m ms_service_profiler.parse --input-path ${PATH}/prof_dir/
```

解析生成`batch.csv`、`forward.csv`、`request.csv`、`chrome_tracing.json`等文件。

### 步骤 3：分析batch.csv和forward.csv中的模型执行耗时

在`batch.csv`中筛选`name`为`modelExec`的行，按时间序列观察`during_time(ms)`的变化趋势。如果模型执行耗时在某个时间点后突然增加，可能是配置变更或环境变化导致。

按`dp_rank`分组统计各DP域的模型执行耗时。如果特定DP域的执行耗时明显高于其他DP域，可能是该DP域对应的设备或进程存在异常。

按`batch_type`（prefill/decode）分别统计。如果Prefill执行耗时增加，可能是输入处理或注意力计算变慢；如果Decode执行耗时增加，可能是逐Token生成效率下降。

在`forward.csv`中按`dp_rank`和`forward_iter`观察`during_time(ms)`和`bubble_time(ms)`。如果`during_time(ms)`增加而`bubble_time(ms)`正常，说明瓶颈在模型前向计算；如果`bubble_time(ms)`也增加，可能是调度或通信问题。

### 步骤 4：用服务化拆解工具细粒度拆解模型执行阶段

```bash
msserviceprofiler split --input-path /path/to/input --prefill-batch-size 1 --prefill-number 50
msserviceprofiler split --input-path /path/to/input --decode-batch-size 1 --decode-number 50
```

查看`prefill.csv`和`decode.csv`中各子阶段的耗时分布，定位模型执行的具体瓶颈子环节（如数据下发、算子执行、数据接收等）。

### 步骤 5：通过Timeline视图定位瓶颈算子

打开`chrome_tracing.json`，在Timeline视图中放大模型执行阶段，观察各算子的执行耗时。如果开启了`acl_task_time`采集，可在Timeline中看到算子级别的执行时序，定位耗时最长的算子。

对比劣化前后的Timeline（如果有历史数据），观察哪些算子的执行耗时发生了变化。使用服务化性能数据比对工具：

```bash
msserviceprofiler compare ./profiling_data/before ./profiling_data/after
```

对比两个采集时间点的性能数据差异。

### 步骤 6：用多维度解析工具获取整体统计

```bash
msserviceprofiler analyze --input-path=/path/to/input
```

查看`batch_summary.csv`中prefill和decode执行时间的P50/P90/P99分位数，判断是否存在长尾。查看`request_summary.csv`中`first_token_latency(ms)`和`exec_time(ms)`的分布。查看`service_summary.csv`中`generate_token_speed`和`generate_all_token_speed`，确认整体吞吐是否下降。

### 步骤 7：如果怀疑算子级别问题，用msprof工具进行算子级性能分析

在采集时配置`acl_task_time`参数值为3，确保采集的性能数据文件目录中包含以`_ascend_pt`为后缀的算子数据文件。解析完成后使用msprof导出算子数据：

```bash
msprof --export=on --output=/path/to/output
```

使用MindStudio Insight打开解析后的性能数据，在"算子耗时"面板中查看TOP耗时算子，定位性能瓶颈算子。

### 步骤 8：如果怀疑MoE模型专家负载不均，采集eplb_observe domain数据

配置`domain`为`"eplb_observe"`，采集专家热点信息。解析后查看专家热点热力图和负载不均折线图，判断是否存在专家负载不均导致的性能劣化。

## 问题根因

模型性能劣化导致SLO劣化的常见根因包括：

1. **算子性能回退**：CANN版本升级后，某些算子的实现发生变化导致性能下降；模型版本升级后计算图结构变化引入低效算子；算子融合策略变化导致融合失效。

2. **模型从GPU迁移到昇腾NPU后适配不足**：模型中存在大量Pad、Strided_Slice等算子在昇腾上实现效率较低（涉及数组重排）；部分算子在昇腾上不支持导致模型切分为多个子图，子图间数据传输增加耗时；模型未真正调用昇腾后端而自动切换到CPU执行。

3. **动态shape场景下编译缓存未命中**：每次推理的输入shape不同导致图编译缓存失效，触发重复编译，首次推理或shape变化时耗时显著增加。

4. **NPU显存不足导致Swap**：KVCache占用过高或模型权重占用过大导致NPU显存不足，触发request Swap（将请求的KV Cache换出到CPU内存），Swap和恢复操作引入大量延迟。

5. **MoE模型专家负载不均**：DeepSeek等MoE模型中，不同专家被调用的频率差异大，部分专家所在设备成为计算热点，其他设备空闲等待，形成快慢卡现象。

6. **Host Bound导致Device空闲**：小模型或Decode阶段，Host下发Task的速度跟不上Device执行速度，Device出现间歇性空闲。可通过CANN图模式调度或模型下沉解决。

7. **通信瓶颈**：多卡推理场景中，AllReduce/AllGather等集合通信耗时占比过高；PD分离场景中KV Cache传输延迟大；EP（专家并行）场景中Dispatch-Combine通信成为瓶颈。

8. **模型量化或精度设置不当**：量化模型推理时精度损失导致需要更多计算步骤；或量化配置不当导致部分算子回退到高精度计算。

## 解决方法

- 算子性能回退：使用AOE自动调优优化模型计算图，或回退CANN版本。
- GPU迁移适配不足：使用昇腾亲和算子替换低效算子，确认模型已调用昇腾后端。
- 动态shape编译缓存未命中：固定输入shape或启用编译缓存。
- NPU显存不足Swap：调整KVCache配置避免Swap，降低单实例并发或增加显存。
- MoE专家负载不均：启用专家负载均衡（EPLB），调整专家放置策略。
- Host Bound：启用CANN图模式调度或模型下沉。
- 通信瓶颈：优化通信策略（如使用HIXL替代HCCL），调整并行配置。
- 量化精度问题：调整量化配置，确保量化参数正确。

处理后需要回看SLO指标是否恢复、模型执行耗时是否下降、NPU利用率是否正常。

## 定位方法论总结

该场景的完整判断链是：先用ms-service-metric确认SLO劣化的具体指标和劣化幅度；再用msServiceProfiler采集ModelExecute domain数据，通过`batch.csv`和`forward.csv`确认模型执行耗时是否增加；然后用服务化拆解工具细粒度定位模型执行的瓶颈子阶段；接着通过Timeline视图和算子级Profiling定位具体瓶颈算子；最后根据瓶颈类型（算子/通信/调度/内存）采取针对性优化。

核心判断逻辑：

- 如果`modelExec`的`during_time(ms)`增加而`batch_size`未变 → 模型执行效率下降
- 如果`forward.csv`中`bubble_time(ms)`也增加 → 可能是调度或通信问题
- 如果特定`dp_rank`的执行耗时偏高 → 设备或进程异常
- 如果`request.csv`中`queue_wait_time(ms)`正常但`execution_time(ms)`高 → 瓶颈在执行阶段而非排队
- 如果`request_status.csv`中`swapped`请求数>0 → NPU显存不足触发Swap

## 对工具的改进建议

### ms-service-metric

当前在线监控已能查看TTFT、TPS、E2E Latency等SLO指标。建议增加SLO劣化自动告警能力，当TTFT、TPS、E2E Latency等指标连续超过基线阈值时自动输出告警。增加模型执行耗时与调度耗时的比值监控，便于快速区分执行瓶颈和调度瓶颈。增加Swap请求数监控指标，当出现Swap时及时告警。

### msServiceProfiler

当前Profiler已能通过`batch.csv`和`forward.csv`分析模型执行耗时。建议在`batch.csv`中增加模型执行阶段的子阶段拆解字段（如"数据下发耗时"、"算子执行耗时"、"数据接收耗时"），便于直接定位执行瓶颈子环节。增加与历史数据的自动对比能力，当检测到模型执行耗时显著增加时，自动提示可能的劣化原因。在解析结果中增加SLO影响评估面板，自动计算模型执行耗时增加对TTFT、TPS等SLO指标的量化影响。

### 服务化拆解工具

当前拆解工具需要手动指定`batch_size`或`rid`。建议增加模型执行阶段的自动拆解能力，无需手动指定参数，自动选取耗时异常的Batch进行拆解。在拆解结果中增加与基线数据的对比，标注各子阶段耗时的变化幅度。

### 服务化专家建议工具

当前专家建议工具已能基于benchmark结果给出调参建议。建议增加基于性能劣化数据的自动调参建议，当检测到模型执行耗时增加时，自动分析是否可通过调整`maxBatchSize`、`maxPrefillBatchSize`等参数缓解。
