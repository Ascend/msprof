# scheduler耗时过长

## 问题背景

在服务化推理场景中，调度器（Scheduler）负责将到达的推理请求按一定策略组Batch并下发到NPU执行。当调度器自身耗时占比过高时，会导致NPU设备空闲等待，整体吞吐下降、请求排队时间增加。尤其在Host Bound场景下（如小模型推理、Decode阶段），单个算子的Host下发时间可能超过Device执行时间，调度开销成为系统瓶颈。昇腾CANN通过图模式调度和模型下沉调度技术可优化此问题，但在实际部署中，调度策略配置不当、动态调度优先级切换、组Batch逻辑复杂等因素仍可能导致scheduler耗时过长。

## 问题来源

推理

## 问题现象

用户通常先看到推理服务整体吞吐低于预期，NPU利用率偏低但请求排队（waiting）数量持续增长。在Grafana中按调度指标拆开后，常见现象是：

- `waiting_batch_size`指标持续处于高位，而`batch_size`（running请求数）未达到配置的`maxBatchSize`上限。
- 从`request_status.csv`中可观察到大量请求长期处于waiting状态，running状态请求数波动较大。
- NPU执行流上出现间歇性空闲，Timeline视图中连续的Schedule色块之间存在明显的空闲间隙（bubble）。

使用msServiceProfiler采集数据后，在`batch.csv`中观察到`name`为`batchFrameworkProcessing`（组Batch阶段）的`during_time(ms)`显著偏高，与`modelExec`（模型执行阶段）耗时之比异常。典型表现为：组Batch耗时占总Batch耗时的比例偏高，而正常场景下该比例通常较低。在Host Bound模型（如小参数量的Encoder模型或Decode阶段的小Batch场景）中尤为明显。

## 定位过程

### 步骤 1：先确认调度瓶颈是否存在

在Grafana中查看`batch_size`、`waiting_batch_size`、`num_running_reqs`、`num_waiting_reqs`等调度相关指标。如果`waiting_batch_size`持续增长而`batch_size`未达到上限，说明调度器未能及时将等待请求组Batch下发。同时观察NPU利用率指标，若NPU利用率低但等待队列长，可初步判断存在调度瓶颈。

### 步骤 2：用Profiler采集调度阶段数据

配置`ms_service_profiler_config.json`，设置`domain`为`"Schedule; Request; ModelExecute"`，开启数据采集。采集完成后执行解析命令：

```bash
python3 -m ms_service_profiler.parse --input-path ${PATH}/prof_dir/
```

解析生成`batch.csv`、`request.csv`、`forward.csv`、`chrome_tracing.json`等文件。

### 步骤 3：分析batch.csv中的调度耗时

在`batch.csv`中筛选`name`为`batchFrameworkProcessing`的行，统计其`during_time(ms)`的分布（平均值、P90、P99）。同时筛选`name`为`modelExec`的行，计算组Batch耗时与模型执行耗时的比值。如果组Batch耗时占比偏高，说明调度阶段存在瓶颈。

按`dp_rank`维度分别统计各DP域的调度耗时，判断是否为特定DP域调度慢。按`batch_type`（prefill/decode）分别统计，判断是Prefill调度慢还是Decode调度慢。

### 步骤 4：用服务化拆解工具细粒度拆解Batch执行阶段

```bash
msserviceprofiler split --input-path /path/to/input --decode-batch-size 1 --decode-number 100
```

拆解后查看`decode.csv`中各子阶段（如数据下发、模型执行、数据接收等）的耗时分布，确认调度阶段的具体耗时瓶颈在哪个子环节。

### 步骤 5：通过Timeline视图确认bubble位置

打开`chrome_tracing.json`，在Timeline视图中观察Schedule色块与ModelExecute色块之间的时序关系。如果存在大量bubble（空闲间隙），且bubble出现在组Batch阶段而非模型执行阶段，可确认调度器是瓶颈。

