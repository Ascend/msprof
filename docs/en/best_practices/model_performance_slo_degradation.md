# Model Performance Degradation Causing SLO Degradation

## Background

In service-based inference scenarios, service level objectives (SLOs) are core metrics for measuring service quality and typically include metrics such as time to first token (TTFT), tokens per second (TPS), request end-to-end latency (E2E latency), and throughput. When model execution performance degrades, SLO metrics deteriorate directly, affecting user experience and business SLAs. Model performance degradation can be caused by various factors, including decreased operator execution efficiency, ineffective model computation graph compilation and optimization, insufficient NPU memory causing swap, performance regressions introduced by model version upgrades, changes in operator performance caused by CANN version changes, compilation cache misses in dynamic-shape scenarios, and imbalanced expert loads in MoE models. On Ascend NPUs, model performance may degrade after migration from GPUs due to differences in operator adaptation, graph compilation strategies, memory management mechanisms, and other factors. A systematic troubleshooting approach is required to identify the cause layer by layer.

**Source**

Inference.

**Symptoms**

Users typically first observe an overall deterioration in the inference service SLO metrics: increased TTFT, decreased TPS, increased P95/P99 end-to-end latency, and decreased overall throughput. When the metrics are broken down in Grafana, the following symptoms are commonly observed:

- The P50, P90, and P99 values of the `first_token_latency` metric all increase.
- `generate_token_speed` (token generation speed) decreases.
- `request_latency` (request end-to-end latency) increases.
- `batch_size` may remain normal, but `during_time(ms)` (model execution time) increases.
- NPU utilization may remain normal or even increase, indicating that the NPU is busy executing inefficient operators.

In `batch.csv`, the `during_time(ms)` of rows where `name` is `modelExec` increases significantly, while `batch_size` and `total_scheduled_tokens` remain similar to their values before the degradation. This indicates that model execution takes longer for the same amount of computation. In `forward.csv`, forward execution time (`during_time(ms)`) increases, while bubble time (`bubble_time(ms)`) may remain normal, indicating that the bottleneck is in the model forward computation itself rather than scheduling. In `request.csv`, both `execution_time(ms)` and `first_token_latency(ms)` increase, while `queue_wait_time(ms)` may remain normal, indicating that the request wait time in the queue has not increased and that the bottleneck is in the execution stage.

Typical scenarios include performance regressions after a model version upgrade, changes in operator performance after a CANN version upgrade, performance below expectations after migrating a model from a GPU to an Ascend NPU, imbalanced expert loads in an MoE model causing some devices to become bottlenecks, and compilation cache misses in dynamic-shape scenarios causing repeated compilation.

## Troubleshooting Process

### Step 1: Confirming the Scope and Magnitude of SLO Degradation

In Grafana, view the time-series changes in SLO metrics such as `first_token_latency`, `generate_token_speed`, and `request_latency` to identify when the degradation started and its magnitude. Compare metrics such as `batch_size`, `total_scheduled_tokens`, and NPU utilization before and after the degradation to determine whether the degradation is related to a change in workload. If the workload remains unchanged while the SLO metrics deteriorate, the issue is likely caused by performance degradation on the model execution side.

### Step 2: Collecting Model Execution Data Using the Profiler

Configure `ms_service_profiler_config.json`, set `domain` to `"Schedule; ModelExecute; Request; KVCache"`, and enable data collection. If an operator-level performance issue is suspected, set `acl_task_time` to 1 or 2 to enable operator execution time collection. Note that enabling operator collection introduces additional performance overhead and is recommended when the model execution time is abnormal. After collection is complete, perform parsing:

```bash
python3 -m ms_service_profiler.parse --input-path ${PATH}/prof_dir/
```

The parsing generates files such as `batch.csv`, `forward.csv`, `request.csv`, and `chrome_tracing.json`.

### Step 3: Analyzing Model Execution Time in `batch.csv` and `forward.csv`

In `batch.csv`, filter the rows where `name` is `modelExec` and observe the trend of `during_time(ms)` over time. If the model execution time suddenly increases after a certain point in time, the cause may be a configuration change or an environmental change.

Group the data by `dp_rank` and calculate the model execution time for each DP domain. If the execution time of a specific DP domain is significantly higher than that of other DP domains, the corresponding device or process may be abnormal.

Group the data separately by `batch_type` (`prefill`/`decode`). If prefill execution time increases, input processing or attention computation may have slowed down. If decode execution time increases, token-by-token generation efficiency may have decreased.

