# 框架调度下发不同步问题调优指导实例

## 场景说明

在大模型推理服务化场景中，框架侧通常需要完成请求入队、批处理调度、Host侧任务下发、Device侧执行、跨节点或跨卡同步等流程。若不同节点或不同Device的任务下发节奏不一致，可能出现快慢卡、同步等待时间异常、首Token时延抖动、吞吐下降等问题。

本文以PD分离、多节点并发推理场景为例，介绍如何使用msServiceProfiler采集调度链路数据，定位框架调度下发不同步问题，并根据一致性要求选择优化方案。

## 问题现象

**典型表现**

- 多节点模型执行开始时间存在明显差异，部分节点已进入Device执行，部分节点仍停留在Host下发或调度等待阶段。
- 同一轮迭代中，不同Device的BatchSchedule、forward或自定义Span耗时差异明显，出现快慢卡现象。
- 同步点前后存在较长空洞，端到端时延中出现90ms以上的同步等待。
- Request_Latency_curve或First_Token_Latency_curve中P90/P99抖动明显，平均吞吐未达到预期。
- 在高并发下，关键调度线程被非关键线程抢占，主线程等待时间增加。

**影响范围**

| 影响项 | 表现 |
| --- | --- |
| 首Token时延 | Prefill阶段调度等待增加，首Token P90/P99升高。 |
| Decode吞吐 | 部分快卡等待慢卡，Decode步进被同步点拉长。 |
| 资源利用率 | Device计算空洞增多，AI Core利用率下降。 |
| 稳定性 | 流量波动时尾时延放大，跨节点同步更容易被慢节点拖累。 |

## 数据采集

### 功能说明

使用msServiceProfiler采集框架调度、Host下发、Device执行和同步等待等关键阶段的Span、Event、Metric和Link数据，结合BatchSchedule.csv、forward.csv、Request_Latency_curve等数据判断调度下发是否同步。

### 注意事项

- 建议先在压测环境复现问题，再开启采集，避免采集范围过大影响线上服务。
- 多节点场景采集前需校准各节点系统时间，否则时间轴对齐结果可能存在偏差。
- 若需要分析Host与Device之间的下发耗时，可在配置中开启acl任务耗时相关采集项，但需注意该能力可能带来额外性能开销。
- 若已经使用msprof动态采集能力，不建议同时开启与其冲突的采集开关。

### 配置示例

创建ms_service_profiler_config.json，开启服务化性能数据采集。

```json
{
    "enable": 1,
    "prof_dir": "${HOME}/.ms_server_profiler",
    "profiler_level": "INFO",
    "acl_task_time": 1,
    "acl_prof_task_time_level": "L0"
}
```

设置环境变量并启动服务。

```bash
export SERVICE_PROF_CONFIG_PATH=/path/to/ms_service_profiler_config.json
```

### 插桩建议

在框架调度链路中对关键阶段增加Span采集。

```C++
auto scheduleSpan = PROF(INFO, SpanStart("FrameworkSchedule"));

// 请求出队、batch构造、策略选择等调度逻辑

PROF(scheduleSpan.SpanEnd());

auto hostSubmitSpan = PROF(INFO, SpanStart("HostSubmit"));

// Host侧任务下发、流绑定、通信任务提交等逻辑

PROF(hostSubmitSpan.SpanEnd());

auto deviceExecSpan = PROF(INFO, SpanStart("DeviceExecute"));

// Device侧计算执行或等待执行完成

PROF(deviceExecSpan.SpanEnd());
```

对同步点和队列积压增加Event和Metric采集。

```C++
PROF(INFO, Event("BeforeGlobalSync"));
PROF(INFO, Metric("dispatchQueueSize", queueSize).MetricScope("scheduler", rankId).Launch());
PROF(INFO, Metric("syncWaitMs", syncWaitMs).MetricScope("rank", rankId).Launch());
PROF(INFO, Event("AfterGlobalSync"));
```

跨线程或跨模块链路可使用Link关联请求ID，避免只看到局部耗时。

```C++
PROF(INFO, Link("request_in_scheduler", "request_in_executor"));
```

## 定位方法

1. 观察BatchSchedule.csv中同一批次在不同进程、不同Device上的开始时间和耗时。
   - 若开始时间相差较大，优先排查调度线程、Host下发队列、跨节点时钟和负载均衡策略。
   - 若开始时间接近但结束时间差异明显，优先排查Device执行、专家负载不均、通信等待或算子耗时差异。

2. 对比FrameworkSchedule、HostSubmit、DeviceExecute三个Span。
   - FrameworkSchedule耗时长：说明框架调度本身存在瓶颈，可能是同步队列、串行事务、锁竞争或批处理策略造成。
   - HostSubmit耗时长：说明Host侧下发能力不足，可能是单线程提交、流管理阻塞或通信任务提交串行化造成。
   - DeviceExecute前存在空洞：说明任务已经完成调度但未及时进入Device执行，需检查Device队列、同步点和上游下发节奏。

3. 检查syncWaitMs和dispatchQueueSize指标。
   - syncWaitMs在部分rank持续偏高，通常表示快卡等待慢卡。
   - dispatchQueueSize周期性堆积，通常表示调度线程生产速度与执行线程消费速度不匹配。
   - 队列不高但等待高，通常表示跨节点同步、通信或强一致性屏障成为瓶颈。