结合`forward.csv`中的`bubble_time(ms)`字段，统计forward之间的空泡时间。如果空泡时间占比较高，说明调度下发不及时导致NPU等待。

### 步骤 6：用多维度解析工具获取整体统计

```bash
msserviceprofiler analyze --input-path=/path/to/input
```

查看`batch_summary.csv`中prefill和decode的batch数量和执行时间统计，从服务整体维度判断调度效率。

## 问题根因

调度器耗时过长的常见根因包括：

1. **Host Bound场景**：小模型或Decode阶段单算子执行时间极短，但Host调度每个算子的下发流程耗时相对较长，导致Device频繁空闲等待。这是昇腾NPU上Host调度模式的固有问题，可通过CANN的图模式调度或模型下沉调度解决。

2. **调度策略配置不当**：`maxBatchSize`设置过小导致频繁组Batch；动态调度优先级在单并发场景下引入额外的策略切换开销；`maxPrefillBatchSize`与`maxBatchSize`配比不合理导致Prefill和Decode调度互相阻塞。

3. **组Batch逻辑复杂度过高**：请求数量大、请求长度差异大时，调度器需要遍历大量候选请求进行组Batch匹配，匹配算法复杂度随请求数增长。

4. **框架适配问题**：调度器与昇腾NPU的Task下发机制未充分优化，未启用图模式调度或模型下沉，仍使用单算子模式逐算子下发。

5. **资源竞争**：调度器线程与其它服务线程竞争CPU资源，导致调度延迟抖动。

## 解决方法

- Host Bound：启用CANN图模式调度或模型下沉，减少Host下发开销。
- 调度策略不当：调整`maxBatchSize`和`maxPrefillBatchSize`配比，关闭不必要的动态调度优先级。
- 组Batch逻辑复杂：优化组Batch算法，减少候选请求遍历范围。
- 框架适配：确认已启用图模式调度或模型下沉，避免单算子模式。
- 资源竞争：确保调度线程有足够CPU资源，避免与其它服务线程竞争。

处理后需要回看`waiting_batch_size`是否下降、`batch_size`是否接近上限、组Batch耗时占比是否降低、NPU利用率是否提升。

## 定位方法论总结

该场景的完整判断链是：先用ms-service-metric确认`waiting_batch_size`高、`batch_size`未达上限、NPU利用率低的现象；再用msServiceProfiler采集Schedule和ModelExecute domain数据，通过`batch.csv`对比组Batch耗时与模型执行耗时；然后用服务化拆解工具细粒度拆解Batch各子阶段耗时；最后通过Timeline视图确认bubble位置和持续时间。

核心判断逻辑：如果组Batch耗时占比高且Timeline上存在大量调度间隙，则瓶颈在调度器；如果模型执行耗时占比高，则瓶颈在模型侧。

## 对工具的改进建议

### ms-service-metric

当前在线监控已能查看`waiting_batch_size`、`batch_size`等调度指标。建议增加调度效率指标，如"调度耗时/模型执行耗时"比值、"平均组Batch延迟"等，便于在线快速识别调度瓶颈。增加Host Bound风险提示，当检测到小模型或Decode阶段NPU利用率持续偏低时，自动提示可能存在Host Bound。

### msServiceProfiler

当前Profiler已能通过`batch.csv`对比组Batch耗时与模型执行耗时。建议在`batch.csv`中增加调度子阶段拆解字段，如"请求匹配耗时"、"Token分配耗时"、"Task下发耗时"等，便于直接定位调度瓶颈子环节。在Timeline视图中增加调度效率面板，自动计算并展示组Batch耗时占比、bubble占比等关键指标。

### 服务化拆解工具

当前拆解工具需要手动指定`batch_size`或`rid`。建议增加自动识别耗时异常Batch的能力，自动选取耗时最长的Top N个Batch进行拆解分析。
