# DP Load Imbalance

## Background

In data parallelism (DP) scenarios, a service instance contains multiple DP domains. Under normal circumstances, requests and token loads should be distributed as evenly as possible across the DP domains. If a DP domain continuously handles more requests or heavier requests, it will first experience queueing, higher KV cache utilization, or longer execution durations, slowing down the entire instance.

## Source

Inference

## Symptoms

Users typically first observe that the overall instance throughput is lower than expected and P95/P99 latency is higher, but instance-level metrics alone may not explain the cause. After further breaking down the metrics by `dp`, common symptoms include:

- The number of running/waiting requests in some DP domains remains higher than that in other DP domains for an extended period.
- Some DP domains have higher prompt/generation token throughput or larger batch sizes.
- Hotspot DP domains have higher KV cache utilization, and `free_kvcache_blocks` decreases faster.
- Non-hotspot DP domains still have available capacity, but overall latency has been increased by the hotspot DP domains.

## Troubleshooting Process

### Step 1: Verifying Whether Only Some DP Domains Are Busy

In the DP-dimension load panel in Grafana, compare the number of requests, prompt token throughput, generation token throughput, batch size, and number of waiting requests across `dp` or `engine` dimensions within the same time window.

If the metrics diverge only for a short period and latency does not change significantly, the difference may be considered a traffic fluctuation. If some DP domains remain higher across multiple consecutive windows, proceed to the next step.

### Step 2: Verifying Whether the Imbalance Is Caused by More Requests or Heavier Requests

View the number of requests, prompt token throughput, and generation token throughput in Grafana together to determine the source of the load of the hotspot DP domain:

- **More requests and more tokens:** This usually indicates that the hotspot DP domain is handling more traffic. Investigate the scheduling strategy, DP allocation strategy within the instance, or request stickiness.
- **Similar number of requests but more prompt/generation tokens:** This usually indicates that the hotspot DP domain is handling heavier requests. Check request logs, load-testing datasets, or request length statistics to determine whether long inputs or long outputs are concentrated in this DP domain.
- **Similar numbers of requests and tokens but longer execution duration in one DP domain:** Check device monitoring, process logs, and Profiler data to determine whether the process, device, or communication corresponding to that DP domain is abnormal.

The findings from this step directly determines the subsequent handling approach: scheduling skew requires changes to the allocation strategy; request-weight skew requires balancing by token count or request length; and device anomalies require checking the hardware or process status first.

### Step 3: Verifying Whether DP Imbalance Has Affected the Service

Differences in DP curves alone are insufficient to determine that a fault exists. Check whether the hotspot DP domain also exhibits the following:

- An increase in the number of waiting requests or queue wait duration.
- High KV cache utilization and fewer free blocks.
- Longer batch execution duration or decode duration than other DP domains.
- An increase in overall instance P95/P99 latency during the same period.

If these signals occur simultaneously, DP load imbalance may be a bottleneck.

### Step 4: Using the Offline Profiler to Identify Specific Scheduling Differences

After collecting `Schedule`-, `Request`-, and `KVCache`-related data, aggregate the data by `dp_rank`:

- In `batch.csv`, compare `batch_size`, `prefill_batch_size`, `decode_batch_size`, `prefill_scheduled_tokens`, `decode_scheduled_tokens`, `total_scheduled_tokens`, and `during_time(ms)` across DP domains.
- In `request.csv`, count the number of requests by DP domain and compare input length, output length, `queue_wait_time(ms)`, and `first_token_latency(ms)`.
- In `kvcache.csv`, compare `used_blocks`, `free_blocks`, and `kvcache_usage_rate` across DP domains.

If a DP domain continuously schedules more tokens per batch and has longer execution durations, while also having higher request wait time and KV cache utilization, this indicates that the hotspot is caused by skewed DP scheduling load.

## Root Cause

The number of requests, token load, or execution capability differs across DP domains. Common root causes include scheduling strategies that do not balance load by token count, concentration of long requests in some DP domains, abnormal DP process or device status, inconsistent configurations across DP domains, or request stickiness that causes traffic to remain concentrated in a small number of DP domains.

## Resolution

- **Uneven request distribution:** Adjust the DP scheduling strategy to avoid concentrating requests in some DP domains through fixed-order or fixed-stickiness scheduling.
- **Uneven token load:** Balance requests based on input/output token counts, or schedule long requests separately to avoid balancing solely by request count.
- **Concentrated KV cache pressure:** Reduce the number of requests scheduled to hotspot DP domains, or adjust the scheduling strategy to bring KV cache utilization across DP domains closer together.
- **Abnormal device or process:** Compare device utilization, error logs, and Profiler execution durations between hotspot and other DP domains, and restore abnormal DP domains to normal operation first.
- **Inconsistent configurations:** Check the service startup parameters and deployment configuration to ensure that the model, parallelism parameters, and device memory configurations are consistent across all DP domains.

After the issue is addressed, observe token throughput, number of waiting requests, KV cache utilization, and P95/P99 latency again by DP domain to determine whether the metrics are normal.

## Methodology Summary

For DP load imbalance scenarios, first use ms-service-metric to compare the number of requests, prompt/generation tokens, waiting requests, batch size, and KV cache utilization across DP domains to determine whether persistent hotspot DP domains exist. After confirming persistent divergence in the online metrics, use msServiceProfiler to aggregate `request.csv`, `batch.csv`, and `kvcache.csv` by `dp_rank` to distinguish between uneven request distribution, uneven token load, concentrated KV cache pressure, and device or process anomalies.

## Suggestions for Tool Improvements

### `ms-service-metric`

Currently, `ms-service-metric` can compare request volume, token volume, queues, and KV cache utilization across DP domains. It is recommended to add DP load imbalance alerts that automatically identify hotspot DP domains when the differences across DP domains continue to increase while overall instance P95/P99 latency also increases.

### `msServiceProfiler`

Currently, `msServiceProfiler` can aggregate and analyze scheduling differences by `dp_rank` using `request.csv`, `batch.csv`, and `kvcache.csv`. It is recommended to directly output comparisons of the number of requests, input/output tokens, batch scheduled tokens, execution duration, and KV cache utilization across DP domains in offline reports.
