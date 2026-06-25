# 模型推理请求等待时间过长问题调优指导实例

## 场景说明

在大模型推理服务化场景中，请求从进入服务到开始执行模型推理之间，通常会经过网关转发、请求入队、批处理组batch、Prefill执行、Decode调度、结果回传等阶段。若请求等待时间过长，用户侧会表现为首Token时延升高、端到端时延P99变差、低负载下响应不稳定或高负载下排队持续堆积。

本文以推理请求等待时间过长为例，介绍如何使用msServiceProfiler采集Request和BatchSchedule数据，定位等待时间主要发生在入口排队、组batch等待、Prefill/Decode资源竞争、外部依赖阻塞还是系统资源瓶颈，并给出批处理、调度、资源、缓存和监控方向的优化方案。

## 问题现象

**典型表现**

- Request_Status_curve中waiting状态请求数持续升高，长时间无法回落。
- First_Token_Latency_curve的P90/P99明显偏高，但模型单次执行耗时并未同比升高。
- Request_Latency_curve端到端时延抖动大，高峰流量下尾延迟快速放大。
- Batch_Size_curve显示batch长期偏小或等待时间过长，说明组batch策略与请求负载不匹配。
- Prefill请求和Decode请求互相抢占资源，短请求被长请求阻塞。
- 服务进程CPU繁忙但NPU利用率低，说明请求可能阻塞在I/O、调度或预处理阶段。

**影响范围**

| 影响项 | 表现 |
| --- | --- |
| 首Token时延 | 请求长时间等待组batch或Prefill资源，TTFT升高。 |
| 端到端时延 | 排队时间被放大，Request Latency P90/P99恶化。 |
| 吞吐 | batch未打满或调度过于保守，NPU利用率不足。 |
| 稳定性 | 高峰流量下队列持续堆积，出现超时或请求失败。 |
| 资源利用率 | CPU、内存、KVCache或NPU资源分配不均，导致局部瓶颈。 |

## 数据采集

### 功能说明

使用msServiceProfiler采集Request、BatchSchedule、ModelExecute、KVCache和Communication域数据，结合batch.csv、request.csv、BatchSchedule.csv、forward.csv以及可视化曲线，拆解请求等待时间来源。

### 注意事项

- 建议在稳定复现的压测场景下采集，记录并发、输入输出长度、maxBatchSize和max_wait_time等关键参数。
- 若等待时间主要出现在服务入口或外部依赖，需要同时采集业务网关、数据库、缓存和后处理链路日志。
- 多实例或分布式集群场景需保证各节点时间同步，避免Timeline分析出现偏差。
- 开启更多domain会增加采集数据量，建议先采集Request和BatchSchedule，再按需增加ModelExecute、KVCache和Communication。

### 配置示例

创建ms_service_profiler_config.json，采集请求状态、批处理调度和模型执行数据。

```json
{
    "enable": 1,
    "prof_dir": "${HOME}/.ms_server_profiler",
    "profiler_level": "INFO",
    "domain": "Request;BatchSchedule;ModelExecute;KVCache;Communication"
}
```

启动服务前设置配置文件路径。

```bash
export SERVICE_PROF_CONFIG_PATH=/path/to/ms_service_profiler_config.json
```

解析时建议导出默认Span数据，重点查看forward.csv和BatchSchedule.csv。

```bash
ms_service_profiler_parse --input-path ${PROF_DIR} --output-path ${OUTPUT_DIR} --span
```

### 插桩建议

若框架中已有自定义调度逻辑，可增加请求排队、组batch、外部依赖和模型执行前后的Span与Metric。

```C++
auto enqueueSpan = PROF(INFO, SpanStart("RequestEnqueue"));

// 请求入队、鉴权、参数解析等逻辑

PROF(enqueueSpan.SpanEnd());

auto batchWaitSpan = PROF(INFO, SpanStart("BatchWait"));

// 等待组batch或等待max_wait_time触发

PROF(batchWaitSpan.SpanEnd());

auto modelReadySpan = PROF(INFO, SpanStart("ModelReady"));

// Prefill/Decode调度完成，准备进入模型执行

PROF(modelReadySpan.SpanEnd());
```

采集队列长度、组batch等待时间和外部依赖耗时。

```C++
PROF(INFO, Metric("requestQueueSize", requestQueueSize).MetricScope("scheduler", instanceId).Launch());
PROF(INFO, Metric("batchWaitMs", batchWaitMs).MetricScope("batch", batchId).Launch());
PROF(INFO, Metric("externalIOMs", externalIOMs).MetricScope("request", reqId).Launch());
```

## 定位方法

