# Troubleshooting Guide: Long Wait Time for Model Inference Requests

## Scenario Description

In foundation-model inference service scenarios, a request typically goes through multiple stages from entering the service to starting model inference, including gateway forwarding, request queueing, batch formation, prefill execution, decode scheduling, and result delivery. If a request waits too long, users may experience increased time to first token (TTFT), degraded end-to-end P99 latency, unstable response time under low load, or continuously growing queues under high load.

This document uses long wait time for inference requests as an example to describe how to use `msServiceProfiler` to collect `Request` and `BatchSchedule` data, identify whether the waiting time is primarily caused by queueing at the service entry point, batch formation, prefill/decode resource contention, external dependency blocking, or system resource bottlenecks, and provide optimization approaches for batching, scheduling, resources, caching, and monitoring.

## Symptoms

**Typical Symptoms**

- The number of requests in the `waiting` state in `Request_Status_curve` continues to increase and does not decrease for an extended period.
- P90/P99 of `First_Token_Latency_curve` is significantly elevated, while individual model execution durations do not increase proportionally.
- `Request_Latency_curve` shows significant fluctuations in end-to-end latency, and tail latency increases rapidly during peak traffic.
- `Batch_Size_curve` shows that the batch size remains small or that batch formation takes too long, indicating that the batch formation strategy does not match the request workload.
- Prefill and decode requests contend for resources, causing short requests to be blocked by long requests.
- The service process has high CPU activity but low NPU utilization, indicating that requests may be blocked in I/O, scheduling, or preprocessing stages.

**Impact**

| Item | Symptom |
| --- | --- |
| Time to first token | Requests wait too long for batch formation or prefill resources, resulting in higher TTFT. |
| End-to-end latency | Queueing time is amplified, resulting in degraded P90/P99 request latency. |
| Throughput | Batches are not filled to the expected size or scheduling is too conservative, resulting in low NPU utilization. |
| Stability | Queues continue to grow under peak traffic, resulting in timeouts or request failures. |
| Resource utilization | CPU, memory, KV Cache, or NPU resources are allocated unevenly, resulting in local resource bottlenecks. |

## Data Collection

### Function Description

Use `msServiceProfiler` to collect data from the `Request`, `BatchSchedule`, `ModelExecute`, `KVCache`, and `Communication` domains. Combine the collected data with `batch.csv`, `request.csv`, `BatchSchedule.csv`, `forward.csv`, and visualization curves to identify the sources of request waiting time.

### Precautions

- Collect profile data in a stress testing scenario that can reproduce the issue consistently. Record key parameters such as concurrency, input and output lengths, `maxBatchSize`, and `max_wait_time`.
- If most of the waiting time occurs at the service entry point or in external dependencies, also collect logs from the service gateway, database, cache, and postprocessing paths.
- In multi-instance or distributed cluster scenarios, ensure that the clocks of all nodes are synchronized to avoid deviations in Timeline analysis.
- Enabling more domains increases the amount of collected data. It is recommended to collect `Request` and `BatchSchedule` data first, and then add `ModelExecute`, `KVCache`, and `Communication` data as needed.

### Configuration Example

Create `ms_service_profiler_config.json` to collect request status, batch scheduling, and model execution data.

```json
{
    "enable": 1,
    "prof_dir": "${HOME}/.ms_server_profiler",
    "profiler_level": "INFO",
    "domain": "Request;BatchSchedule;ModelExecute;KVCache;Communication"
}
```

Set the configuration file path before starting the service.

```bash
export SERVICE_PROF_CONFIG_PATH=/path/to/ms_service_profiler_config.json
```

When parsing the data, it is recommended to export the default span data and focus on `forward.csv` and `BatchSchedule.csv`.

```bash
ms_service_profiler_parse --input-path ${PROF_DIR} --output-path ${OUTPUT_DIR} --span
```

### Instrumentation Recommendations

