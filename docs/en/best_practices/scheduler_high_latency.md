# Excessive Scheduler Execution Time

## Background

In inference service scenarios, the scheduler is responsible for grouping incoming inference requests into batches according to specific policies and dispatching them to the NPU for execution. When the scheduler itself accounts for an excessive proportion of the total execution time, the NPU may remain idle while waiting, resulting in lower overall throughput and longer request queueing time. This is especially significant in Host Bound scenarios, such as small-model inference and the decode stage, where the host dispatch time of a single operator may exceed its device execution time, making scheduling overhead a system bottleneck. Ascend CANN provides graph-mode scheduling and model sinking technologies to optimize this issue. However, improper scheduling policy configuration, dynamic scheduling priority switching, and complex batch formation logic may still result in excessive scheduler execution time in actual deployments.

## Source

Inference

## Symptoms

Users typically first observe that the overall inference service throughput is lower than expected, NPU utilization is relatively low, and the number of waiting requests continues to increase. After breaking down the scheduling metrics in Grafana, common symptoms include:

- `waiting_batch_size` remains high, while the number of running requests (`batch_size`) does not reach the configured `maxBatchSize`.
- A large number of requests remain in the waiting state for an extended period in `request_status.csv`, while the number of requests in the running state fluctuates significantly.
- Intermittent idle periods appear on the NPU execution stream, and significant idle gaps (bubbles) exist between consecutive Schedule spans in the **Timeline** view.

After data is collected using msServiceProfiler, `batch.csv` shows that the `during_time(ms)` of entries whose `name` is `batchFrameworkProcessing` (batch formation stage) is significantly higher than that of `modelExec` (model execution stage). A typical symptom is that batch formation accounts for an excessively high proportion of the total batch processing time, whereas this proportion is typically low under normal conditions. This is particularly noticeable for Host Bound models, such as small-parameter-count encoder models or small-batch scenarios in the decode stage.

## Troubleshooting Process

### Step 1: Confirming Whether a Scheduling Bottleneck Exists

In Grafana, check scheduling-related metrics such as `batch_size`, `waiting_batch_size`, `num_running_reqs`, and `num_waiting_reqs`. If `waiting_batch_size` continues to increase while `batch_size` does not reach its upper limit, the scheduler is not grouping and dispatching waiting requests in a timely manner. Also check NPU utilization. If NPU utilization is low while the waiting queue is long, this might indicate a scheduling bottleneck.

### Step 2: Collecting Scheduling Data Using the Profiler

Configure `ms_service_profiler_config.json`, set `domain` to `"Schedule;Request;ModelExecute"`, and enable data collection. After collection is complete, run the following parsing command:

```bash
python3 -m ms_service_profiler.parse --input-path ${PATH}/prof_dir/
```

The parsing process generates files such as `batch.csv`, `request.csv`, `forward.csv`, and `chrome_tracing.json`.

### Step 3: Analyzing Scheduling Time in `batch.csv`

In `batch.csv`, filter the rows whose `name` is `batchFrameworkProcessing` and calculate the distribution of `during_time(ms)`, including the average, P90, and P99. Then filter the rows whose `name` is `modelExec` and calculate the ratio of batch formation time to model execution time. An excessively high proportion of batch formation time indicates a bottleneck in the scheduling stage.

Calculate scheduling time separately for each DP domain based on `dp_rank` to determine whether scheduling is slow in a specific DP domain. Calculate the statistics separately for each `batch_type` (prefill/decode) to determine whether the bottleneck occurs in prefill scheduling or decode scheduling.

### Step 4: Performing Fine-grained Analysis of the Batch Execution Stage Using the Inference Service Breakdown Tool

```bash
msserviceprofiler split --input-path /path/to/input --decode-batch-size 1 --decode-number 100
```

After the breakdown, check the execution time distribution of each substage in `decode.csv`, such as data dispatch, model execution, and data reception, to identify the specific substage responsible for the scheduling time bottleneck.

### Step 5: Confirming the Bubble Location in the Timeline

Open `chrome_tracing.json` and observe the timing relationship between the Schedule and ModelExecute spans in the **Timeline** view. If a large number of bubbles (idle gaps) exist and these bubbles occur during the batch formation stage rather than the model execution stage, the scheduler can be identified as the bottleneck.

Combine this analysis with the `bubble_time(ms)` field in `forward.csv` to calculate the bubble time between forward executions. If bubble time accounts for a high proportion of the total time, the NPU is waiting because scheduling and dispatch are not being performed in a timely manner.