1. 查看Request_Status_curve。
   - waiting请求数持续升高：入口流量超过服务消费能力，需检查batch策略、实例数量和资源瓶颈。
   - waiting周期性升高后回落：可能是max_wait_time过长或批处理窗口过大导致。
   - running请求数很低但waiting很高：请求可能阻塞在调度、I/O或线程池阶段。

2. 查看Batch_Size_curve和batch.csv。
   - batch size长期低于maxBatchSize：可能是max_wait_time过短、并发不足或调度条件过严。
   - batch size接近maxBatchSize但等待仍高：可能是模型执行能力不足，需扩容或优化Prefill/Decode调度。
   - prefill_batch_size和decode_batch_size波动大：可能是Prefill和Decode资源竞争，需调整优先级策略。

3. 查看First_Token_Latency_curve。
   - TTFT升高且Prefill执行耗时正常：等待主要发生在请求入队到Prefill开始之间。
   - TTFT升高且Prefill执行耗时也升高：需检查maxPrefillBatchSize、maxSeqLen、KVCache和Prefill算子性能。

4. 查看Request_Latency_curve。
   - 端到端P99升高但平均值变化小：说明少量请求被长队列、长上下文或外部依赖拖慢。
   - 平均值和P99同时升高：说明服务整体消费能力不足，应优先扩容或降低单请求计算成本。

## 原因分析与解决方案

### 批处理参数不匹配

**原因**

maxBatchSize过小会限制吞吐，max_wait_time过长会让请求在低负载下等待过久，max_wait_time过短又会导致batch难以打满。若参数与实际并发和请求长度不匹配，会同时影响等待时间和吞吐。

**解决方案**

- 根据硬件资源逐步增大maxBatchSize，例如将voc_inference_max_bs从4调整为8，并观察NPU利用率和Request Latency。
- 缩短最大等待时间，例如将voc_inference_max_wait_ms调整到20ms，降低低负载下的组batch等待。
- 建立动态批处理策略：低负载时适当等待以提升batch利用率，高负载时缩短等待窗口以降低尾延迟。
- 每次调整后对比Batch_Size_curve、Request_Status_curve、First_Token_Latency和吞吐，避免只优化单一指标。

### Prefill/Decode调度策略不合理

**原因**

Prefill阶段通常计算量大，Decode阶段需要持续迭代。如果调度策略固定或只按到达顺序处理，长Prefill请求可能阻塞Decode，Decode持续占用也可能导致新请求首Token等待过长。

**解决方案**

- 启用Prefill/Decode优先级调度，根据prefillTimeMsPerReq与decodeTimeMsPerReq动态选择调度策略。
- 对短请求设置更高调度优先级，减少短请求被长上下文请求阻塞的概率。
- 在高并发下限制单轮Prefill占用，避免Decode被长期饿死。
- 对长上下文或超长输出请求单独分桶，避免拖慢普通请求队列。
- 结合BatchSchedule.csv观察prefill和decode batch的时间分布，确认调度调整是否降低排队时间。

### 同步阻塞或外部依赖拖慢主线程

**原因**

服务化链路中可能存在鉴权、数据库查询、向量检索、日志落盘、网络调用或后处理逻辑。如果这些操作在主线程或调度线程中同步执行，会导致请求无法及时入队或组batch。

**解决方案**

- 引入异步非阻塞架构，例如使用FastAPI等异步框架处理I/O操作。
- 将数据库查询、缓存访问、日志落盘和后处理从调度关键路径移出。
- 为外部依赖设置超时和降级策略，避免单个慢依赖拖住整个请求。
- 使用Metric记录externalIOMs和requestQueueSize，确认等待是否来自外部I/O。

### 实例资源不足或负载不均

**原因**

单实例承接过多请求时，队列会持续堆积。多实例部署中若路由策略粗糙，可能出现部分实例过载、部分实例空闲，导致整体P99升高。

**解决方案**

- 部署分布式推理集群，通过Kubernetes Service或网关进行多实例负载均衡。
- 使用HPA基于QPS、队列长度、NPU利用率或P99延迟自动扩缩容。
- 建立智能请求路由，采用“网关粗分桶 + 实例细预测”的两阶段调度策略。
- 将不同长度、不同业务优先级或不同模型版本的请求分流到对应实例池。
- 对过载实例设置熔断和排队上限，避免尾延迟无限放大。

### Prefill与Decode资源竞争

**原因**

Prefill阶段和Decode阶段对计算、显存和KVCache的需求不同。若二者部署在同一资源池中，在长上下文或高并发场景下容易互相抢占，导致请求等待和Decode抖动。

**解决方案**