If the framework already contains custom scheduling logic, add spans and metrics around request queueing, batch formation, external dependencies, and model execution.

```C++
auto enqueueSpan = PROF(INFO, SpanStart("RequestEnqueue"));

// Request queueing, authentication, parameter parsing, and other logic

PROF(enqueueSpan.SpanEnd());

auto batchWaitSpan = PROF(INFO, SpanStart("BatchWait"));

// Wait for batch formation or for max_wait_time to expire

PROF(batchWaitSpan.SpanEnd());

auto modelReadySpan = PROF(INFO, SpanStart("ModelReady"));

// Prefill/Decode scheduling is complete and the request is ready for model execution

PROF(modelReadySpan.SpanEnd());
```

Collect the queue length, batch formation wait time, and external dependency duration.

```C++
PROF(INFO, Metric("requestQueueSize", requestQueueSize).MetricScope("scheduler", instanceId).Launch());
PROF(INFO, Metric("batchWaitMs", batchWaitMs).MetricScope("batch", batchId).Launch());
PROF(INFO, Metric("externalIOMs", externalIOMs).MetricScope("request", reqId).Launch());
```

## Troubleshooting

1. Check `Request_Status_curve`.
   - If the number of `waiting` requests continues to increase, the incoming traffic exceeds the service consumption capacity. Check the batching strategy, number of instances, and resource bottlenecks.
   - If the number of `waiting` requests periodically increases and then decreases, `max_wait_time` may be too long or the batch formation window may be too large.
   - If the number of `running` requests is low while the number of `waiting` requests is high, requests may be blocked in scheduling, I/O, or thread pool processing.

2. Check `Batch_Size_curve` and `batch.csv`.
   - If the batch size remains below `maxBatchSize`, `max_wait_time` may be too short, concurrency may be insufficient, or scheduling conditions may be too restrictive.
   - If the batch size is close to `maxBatchSize` but waiting time remains high, model execution capacity may be insufficient. Consider scaling out or optimizing prefill/decode scheduling.
   - If `prefill_batch_size` and `decode_batch_size` fluctuate significantly, prefill and decode may be contending for resources. Consider adjusting the priority policy.

3. Check `First_Token_Latency_curve`.
   - If TTFT increases while prefill execution time remains normal, the waiting time primarily occurs between request queueing and the start of prefill.
   - If both TTFT and prefill execution time increase, check `maxPrefillBatchSize`, `maxSeqLen`, KV Cache, and prefill operator performance.

4. Check `Request_Latency_curve`.
   - If P99 end-to-end latency increases while average latency remains largely unchanged, a small number of requests may be delayed by long queues, long contexts, or external dependencies.
   - If both average latency and P99 latency increase, the overall service consumption capacity is insufficient. Prioritize scaling out or reducing the computation cost of individual requests.

## Cause Analysis and Solutions

### Mismatched Batching Parameters

**Cause**

A small `maxBatchSize` limits throughput. A long `max_wait_time` causes excessive waiting under low load, while a short `max_wait_time` makes it difficult to fill batches. If these parameters do not match the actual concurrency and request lengths, both waiting time and throughput are affected.

**Solution**

- Gradually increase `maxBatchSize` based on available hardware resources. For example, increase `voc_inference_max_bs` from `4` to `8` and observe NPU utilization and request latency.
- Reduce the maximum wait time. For example, set `voc_inference_max_wait_ms` to 20 ms to reduce batch formation wait time under low load.
- Implement dynamic batching: allow a longer wait under low load to improve batch utilization and shorten the wait window under high load to reduce tail latency.
- Compare `Batch_Size_curve`, `Request_Status_curve`, `First_Token_Latency`, and throughput after each adjustment to avoid optimizing only a single metric.

### Improper Prefill/Decode Scheduling Strategy

**Cause**

The prefill stage is typically compute-intensive, while the decode stage requires continuous iterations. If the scheduling policy is fixed or requests are processed only in arrival order, long prefill requests may block decode requests, while continuous decode execution may cause new requests to wait too long for their first token.