### Step 6: Obtaining Overall Statistics Using the Multidimensional Analysis Tool

```bash
msserviceprofiler analyze --input-path=/path/to/input
```

Check the batch count and execution time statistics for prefill and decode in `batch_summary.csv` to evaluate scheduling efficiency from the perspective of the overall service.

## Root Cause

Common causes of excessive scheduler execution time include:

1. **Host Bound scenarios**: For small models or individual operators in the decode stage, device execution time may be extremely short, while the host dispatch process for each operator takes relatively long. This causes the device to frequently remain idle while waiting. This is an inherent issue with host scheduling on Ascend NPUs and can be addressed using graph-mode scheduling or model sinking provided by CANN.

2. **Improper scheduling policy configuration**: A small `maxBatchSize` causes frequent batch formation. Dynamic scheduling priority switching introduces additional policy switching overhead under single-concurrency workloads. An unreasonable ratio between `maxPrefillBatchSize` and `maxBatchSize` causes prefill and decode scheduling to block each other.

3. **Excessive batch formation complexity**: When the number of requests is large and request lengths vary significantly, the scheduler needs to traverse a large number of candidate requests to match and form batches, causing the complexity of the matching algorithm to increase with the number of requests.

4. **Framework adaptation issues**: The scheduler is not sufficiently optimized for the Task dispatch mechanism of Ascend NPUs, and graph-mode scheduling or model sinking is not enabled. Instead, individual operators are still dispatched one by one.

5. **Resource contention**: The scheduler thread competes for CPU resources with other service threads, resulting in scheduling latency fluctuations.

## Solutions

- Host Bound: Enable CANN graph-mode scheduling or model sinking to reduce host dispatch overhead.
- Improper scheduling policy: Adjust the ratio between `maxBatchSize` and `maxPrefillBatchSize`, and disable unnecessary dynamic scheduling priority switching.
- Complex batch formation logic: Optimize the batch formation algorithm and reduce the range of candidate requests that need to be traversed.
- Framework adaptation: Ensure that graph-mode scheduling or model sinking is enabled to avoid individual-operator dispatch.
- Resource contention: Ensure that the scheduler thread has sufficient CPU resources and avoid contention with other service threads.

After applying the changes, check whether `waiting_batch_size` decreases, `batch_size` approaches the upper limit, the proportion of batch formation time decreases, and NPU utilization increases.

## Summary of the Troubleshooting Methodology

The complete troubleshooting path for this scenario is as follows: first, use ms-service-metric to verify that `waiting_batch_size` is high, `batch_size` has not reached its upper limit, and NPU utilization is low; then use msServiceProfiler to collect data from the Schedule and ModelExecute domains and compare batch formation time with model execution time using `batch.csv`; next, use the inference service breakdown tool to perform a fine-grained breakdown of the execution time of each batch substage; finally, use the **Timeline** view to confirm the location and duration of bubbles.

The core decision logic is as follows: if batch formation accounts for a high proportion of the total time and a large number of scheduling gaps exist in the timeline, the bottleneck is in the scheduler; if model execution accounts for a high proportion of the total time, the bottleneck is on the model side.

## Suggestions for Improving the Tools

### `ms-service-metric`

Currently, `ms-service-metric` can display scheduling metrics such as `waiting_batch_size` and `batch_size`. It is recommended to add scheduling efficiency metrics, such as the ratio of scheduling time to model execution time and average batch formation latency, to facilitate rapid identification of scheduling bottlenecks during online monitoring. A Host Bound risk alert should also be added. When low NPU utilization is continuously detected for small models or during the decode stage, the system can automatically indicate a potential Host Bound issue.

### `msServiceProfiler`

Currently, `msServiceProfiler` can compare batch formation time with model execution time using `batch.csv`. It is recommended to add scheduling substage breakdown fields to `batch.csv`, such as request matching time, token allocation time, and Task dispatch time, to facilitate direct identification of the specific substage responsible for the scheduling bottleneck. A scheduling efficiency panel can also be added to the **Timeline** view to automatically calculate and display key metrics such as the proportion of batch formation time and bubble time.

### Inference Service Breakdown Tool

The current breakdown tool requires `batch_size` or `rid` to be specified manually. It is recommended to add the capability to automatically identify batches with abnormal execution time and automatically select the Top-N batches with the longest execution time for breakdown analysis.
