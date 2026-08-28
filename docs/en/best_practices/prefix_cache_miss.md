# PrefixCache Misses

## Background

Long system prompts and multi-turn conversations are common scenarios in LLM inference applications. In long system prompt scenarios, the system prompt is the same across different requests, and the corresponding KV cache computation is also the same. In multi-turn conversation scenarios, each turn depends on the context of all previous turns, and the KV cache for previous turns must be recomputed for each subsequent turn. Prefix caching significantly reduces time to first token (TTFT) and prefill computation by caching and reusing the KV cache of previously computed common prefixes. Ascend-vLLM enables prefix caching by default. However, `PrefixCache` misses may still occur in practice due to unmet cache hit conditions, cache eviction caused by insufficient cache capacity, or an insufficient number of common prefix tokens across requests to fill a block. This can result in higher TTFT and longer prefill duration.

## Source

Inference

## Symptoms

Users typically first observe TTFT fluctuating or increasing, with some requests having significantly higher TTFT than similar requests. Further inspection of KV cache and request-related metrics may reveal the following:

- The `cache_hit_rate` field in `request.csv` is low, while `first_token_latency(ms)` is high.
- In `batch.csv`, `during_time(ms)` and `prefill_scheduled_tokens` are both high for prefill batches, indicating that a large number of prefix tokens are being recomputed rather than reused from the cache.
- In KV cache-related metrics, `kvcache_usage_rate` fluctuates significantly and `free_blocks` changes frequently, indicating that the cache blocks are continuously being allocated and released rather than stably reused.
- In `kvcache.csv`, frequent block allocation (`blocks_allocated`) and release (`blocks_freed`) operations can be observed. If `blocks_allocated` is close to the number of tokens in the request's full prefix, this indicates that a cache miss has caused the entire prefix to be reallocated.

Typical scenarios include the following: In multi-turn conversations, the TTFT of the second and subsequent requests is almost the same as that of the first request, indicating no significant acceleration from prefix caching. In long system prompt scenarios, different requests with the same system prompt have similar TTFT.

## Troubleshooting Process

### Step 1: Checking KV Cache Usage and TTFT Changes

In Grafana, check KV cache-related metrics such as `free_kvcache_blocks`, `allocated_kvcache_blocks`, and `kvcache_usage_rate`. If `kvcache_usage_rate` remains low for an extended period while `allocated_kvcache_blocks` fluctuates frequently, this indicates that the cache is repeatedly allocated and released rather than reused. Also monitor the TTFT metric (`first_token_latency`). If TTFT fluctuates significantly and correlates with changes in KV cache utilization, a `PrefixCache` hit rate issue may have occurred.

### Step 2: Collecting Request and KV Cache Data Using the Profiler

Configure `ms_service_profiler_config.json`, set `domain` to `"Request; KVCache; Schedule"`, and enable data collection. After collection is complete, run the following command to parse the data:

```bash
python3 -m ms_service_profiler.parse --input-path ${PATH}/prof_dir/
```

The parsing process generates files such as `request.csv`, `kvcache.csv`, and `batch.csv`.

### Step 3: Analyzing Cache Hit Rate and TTFT in `request.csv`

Check the `cache_hit_rate` field for each request in `request.csv`. Calculate the overall cache hit rate distribution, including the average, P50, and P90. A generally low hit rate indicates that `PrefixCache` is not working effectively.

Sort requests by arrival time and observe changes in `cache_hit_rate` over time. Significant fluctuations in the hit rate may indicate frequent cache eviction caused by insufficient cache capacity. Compare requests with the same `recv_token_size` (input length). If requests with similar input lengths have significantly different `first_token_latency(ms)` values, and requests with lower latency have higher `cache_hit_rate`, it can be confirmed that the `PrefixCache` hit rate is a key factor affecting TTFT.

### Step 4: Analyzing Cache Allocation Patterns in `kvcache.csv`

Observe changes in `blocks_allocated` and `blocks_freed` over time in `kvcache.csv`. If `blocks_allocated` is close to the number of tokens in the request's full prefix each time a new request arrives, rather than only the incremental portion, this indicates a cache miss and full reallocation of the prefix.

Analyze the trends of `total_blocks` and `free_blocks`. If `total_blocks` is sufficiently large but `free_blocks` fluctuates significantly, this indicates an issue with the cache management strategy, possibly because of an overly aggressive LRU eviction strategy or insufficient cache capacity.

### Step 5: Analyzing Prefill Scheduling in `batch.csv`

Filter the rows where `batch_type` is `prefill` and check `prefill_scheduled_tokens` and `during_time(ms)`. If the number of tokens scheduled for prefill is close to the full input length of the request rather than the incremental portion, this indicates that the prefix is not being reused from the cache. Compare the prefill duration of requests with the same input length at different times. If the duration varies significantly and correlates with the cache hit rate, the cache hit rate can be identified as a key factor.