**Solution**

- Enable prefill/decode priority scheduling and dynamically select the scheduling policy based on `prefillTimeMsPerReq` and `decodeTimeMsPerReq`.
- Assign higher scheduling priority to short requests to reduce the probability of short requests being blocked by requests with long contexts.
- Limit the amount of resources that a single prefill round can consume under high concurrency to prevent decode from being starved for an extended period.
- Place requests with long contexts or long outputs into separate buckets to prevent them from slowing down the normal request queue.
- Use `BatchSchedule.csv` to analyze the time distribution of prefill and decode batches and verify whether scheduling changes reduce queueing time.

### Synchronous Blocking or External Dependencies Slowing Down The Main Thread

**Cause**

The service path may include authentication, database queries, vector retrieval, log persistence, network calls, or postprocessing. If these operations are performed synchronously on the main or scheduling thread, requests cannot be queued or formed into batches in a timely manner.

**Solution**

- Adopt an asynchronous, non-blocking architecture. For example, use an asynchronous framework such as FastAPI for I/O operations.
- Move database queries, cache access, log persistence, and postprocessing out of the critical scheduling path.
- Set timeouts and fallback mechanisms for external dependencies to prevent a single slow dependency from blocking the entire request.
- Use metrics to record `externalIOMs` and `requestQueueSize` to determine whether waiting is caused by external I/O.

### Insufficient Instance Resources or Load Imbalance

**Cause**

When a single instance handles too many requests, its queue continues to grow. In a multi-instance deployment, a coarse-grained routing strategy may overload some instances while leaving others idle, resulting in higher overall P99 latency.

**Solution**

- Deploy a distributed inference cluster and use Kubernetes Service or a gateway for load balancing across instances.
- Use HPA to automatically scale instances based on QPS, queue length, NPU utilization, or P99 latency.
- Implement intelligent request routing using a two-stage scheduling strategy of "coarse-grained bucketing at the gateway + fine-grained prediction at the instance."
- Route requests with different lengths, business priorities, or model versions to corresponding instance pools.
- Set circuit breakers and queue length limits for overloaded instances to prevent tail latency from increasing without bound.

### Prefill/Decode Resource Contention

**Cause**

The prefill and decode stages have different requirements for computation, memory, and KV Cache. When they share the same resource pool, they can easily contend for resources under long-context or high-concurrency workloads, resulting in request waiting and decode fluctuations.

**Solution**

- Adopt prefill/decode disaggregation (PD disaggregation) and deploy prefill and decode on different nodes or instance pools.
- Use high-performance KV Cache transfer to reduce the overhead of transferring KV Cache between prefill and decode.
- Configure different batch sizes, concurrency levels, and scaling policies for prefill and decode instances.
- Collect data from the Communication domain for the PD disaggregation path to verify that KV Cache transfer does not become a new bottleneck.

### Missing Caching or Preprocessing Optimization

**Cause**

If repeated requests, similar queries, fixed system prompts, and long conversation histories are fully processed each time, the prefill workload and request waiting time increase.

**Solution**

- Build a vector cache layer. For example, use Redis to cache embeddings or retrieval results for frequently requested queries and reduce repeated embedding model invocations.
- Enable prefix caching for frequently used fixed prefixes to reuse the KV Cache of historical contexts and reduce prefill computation.
- Cache request preprocessing results, such as template concatenation, tokenization, or routing decisions.
- Monitor the cache hit rate and correlate it with TTFT and prefill execution time.

### System-Level Performance Fluctuations

**Cause**

CPU scheduling fluctuations, memory paging overhead, thread migration, or KV Cache memory fragmentation may cause request waiting time to become unstable. The effects of memory paging and CPU scheduling may be more significant in streaming input or speech model scenarios.

**Solution**

