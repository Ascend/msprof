# Insufficient KV Blocks

## Background

KV blocks are used to hold the KV cache for requests. After a request enters the prefill or decode stage, the scheduler continuously needs to allocate KV blocks. If there are insufficient available KV blocks, a request may be queued, preempted, or recomputed because it cannot obtain the required KV cache resources, even when compute resources are still available.

## Source

Inference

## Symptoms

Users typically observe performance degradation rather than a direct indication of "insufficient KV blocks":

- As test concurrency continues to increase, throughput stops increasing, while first-token latency and end-to-end latency increase significantly.
- The service has a relatively large number of waiting/pending requests, and some requests remain unable to enter the decode stage for an extended period.
- NPU utilization does not necessarily remain at full capacity, but request queue wait time continues to increase.
- Monitoring shows that KV cache utilization remains high for an extended period, `free_kvcache_blocks` remains close to 0, or metrics related to preemption or allocation failures continue to increase.

## Troubleshooting Process

### Step 1: Verifying Whether the Issue Is Caused by Increased Queue Wait Time

In the Grafana service overview or request latency dashboard, check the latency breakdown and first determine which stage accounts for the latency increase:

- Whether first-token latency has increased.
- Whether request queue/waiting time has increased.
- Whether the number of running requests has not increased significantly while waiting/pending requests continue to accumulate.

If the latency increase occurs primarily during model execution and NPU utilization remains high, investigate compute bottlenecks first. If the latency increase occurs primarily during the waiting stage, continue the investigation in the scheduling and KV cache areas.

### Step 2: Verifying Whether the Waiting Is Caused by Insufficient KV Blocks

In the KV cache, request status, and scheduling dashboards in Grafana, focus on whether the following phenomena occur simultaneously within the same time window:

- `free_kvcache_blocks` remains low for an extended period, while KV cache utilization approaches the upper limit.
- Waiting/pending requests, queue wait time, or first-token latency increases simultaneously.
- Metrics related to preemption, recomputation, or allocation failures increase during the same period.

If these three signals are aligned, the requests are likely bottlenecked by KV cache capacity rather than being compute-bound.

### Step 3: Identifying the Source of Capacity Pressure

The KV cache usage only indicates that the instance is approaching its capacity limit. To determine whether the pressure is caused by concurrency, request length, traffic distribution, or KV cache configuration, analyze the test configuration, service startup parameters, request logs, and Grafana monitoring data together:

- Check the test configuration, service startup parameters, or change records to determine whether concurrency, `max_num_seqs`, `max_model_len`, maximum output length, or batch-related configuration has been increased.
- Check request logs, test datasets, or request-length statistics to determine whether requests with long contexts or long outputs are concentrated on the instance.
- Check instance-level monitoring in Grafana to determine whether only some instances are receiving more requests or tokens while other instances still have available capacity.
- Check service startup parameters, device memory configuration, and the total/free KV blocks in Grafana to determine whether the total number of available KV blocks is too small.

If test traffic is fixed but the proportion of long-input or long-output requests increases, the issue is usually that longer requests consume more blocks. If request lengths remain stable but concurrency increases, the concurrency on a single instance has exceeded the KV cache capacity available to that instance. If only some instances exhaust their blocks, uneven load distribution across instances should also be investigated.

### Step 4: Using Offline Profiler Data to Identify the Affected Requests and Batches

If online monitoring indicates insufficient KV blocks, use `msServiceProfiler` to collect `Schedule`, `KVCache`, and `Request` data. Focus on the following three files:

- `request.csv`: Check whether `queue_wait_time(ms)` and `first_token_latency(ms)` have increased significantly and identify the requests with the longest wait time.
- `batch.csv`: Check whether `used_blocks` and `kvcache_usage_rate` remain high and `free_blocks` remains low for an extended period across consecutive batches, and whether `decode_batch_size` is unable to increase steadily due to insufficient blocks.
- `kvcache.csv`: Check `blocks_allocated` and `blocks_freed`. If `blocks_allocated` continues to exceed `blocks_freed` across multiple time windows and `free_blocks` is exhausted, the root cause can be narrowed down to insufficient KV cache capacity.

The Profiler analysis should answer two questions: which requests consume the most blocks, and at which batches the scheduler begins queuing or preempting requests after the available blocks are exhausted.

## Root Causes

The number of available KV blocks is insufficient, preventing requests from promptly obtaining KV cache resources. Common root causes include excessive concurrency on a single instance, excessively long input or output sequences, insufficient device memory reserved for the KV cache, concentration of long requests on a small number of instances, or scheduling parameters that allow too many requests to enter the scheduler simultaneously.

## Solutions

Select an appropriate solution based on the troubleshooting results:

- **Excessive concurrency:** Reduce per-instance concurrency, `max_num_seqs`, or the ingress traffic throttling threshold to avoid admitting too many requests at once.
- **Requests are too long:** Limit the maximum input or output length, or route long-context requests to instances with greater KV cache capacity.
- **Insufficient total KV blocks on a single instance:** Adjust KV cache-related device memory configuration or increase the proportion of device memory available for the KV cache. If no device memory remains available on a single device, add instances, increase the number of devices, or use devices with greater memory capacity.
- **Uneven traffic distribution:** First address load imbalance across instances so that requests are distributed based on instance capacity and queue status, avoiding premature KV block exhaustion on a small number of instances.
- **Significant preemption/recomputation:** Reduce the number of requests entering the scheduler or adjust the scheduling policy to avoid frequently swapping out requests that have already entered the decode stage.

After the changes, recheck the same set of signals: whether `free_kvcache_blocks` has returned to a stable level, whether waiting/pending requests have decreased, whether first-token latency has decreased, and whether throughput resumes increasing with concurrency.

## Methodology Summary

For scenarios involving insufficient KV blocks, first use `ms-service-metric` to confirm whether the latency increase occurs primarily during queue waiting and the first-token stage, and observe whether KV cache utilization, `free_kvcache_blocks`, and waiting/pending requests become abnormal within the same time window. After online metrics indicate KV cache capacity pressure, use `msServiceProfiler` to collect `request.csv`, `batch.csv`, and `kvcache.csv` to determine whether the issue is caused by request length, concurrency, batch scheduling, or an imbalance between block allocation and release.

## Suggestions for Tool Improvements

### `ms-service-metric`

Currently, `ms-service-metric` can display KV cache utilization, `free_kvcache_blocks`, the number of waiting/pending requests, and first-token latency. Add KV block shortage diagnostic prompts to the existing dashboards and automatically correlate these metrics within the same time window to help determine whether queueing is caused by KV cache capacity pressure.

### `msServiceProfiler`

Currently, `msServiceProfiler` can use `request.csv`, `batch.csv`, and `kvcache.csv` to analyze request waiting, batch scheduling, and KV cache allocation/freeing. Add a KV block shortage summary to offline reports to automatically identify time windows in which `free_blocks` remains low, `decode_batch_size` cannot increase, or `blocks_allocated` continuously exceeds `blocks_freed`.
