# Multi-Instance Load Imbalance

## Background

Inference services typically use multiple instances to handle traffic. Ideally, the number of requests, token volume, queue length, and KV cache utilization should be roughly balanced across instances. If the ingress routing, instance weights, health status, or request length distribution is abnormal, a small number of instances may become hotspots while other instances remain idle, causing overall throughput and latency to be constrained by the hotspot instances.

## Source

Inference

## Symptoms

Users typically first observe that overall throughput is lower than expected for the available multi-instance capacity, or that P95/P99 latency is significantly higher than average latency. When the metrics are broken down by instance, the following symptoms are commonly observed:

- A small number of instances consistently have higher request counts, token throughput, or batch sizes than other instances.
- Hotspot instances have more waiting/pending requests, higher time to first token, and longer queue wait time.
- Hotspot instances have higher KV cache utilization and lower `free_kvcache_blocks`.
- Other instances remain idle, but the overall service already exhibits long-tail latency.

## Troubleshooting Process

### Step 1: Verifying Whether Hotspot Instances Exist

In the instance-level panels in Grafana, select the same load testing window or production time window and compare the following metrics across instances:

- Request QPS and the number of running/waiting requests.
- Prompt tokens, generated tokens, and total token throughput.
- Time to first token, end-to-end latency, and queue wait time.
- Batch size, KV cache utilization, and `free_kvcache_blocks`.

Short-term fluctuations do not necessarily indicate a problem. If a small number of instances remain consistently higher across multiple consecutive windows while other instances remain consistently lower, this likely indicates a load imbalance across instances.

### Step 2: Determining Whether the Imbalance Is Caused by Request Count or Request Size

Analyze request count and token count separately:

- If both the number of requests and the token volume are higher, prioritize checking ingress routing, instance weights, or connection stickiness based on gateway logs, load balancing configuration, service discovery status, and connection reuse.
- If the number of requests is similar but the token volume is higher and the average input and output lengths are longer, use request logs, load testing datasets, or request length statistics to determine whether long requests are concentrated on hotspot instances.
- If both the number of requests and the token volume are similar but hotspot instances have higher latency, use service startup parameters, device monitoring, process logs, and the Profiler to check instance configuration, device status, process status, and local resource contention.

First determine the type of imbalance based on this analysis, and then select the corresponding troubleshooting direction.

### Step 3: Checking Ingress Routing and Instance Status

If the imbalance is determined to be caused by request count, check the ingress configuration based on information from the gateway, service discovery, or load balancing system:

- Whether traffic weights are consistent across instances.
- Whether any instance has failed health checks and is therefore not receiving traffic.
- Whether long-lived connections, connection pools, or session stickiness are causing requests to be consistently routed to a small number of instances.
- Whether some instances failed to properly join the load balancer after a canary deployment, scaling operation, or restart.

If the imbalance is determined to be caused by request size, use request length statistics and the load balancing strategy to determine whether the current strategy distributes requests based only on request count without considering input length, output length, queue status, or KV cache utilization.

### Step 4: Verifying Whether Hotspot Instances Are Constrained by Resources

Continue checking the internal status of hotspot instances:

- Whether the number of waiting/pending requests continues to increase.
- Whether KV cache utilization remains high and free blocks are nearly exhausted.
- Whether NPU utilization is saturated. If NPU utilization is not saturated but the queue is growing, KV cache or scheduling resources are more likely to be the bottleneck.
- Whether the increase in end-to-end latency is primarily caused by queue wait time or the time to first token.

If a hotspot instance simultaneously exhibits queue buildup and high KV cache utilization, continue investigating based on the "Insufficient KV Block Count" scenario.

### Step 5: Comparing Hotspot and Idle Instances Using the Profiler

Collect `Schedule`, `Request`, and `KVCache` data from hotspot and idle instances separately, and focus on the following comparisons:

- `request.csv`: Compare the number of requests, input length, output length, `queue_wait_time(ms)`, and `first_token_latency(ms)`.
- `batch.csv`: Compare `batch_size`, `prefill_batch_size`, `decode_batch_size`, `total_scheduled_tokens`, and `during_time(ms)`.
- `kvcache.csv`: Compare `used_blocks`, `free_blocks`, and `kvcache_usage_rate`.

If hotspot instances have more requests or higher token volume, and the Profiler shows higher queue wait time, larger batch sizes, and higher KV cache utilization, the issue can be traced to load imbalance across instances.

## Root Causes

The request count, request length distribution, or instance capabilities differ across instances. Common root causes include incorrect load balancing weights, health check or service discovery issues, long-lived connections or session stickiness, concentration of long requests on a small number of instances, inconsistent instance configurations, and load balancing strategies that do not account for token volume, queue length, or KV cache utilization.

## Solutions

- Incorrect routing weights: Correct the routing or instance weights and ensure that all healthy instances are included in load balancing.
- Health check issues: Restore abnormal instances or remove unavailable instances from load balancing to prevent traffic from being routed only to a subset of instances.
- Long-lived connections or session stickiness: Adjust the connection pool, gateway strategy, or load balancing algorithm to reduce the likelihood of repeatedly routing requests to the same instances.
- Concentration of long requests: Schedule requests based on input/output token volume, queue length, or KV cache utilization, and isolate long and short requests when necessary.
- Inconsistent instance capabilities: Standardize the model version, startup parameters, parallelism configuration, memory configuration, and hardware specifications.
- KV cache exhaustion on hotspot instances: First throttle traffic or migrate traffic, and then adjust concurrency, request length, or KV cache capacity based on the insufficient KV block scenario.

After the issue is addressed, check whether the number of requests, token volume, number of waiting requests, KV cache utilization, and P99 latency become balanced across instances.

## Summary of the Troubleshooting Methodology

For multi-instance load imbalance, first use ms-service-metric to compare the number of requests, token throughput, number of waiting requests, latency, and KV cache utilization across instances to determine whether only a small number of instances have consistently become hotspots. After confirming an imbalance across instances, use `msServiceProfiler` to separately collect `request.csv`, `batch.csv`, and `kvcache.csv` from hotspot and idle instances for comparison, and distinguish among abnormal ingress routing or instance weights, imbalanced request length distribution, inconsistent instance capabilities, and resource exhaustion within hotspot instances.

## Suggestions for Improving the Tools

### `ms-service-metric`

Currently, `ms-service-metric` supports comparison of request volume, token volume, latency, queue status, and KV cache utilization across instances. It is recommended to add multi-instance load imbalance detection to automatically distinguish between "request count imbalance" and "request length/token volume imbalance" and prompt users to check ingress routing, instance weights, or request length distribution.

### `msServiceProfiler`

Currently, `msServiceProfiler` can separately collect `request.csv`, `batch.csv`, and `kvcache.csv` from hotspot and idle instances for manual comparison. It is recommended to add support for merging and analyzing profiling results from multiple instances and directly reporting differences between hotspot and idle instances in request volume, token volume, batch size, queue wait time, and KV cache utilization.