In `forward.csv`, observe `during_time(ms)` and `bubble_time(ms)` by `dp_rank` and `forward_iter`. If `during_time(ms)` increases while `bubble_time(ms)` remains normal, the bottleneck is in model forward computation. If `bubble_time(ms)` also increases, the issue may be related to scheduling or communication.

### Step 4: Performing Fine-Grained Decomposition of the Model Execution Stage Using the Service-Based Decomposition Tool

```bash
msserviceprofiler split --input-path /path/to/input --prefill-batch-size 1 --prefill-number 50
msserviceprofiler split --input-path /path/to/input --decode-batch-size 1 --decode-number 50
```

Examine the time distribution of each sub-stage in `prefill.csv` and `decode.csv` to identify the specific bottleneck sub-stage in model execution, such as data delivery, operator execution, or data reception.

### Step 5: Locating Bottleneck Operators in the Timeline View

Open `chrome_tracing.json`, zoom in on the model execution stage in the Timeline view, and observe the execution time of each operator. If `acl_task_time` collection is enabled, operator-level execution timelines are available in the Timeline, allowing you to identify the operators with the longest execution time.

Compare the Timelines before and after the degradation, if historical data is available, and observe which operators have changed execution time. Use the service-based profile data comparison tool:

```bash
msserviceprofiler compare ./profiling_data/before ./profiling_data/after
```

Compare the differences between the profile data collected at the two time points.

### Step 6: Obtaining Overall Statistics Using the Multidimensional Analysis Tool

```bash
msserviceprofiler analyze --input-path=/path/to/input
```

In `batch_summary.csv`, view the P50, P90, and P99 percentiles of prefill and decode execution time to determine whether long-tail latency exists. In `request_summary.csv`, view the distributions of `first_token_latency(ms)` and `exec_time(ms)`. In `service_summary.csv`, view `generate_token_speed` and `generate_all_token_speed` to determine whether overall throughput has decreased.

### Step 7: Performing Operator-Level Performance Analysis Using msprof If an Operator-Level Issue Is Suspected

Set the `acl_task_time` parameter to 3 during collection to ensure that the profile data directory contains operator data files with the `_ascend_pt` suffix. After parsing is complete, use msprof to export operator data:

```bash
msprof --export=on --output=/path/to/output
```

Open the parsed profile data in MindStudio Insight. In the "Operator Duration" panel, view the top operators by execution time and identify the bottleneck operators.

### Step 8: Collecting `eplb_observe` Domain Data If Imbalanced Expert Loads Are Suspected in an MoE Model

Set `domain` to `"eplb_observe"` to collect expert hotspot information. After parsing, view the expert hotspot heatmap and load imbalance line chart to determine whether imbalanced expert loads are causing performance degradation.

## Root Causes

Common root causes of SLO degradation caused by model performance degradation include:

1. **Operator performance regression**: An upgrade to the CANN version changes the implementation of certain operators and causes performance degradation; a model version upgrade changes the computation graph and introduces inefficient operators; or changes in operator fusion strategies cause fusion to be ineffective.

2. **Insufficient model adaptation after migration from GPU to Ascend NPU**: The model contains a large number of operators such as `Pad` and `Strided_Slice` that have relatively low execution efficiency on Ascend due to array rearrangement; some operators are not supported on Ascend, causing the model to be split into multiple subgraphs and increasing data transfer time between subgraphs; or the model does not actually invoke the Ascend backend and automatically falls back to CPU execution.

3. **Compilation cache misses in dynamic-shape scenarios**: Different input shapes for each inference request cause graph compilation cache misses, triggering repeated compilation. Execution time increases significantly during the first inference or when the shape changes.

4. **NPU memory insufficiency causing swap**: Excessive KV cache usage or excessive model weight usage causes insufficient NPU memory, triggering request swap, which swaps the request's KV cache to CPU memory. Swap and recovery operations introduce significant latency.

5. **Imbalanced expert loads in MoE models**: In MoE models such as DeepSeek, different experts are invoked at different frequencies. Some devices hosting frequently invoked experts become computational hotspots, while other devices remain idle and wait, resulting in fast and slow ranks.

6. **Host Bound causing device idle time**: For small models or in the decode stage, the host cannot dispatch tasks fast enough to keep up with the device's execution speed, causing intermittent device idle time. CANN graph-mode scheduling or model sinking can be used to address this issue.

