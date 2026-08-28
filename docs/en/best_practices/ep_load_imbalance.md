# EP Load Imbalance

## Background

During mixture of expert (MoE) model inference, expert parallelism (EP) distributes experts across different ranks or devices. After requests are routed, if a small number of experts are continuously selected, the corresponding ranks will handle more operations such as `GroupedMatmul` computation and `dispatch` or `combine` communication. As a result, these ranks have longer execution durations, consequently increasing the execution duration of the entire MoE layer.

## Source

Inference

## Symptoms

Users typically first observe that the throughput of MoE models such as DeepSeek is lower than expected, while decode duration or end-to-end latency increases. Further observation of MoE-related metrics may reveal the following:

- Some expert hotspots are significantly higher than those of other experts in the same layer.
- Some ranks consistently handle higher expert loads across multiple layers.
- Hotspots are not significantly mitigated after the EPLB update.
- The execution duration of the MoE layer increases, accompanied by longer execution durations on some ranks/devices.

## Troubleshooting Process

### Step 1: Verifying Whether the Performance Issue Occurs During an MoE-Related Stage

First, verify whether the decrease in throughput or increase in latency observed by the user occurs simultaneously with an increase in MoE layer execution duration or decode duration. If overall performance remains stable and expert hotspots fluctuate only briefly, the issue should generally not be directly attributed to an EP problem.

If decode duration increases while MoE operators or the MoE communication stage also become slower, continue to examine the expert load.

### Step 2: Verifying Whether Persistent Expert Hotspots Exist

In the EPLB or expert hotspot panel in Grafana, view the aggregated hotspot metrics to verify whether a small number of experts remain consistently hotter:

- Whether `eplb:expert_hotness:current_max` remains significantly higher than `current_mean`.
- Whether `eplb:expert_hotness:imbalance` remains high.
- Whether the hotspots occur only briefly or persist across multiple windows.

The purpose here is simply to determine whether persistent imbalance exists. If only transient spikes occur and throughput and latency do not deteriorate simultaneously, the issue can be monitored as business input fluctuation.

### Step 3: Identifying the Specific Hotspot Layers, Ranks, and Experts

After verifying that persistent imbalance exists, continue to identify the hotspots in the per-layer expert details panel:

- Identify the layer in which an expert has significantly more selections or higher hotness.
- Check whether the hotspot experts are concentrated on the same rank or a small number of ranks.
- Check whether the same rank handles higher hotspot loads across multiple layers.

If the hotspots are distributed across multiple ranks, their impact may be limited. If the hotspots are concentrated on a small number of ranks, they are more likely to cause those ranks to become performance bottlenecks.

### Step 4: Determining Whether EPLB Is Effective

Use the hotspot distribution before and after the EPLB update, together with the EPLB configuration and service startup parameters in Grafana, to determine whether load balancing is effective:

- If `update_max / update_mean` and `imbalance` decrease after the update, EPLB generally has mitigated the hotspots.
- If the hotspots remain concentrated on the same group of experts or the same ranks after the update, further investigate the EPLB configuration, expert mapping configuration, and request input distribution.
- If the EPLB update itself takes excessive time, assess whether the update overhead negates the load-balancing benefits.

This step determines whether to adjust EPLB parameters or investigate the expert mapping configuration, business input distribution, or routing strategy.

### Step 5: Using the Offline Profiler to Verify Whether the Hotspots Have Caused Some Ranks to Run More Slowly

Online metrics can indicate which experts are hot, but `msServiceProfiler` should also be used to collect MoE-related operator and communication data to verify whether the hotspots have already slowed execution:

- Compare the `GroupedMatmul` execution durations across ranks/devices to verify whether hotspot ranks have longer computation durations.
- Check the execution durations of `MoeDistributeDispatch` and `MoeDistributeCombine` to verify whether communication overhead is amplified by the hotspots.
- Check the trace to determine whether a small number of ranks have longer execution durations in the MoE layer while other ranks wait for synchronization.

Only when hotspot experts, hotspot ranks, and slow operators/communication occur within the same time window can the root cause be narrowed down to EP load imbalance.

## Root Cause

The expert load is unevenly distributed across EP, causing a small number of experts or ranks to continuously handle higher request loads and further making them performance bottlenecks for MoE computation or communication. Common root causes include concentrated business input distributions, EPLB being disabled or improperly configured, uneven expert mapping, concentration of hotspot experts, and abnormal execution performance of individual ranks/devices.

## Resolution

- **EPLB is disabled or ineffective:** Enable EPLB and adjust the update interval, window length, migration threshold, or expert remapping strategy.
- **Hotspot experts are concentrated on a small number of ranks:** Adjust expert placement or mapping to distribute hotspot experts across more ranks.
- **Input distribution causes routing concentration:** Analyze business request types based on request logs or load-testing datasets and, if necessary, segment traffic or isolate input distributions.
- **Amplified communication overhead:** Check `dispatch` or `combine` communication duration and optimize the parallelism configuration or communication path to prevent hotspot ranks from simultaneously handling higher computation and communication loads.
- **Individual ranks are abnormally slow:** First investigate device status, process load, and operator execution issues to avoid misidentifying device problems as routing imbalance.

After the issue is addressed, observe whether the hotspot distribution is significantly mitigated, the execution durations of MoE operators/communications decrease, and decode latency and throughput recover.

## Methodology Summary

For EP load imbalance scenarios, first use `ms-service-metric` to monitor MoE-related execution durations, expert hotspot distribution, and changes in hotspot distribution before and after EPLB updates to verify whether persistent hotspot experts or ranks exist. After verifying the hotspots, use `msServiceProfiler` to collect MoE operator and communication data and compare the execution durations of operations such as `GroupedMatmul`, `dispatch`, and `combine` to determine whether the hotspots have actually created performance bottlenecks, avoiding misidentification of short-term input fluctuations or individual device anomalies as EP load imbalance.

## Suggestions for Tool Improvements

### `ms-service-metric`

Currently, `ms-service-metric` can display hotspot distributions before and after EPLB updates and expert hotspot metrics. It is recommended to add top hotspot lists by layer, rank, and expert dimensions to directly show whether hotspot experts are concentrated on a small number of ranks.

### `msServiceProfiler`

Currently, `msServiceProfiler` can use MoE operator and communication durations to verify whether hotspots have created performance bottlenecks. It is recommended to add a correlation view linking "hotspot expert → rank/device → MoE operator execution duration" to offline reports.