- Pin critical threads to CPUs to reduce thread migration.
- Evaluate the Transparent Huge Pages configuration based on system requirements to reduce memory management fluctuations.
- Use PagedAttention-like techniques to manage KV Cache in blocks, improve memory utilization, and reduce fragmentation.
- For streaming speech input scenarios, optimize memory page and buffer sizes to reduce frequent copying of small blocks of data.

## Optimization Verification

It is recommended to verify optimizations in the following order: batching parameters -> scheduling strategy -> asynchronous processing -> resource scaling -> caching and system optimization.

| Optimization | Metrics | Expected Result |
| --- | --- | --- |
| Adjust `maxBatchSize` | `Batch_Size_curve`, throughput | The batch size is closer to the expected size, and NPU utilization increases. |
| Adjust `max_wait_time` | `First_Token_Latency`, `Request_Status_curve` | TTFT decreases, and the `waiting` queue clears more quickly. |
| Prefill/Decode priority | `BatchSchedule.csv`, decode latency | Mutual blocking between prefill and decode is reduced. |
| Asynchronous I/O | `externalIOMs`, `requestQueueSize` | Main-thread blocking is reduced, and request queueing becomes more stable. |
| Distributed scaling | P99 latency, instance load | The `waiting` queue no longer continues to grow during peak traffic. |
| Prefix cache | Prefill execution time, TTFT | TTFT decreases for requests with repeated prefixes. |
| PagedAttention | KV Cache utilization, concurrency | Memory utilization improves, allowing more requests to be served concurrently. |

When the optimization is effective, the following results are typically observed:

- The peak size of the `waiting` queue in `Request_Status_curve` decreases, and the queue clears more quickly.
- P90/P99 of `First_Token_Latency` decreases.
- P99 of `Request_Latency` becomes more stable, and the number of timeouts decreases during peak traffic.
- `Batch_Size_curve` better matches the expected behavior, with limited waiting under low load and limited queue buildup under high load.
- NPU utilization increases, while CPU or I/O blocking time decreases.

When batched inference is effective, the total execution time can be reduced from the sum of the execution time of multiple serial single-request executions to a duration close to that of a single batched execution, while NPU utilization also increases significantly. If dynamic batching and an asynchronous architecture are both enabled, pay particular attention to TTFT, average inter-token latency, and tokens/s to determine whether they improve simultaneously.

## Recommended Actions

| Issue Type | Recommended Solution |
| --- | --- |
| High waiting time under low load | Reduce `max_wait_time` to shorten the batch formation wait window. |
| Queue buildup under high load | Increase `maxBatchSize` or scale out instances, and enable dynamic batching. |
| High TTFT but normal model execution time | Prioritize checking request queueing, batch formation wait time, external I/O, and prefill scheduling. |
| Significant decode fluctuations | Optimize prefill/decode priorities and evaluate PD disaggregation and communication optimization. |
| High proportion of repeated requests | Enable vector caching, result caching, or prefix caching. |
| KV Cache close to its limit | Adjust `maxSeqLen`, limit concurrency, or enable PagedAttention-like management capabilities. |
| High P99 latency across multiple instances | Optimize request routing and use HPA and fine-grained instance load prediction. |

## Summary

Long wait time for model inference requests is typically not caused solely by model execution. It can also occur in request queueing, batch formation, prefill/decode scheduling, external I/O, resource contention, and missing caching mechanisms in the service path. During troubleshooting, first use `msServiceProfiler` to collect `Request` and `BatchSchedule` data, and use `Request_Status_curve`, `Batch_Size_curve`, `First_Token_Latency`, and `Request_Latency` to determine where the waiting time occurs. For optimization, prioritize adjusting `maxBatchSize` and `max_wait_time`. Then introduce prefill/decode priority scheduling, asynchronous I/O, distributed scaling, PD disaggregation, prefix caching, and PagedAttention as needed to progressively reduce TTFT and end-to-end P99 latency.