7. **Communication bottlenecks**: In multi-device inference scenarios, collective communication such as AllReduce/AllGather accounts for a high proportion of the execution time; in PD disaggregation scenarios, KV cache transfer latency is high; or in EP (Expert Parallelism) scenarios, Dispatch-Combine communication becomes a bottleneck.

8. **Improper model quantization or precision settings**: During inference with a quantized model, precision loss causes additional computation steps to be required, or improper quantization configurations cause some operators to fall back to higher-precision computation.

## Solutions

- **Operator performance regression**: Use AOE to automatically tune and optimize the model computation graph, or roll back to an earlier CANN version.
- **Insufficient adaptation after GPU migration**: Replace inefficient operators with Ascend-affinity operators and confirm that the model invokes the Ascend backend.
- **Compilation cache misses in dynamic-shape scenarios**: Fix the input shape or enable the compilation cache.
- **NPU memory insufficiency causing swap**: Adjust the KV cache configuration to avoid swap, reduce concurrency per instance, or increase available memory.
- **Imbalanced MoE expert loads**: Enable expert load balancing (EPLB) and adjust the expert placement strategy.
- **Host Bound**: Enable CANN graph-mode scheduling or model sinking.
- **Communication bottlenecks**: Optimize the communication strategy, such as using HIXL instead of HCCL, and adjust the parallelism configuration.
- **Quantization precision issues**: Adjust the quantization configuration and ensure that the quantization parameters are correct.

After the issue is addressed, check whether the SLO metrics have recovered, the model execution time has decreased, and NPU utilization is normal.

## Summary of the Troubleshooting Methodology

The complete troubleshooting flow for this scenario is as follows: First, use ms-service-metric to identify the specific SLO metrics that have degraded and quantify the magnitude of the degradation. Then, use msServiceProfiler to collect data from the ModelExecute domain and use `batch.csv` and `forward.csv` to determine whether model execution time has increased. Next, use the service-based decomposition tool to perform fine-grained analysis and locate the bottleneck sub-stage in model execution. Then, use the Timeline view and operator-level profiling to identify the specific bottleneck operators. Finally, apply targeted optimization based on the bottleneck type (operator, communication, scheduling, or memory).

Core decision logic:

- If `modelExec` `during_time(ms)` increases while `batch_size` remains unchanged → model execution efficiency has decreased.
- If `bubble_time(ms)` also increases in `forward.csv` → the issue may be related to scheduling or communication.
- If the execution time of a specific `dp_rank` is significantly higher → the corresponding device or process may be abnormal.
- If `queue_wait_time(ms)` remains normal but `execution_time(ms)` is high in `request.csv` → the bottleneck is in the execution stage rather than the queue.
- If the number of `swapped` requests in `request_status.csv` is greater than 0 → insufficient NPU memory has triggered swap.

## Suggestions for Improving the Tools

### `ms-service-metric`

Currently, `ms-service-metric` can display SLO metrics such as TTFT, TPS, and E2E latency. Add automatic SLO degradation alerting so that an alert is automatically generated when metrics such as TTFT, TPS, and E2E latency continuously exceed baseline thresholds. Add monitoring of the ratio of model execution time to scheduling time to facilitate rapid differentiation between execution and scheduling bottlenecks. In addition, add a swap request count metric and trigger an alert when swap occurs.

### `msServiceProfiler`

Currently, `msServiceProfiler` can analyze model execution time through `batch.csv` and `forward.csv`. Add fields for decomposing the model execution stage into sub-stages in `batch.csv`, such as "data delivery time," "operator execution time," and "data reception time," to facilitate direct identification of bottleneck sub-stages. Add automatic comparison with historical data and automatically suggest possible causes of performance degradation when a significant increase in model execution time is detected. In addition, add an SLO impact assessment panel to the parsing results to automatically quantify the impact of increased model execution time on SLO metrics such as TTFT and TPS.

### Service-Based Decomposition Tool

Currently, the decomposition tool requires `batch_size` or `rid` to be specified manually. Add automatic decomposition of the model execution stage without requiring manual parameter specification, and automatically select batches with abnormal execution time for decomposition. Also add comparison with baseline data to the decomposition results and indicate the magnitude of changes in the execution time of each sub-stage.

### Service-Based Expert Recommendation Tool

Currently, the expert recommendation tool can provide parameter tuning recommendations based on benchmark results. Add automatic parameter tuning recommendations based on performance degradation data. When an increase in model execution time is detected, automatically analyze whether the degradation can be mitigated by adjusting parameters such as `maxBatchSize` and `maxPrefillBatchSize`.