- 采用PD分离架构，将Prefill和Decode部署在不同节点或实例池中。
- 通过高性能KV Cache传输降低Prefill到Decode切换成本。
- 分别为Prefill实例和Decode实例设置不同的batch、并发和扩缩容策略。
- 对PD分离链路采集Communication域数据，确认KV Cache传输没有成为新的瓶颈。

### 缓存与预处理缺失

**原因**

重复请求、相似查询、固定系统Prompt和长历史上下文如果每次都完整计算，会增加Prefill阶段负载和请求等待时间。

**解决方案**

- 构建向量缓存层，例如使用Redis缓存热门问题的Embedding或检索结果，减少重复Embedding模型调用。
- 对高频固定前缀启用Prefix Cache，复用历史上下文的KV Cache，降低Prefill计算开销。
- 对请求预处理结果进行缓存，例如模板拼接、分词或路由判定结果。
- 监控缓存命中率，并将命中率与TTFT、Prefill耗时关联分析。

### 系统级性能抖动

**原因**

CPU调度抖动、内存页抖动、线程迁移或KVCache显存碎片化，可能导致请求等待时间不稳定。流式输入或语音模型场景中，内存页和CPU调度影响会更明显。

**解决方案**

- 对关键线程开启CPU绑核，减少线程迁移。
- 根据系统要求评估透明大页配置，降低内存管理抖动。
- 使用PagedAttention类技术对KV Cache进行分块管理，提高显存利用率并降低碎片化。
- 对语音流式输入场景，优化内存页和缓冲区大小，减少小块数据频繁拷贝。

## 优化验证

建议按“批处理参数 -> 调度策略 -> 异步链路 -> 资源扩容 -> 缓存与系统优化”的顺序逐项验证。

| 优化项 | 观察指标 | 期望结果 |
| --- | --- | --- |
| 调整maxBatchSize | Batch_Size_curve、吞吐 | batch更接近目标大小，NPU利用率提升。 |
| 调整max_wait_time | First_Token_Latency、Request_Status_curve | TTFT下降，waiting队列更快回落。 |
| Prefill/Decode优先级 | BatchSchedule.csv、Decode时延 | Prefill和Decode互相阻塞减少。 |
| 异步I/O | externalIOMs、requestQueueSize | 主线程阻塞降低，请求入队更稳定。 |
| 分布式扩容 | P99延迟、实例负载 | 高峰期waiting队列不再持续堆积。 |
| Prefix Cache | Prefill耗时、TTFT | 重复前缀请求首Token时延下降。 |
| PagedAttention | KVCache使用率、并发 | 显存利用率提升，可承接更多请求。 |

优化有效时，通常会看到以下结果：

- Request_Status_curve中waiting队列峰值下降，回落速度变快。
- First_Token_Latency的P90/P99下降。
- Request_Latency的P99更稳定，高峰期超时减少。
- Batch_Size_curve更符合预期，低负载不过度等待，高负载不过度堆积。
- NPU利用率提升，CPU或I/O阻塞时间下降。

在批处理拼接推理生效的场景中，总耗时可由串行的多次单请求耗时降低到接近一次批处理执行时间，NPU利用率也会明显提升。若动态批处理和异步架构同时生效，应重点观察TTFT、非首Token平均延迟和tokens/s是否同时改善。

## 推荐处理策略

| 问题类型 | 推荐方案 |
| --- | --- |
| 低负载下等待高 | 缩短max_wait_time，降低组batch等待窗口。 |
| 高负载下队列堆积 | 增大maxBatchSize或扩容实例，启用动态批处理。 |
| TTFT高但模型执行正常 | 优先排查请求入队、组batch等待、外部I/O和Prefill调度。 |
| Decode抖动明显 | 优化Prefill/Decode优先级，评估PD分离和通信优化。 |
| 重复请求占比高 | 启用向量缓存、结果缓存或Prefix Cache。 |
| KVCache接近上限 | 调整maxSeqLen、限制并发或启用PagedAttention类管理能力。 |
| 多实例P99高 | 优化请求路由，使用HPA和实例细粒度负载预测。 |

## 总结

模型推理请求等待时间过长的核心原因通常不只在模型执行本身，而是出现在请求入队、组batch等待、Prefill/Decode调度、外部I/O、资源竞争和缓存缺失等服务化链路中。定位时应先使用msServiceProfiler采集Request和BatchSchedule数据，通过Request_Status_curve、Batch_Size_curve、First_Token_Latency和Request_Latency判断等待发生位置。优化时建议优先调整maxBatchSize和max_wait_time，再引入Prefill/Decode优先级调度、异步I/O、分布式扩容、PD分离、Prefix Cache和PagedAttention等能力，逐步降低TTFT和端到端P99。