4. 结合Request_Latency_curve和First_Token_Latency_curve判断用户侧影响。
   - 若平均值变化不大但P99明显升高，说明问题主要影响尾时延。
   - 若平均值和P99同时升高，说明调度下发链路已成为整体瓶颈。

## 原因分析与解决方案

### 分布式调度差异导致快慢卡

**原因**

在PD分离或多节点部署场景下，各节点模型执行开始时间不一致。快节点先到达同步点后等待慢节点，形成明显同步等待。

**解决方案**

- 将调度策略调整为Host统一下发、Device按本地队列执行，减少不同节点之间的启动时间差。
- 对跨节点请求分配增加时间窗约束，避免同一批次被拆分到状态差异过大的节点。
- 对rank维度的HostSubmit开始时间进行监控，超过阈值时触发告警或降级策略。
- 在强同步阶段前增加轻量级对齐逻辑，避免局部节点过早进入等待。

### 同步调度串行化导致瓶颈

**原因**

同步调度模式要求每个分区或每轮任务严格串行完成。若单分区内存在数千个任务，调度线程需要串行处理队列、事务、锁和状态更新，导致下发延迟放大。

**解决方案**

- 若业务允许弱一致性，将同步调度改为异步调度，减少非必要阻塞。
- 若业务必须强一致性，增加调度分区数M，以更多调度资源换取更低单分区排队时间。
- 将调度状态拆分为请求级、批次级和设备级，降低全局锁粒度。
- 将耗时统计、日志落盘、低优先级状态更新从关键路径移出。

### 异步调度缺少背压控制

**原因**

异步调度可以降低阻塞，但若没有队列上限、优先级和结果回收机制，可能造成任务堆积、乱序放大或尾时延恶化。

**解决方案**

- 使用Future机制或双线程架构，将schedule线程和generator线程解耦。
- schedule线程负责批处理决策和任务提交，generator线程负责结果消费、后处理和下一轮触发。
- 在线程之间使用线程安全队列传递任务，并通过Event、条件变量或轻量级信号控制唤醒。
- 设置队列高水位和超时策略，避免异步任务无限堆积。

示例流程如下：

```text
请求入队 -> schedule线程构造batch -> HostSubmit异步下发 -> generator线程消费结果 -> 触发下一轮Decode
```

### Pipeline调度策略不匹配

**原因**

同步pipeline内存效率较好，但容易被慢阶段拖累；异步pipeline统计效率较高，但可能产生权重版本差异或结果修正成本。

**解决方案**

- 对显存压力较大的场景，优先采用同步pipeline并通过动态规划优化内存调度。
- 对吞吐优先且可接受一定统计差异的训练或生成场景，可采用异步pipeline，并配合学习率调度、差异校正或阶段间缓冲。
- 对复杂依赖图，使用BSP超步同步控制全局一致性，在超步内部使用异步执行提升资源利用率。

### 线程QoS资源分配不合理

**原因**

高并发下，关键调度线程、Host下发线程或结果回收线程可能被日志、监控、低优先级后处理线程抢占，导致主线程等待增加。

**解决方案**

- 提升schedule线程、HostSubmit线程、通信提交线程的QoS等级。
- 降低日志、周期性监控、统计聚合等非关键线程优先级。
- 将关键线程绑定到稳定CPU核，减少线程迁移。
- 使用Metric采集关键线程队列长度、唤醒间隔和同步等待时间，评估QoS调整收益。

## 优化验证

优化后重新采集相同压测流量下的性能数据，并对比以下指标。

| 指标 | 期望结果 |
| --- | --- |
| FrameworkSchedule耗时 | 平均值下降，P99抖动收敛。 |
| HostSubmit开始时间差 | 不同rank之间的开始时间差缩小。 |
| syncWaitMs | 同步等待明显下降，快慢卡差异减少。 |
| Request_Latency_curve | 端到端时延P90/P99下降。 |
| First_Token_Latency_curve | 首Token尾时延下降。 |
| Device利用率 | 计算空洞减少，利用率提升。 |

若优化后同步等待仍较高，建议继续从以下方向排查：

- 检查各节点系统时间是否同步。
- 检查网络通信耗时是否在同步点前后放大。
- 检查某些Device是否存在算子耗时异常或专家负载不均。
- 检查调度分区数M是否不足。
- 检查异步队列是否出现反压或结果回收不及时。

## 推荐处理策略

| 业务要求 | 推荐方案 |
| --- | --- |
| 允许弱一致性 | 优先采用异步调度，使用Future机制或双线程架构降低关键路径同步。 |
| 要求强一致性 | 保留同步屏障，增加调度分区数M，优化Host统一下发和Device执行对齐。 |
| 跨节点快慢卡明显 | 重点优化分布式调度策略，缩小各rank HostSubmit和DeviceExecute开始时间差。 |
| 复杂依赖场景 | 使用BSP超步同步控制一致性，超步内部结合工作窃取或局部异步执行。 |
| 高并发尾时延异常 | 调整关键线程QoS，拆分全局锁，增加队列背压和优先级控制。 |

## 总结

框架调度下发不同步的核心矛盾在于同步机制的严格性与性能需求之间的冲突。定位时应先通过msServiceProfiler把调度链路拆分为FrameworkSchedule、HostSubmit、DeviceExecute和SyncWait等阶段，再结合BatchSchedule、Request_Latency和First_Token_Latency判断瓶颈位置。优化时需根据业务一致性要求选择方案：弱一致性场景优先异步化，强一致性场景优先优化分布式下发、增加调度分区并缩小rank间启动差异。