### Step 6: Obtaining Request-Dimension Statistics Using a Multidimensional Analysis Tool

```bash
msserviceprofiler analyze --input-path=/path/to/input
```

Check the P50/P90/P99 percentiles of `first_token_latency(ms)` and the distribution of `input_token_num` in `request_summary.csv` to determine whether TTFT has a long tail.

## Root Causes

Common root causes of `PrefixCache` misses include:

1. **Insufficient number of common prefix tokens across requests to fill a block**: Ascend-vLLM prefix caching is based on the block mechanism of PagedAttention. The KV cache of a common prefix is reused only when the number of common prefix tokens across requests is greater than or equal to the block size. If the common prefix is short, such as a short system prompt, cache reuse cannot be triggered.

2. **Frequent eviction due to insufficient cache capacity**: The total number of KV cache blocks (`total_blocks`) is insufficient, or a large number of concurrent requests quickly fills the cache. When a new request arrives, the LRU policy evicts prefix caches that could otherwise be reused, preventing subsequent requests with the same prefix from achieving cache hits.

3. **Prefix caching is not properly enabled**: Although Ascend-vLLM enables prefix caching by default, configuring `ascend_scheduler_config` at the same time causes a conflict and disables prefix caching. In addition, prefix caching is currently not supported for multimodal models.

4. **Request prefix hash collisions or invalid cache indexing**: In distributed or multi-instance deployments, caches are isolated between instances. When requests are routed to different instances, they cannot reuse caches from other instances. In prefill/decode (PD) disaggregation scenarios, caches on prefill and decode nodes are managed independently, and cross-node cache sharing is not enabled.

5. **Model limitations**: Currently, only the Qwen2.5 and Qwen3 series support prefix caching. The feature may not work with other models.

6. **Interaction between chunked prefill and prefix caching**: When prefix caching is enabled, chunked prefill is also enabled. Chunked prefill may cause cache block boundaries to be misaligned with request prefix boundaries, affecting cache hits.

## Solutions

- Insufficient common prefix length: Ensure that the system prompt or conversation history is longer than the block size, or adjust the block size configuration.
- Insufficient cache capacity: Increase the configured total number of KV cache blocks or reduce concurrency on a single instance.
- Prefix caching not enabled: Check whether `ascend_scheduler_config` is configured. If there is a conflict, remove it and confirm that the model is on the supported list.
- Distributed cache isolation: Enable cross-node KV cache pooling in distributed scenarios, such as an HIXL + MemFabric solution, to implement cache sharing.
- Chunked prefill interaction: Adjust the chunk size for chunked prefill to align with the block size.

After applying the changes, check whether `cache_hit_rate` increases, TTFT decreases, and `blocks_allocated` changes from full allocation to incremental allocation.

## Summary of the Troubleshooting Methodology

For this scenario, first use ms-service-metric to observe KV cache utilization fluctuations and TTFT trends. Then use `msServiceProfiler` to collect Request and KVCache domain data and directly confirm the cache hit rate using the `cache_hit_rate` field in `request.csv`. Next, analyze cache allocation and release patterns in `kvcache.csv` to determine whether the issue is caused by insufficient capacity or unmet cache hit conditions. Finally, use the number of tokens scheduled for prefill in `batch.csv` to confirm whether the prefix is being recomputed.

The key diagnostic logic is as follows: If `cache_hit_rate` is low and `blocks_allocated` is close to the number of tokens in the full prefix, a `PrefixCache` miss has occurred. Further determine the cause as follows: If `free_blocks` remains close to 0 for an extended period, the cache capacity is insufficient. If `free_blocks` is sufficient but the hit rate remains low, check whether the common prefix length meets the block size requirement, whether prefix caching is properly enabled, and whether the model supports the feature.

## Suggestions for Improving the Tools

### `ms-service-metric`

Currently, `ms-service-metric` can display KV cache utilization and TTFT metrics. It is recommended to add a `PrefixCache` hit rate metric that directly displays the `cache_hit_rate` time series for real-time monitoring of cache effectiveness. It is also recommended to add `PrefixCache` miss alerts to automatically provide risk notifications when the hit rate remains low. In addition, it is recommended to add statistics on the common prefix length distribution to help users determine whether request prefixes meet the block size requirement.

### `msServiceProfiler`

Currently, `msServiceProfiler` can use the `cache_hit_rate` field in `request.csv` to confirm the cache hit rate. It is recommended to add `cache_hit_blocks` and `cache_miss_blocks` fields to `request.csv` to distinguish between the number of hit and missed blocks for more precise analysis of cache effectiveness. It is also recommended to add a cache eviction reason field to `kvcache.csv`, such as LRU eviction, release upon request completion, or manual invalidation, to help identify the specific causes of cache misses. In addition, it is recommended to add a `PrefixCache` diagnostic report to the parsing results to automatically check whether the common prefix length meets the block size requirement and whether common configuration issues, such as a conflict with `ascend_scheduler_config`, exist.
