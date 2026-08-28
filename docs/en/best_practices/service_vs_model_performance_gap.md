# Optimization Guide for Large Performance Gaps Between Serving and Pure-Model Inference

## Scenario Description

In foundation-model inference optimization, pure-model execution is typically used first to establish a performance baseline for a model with fixed input and output lengths and a fixed batch size. The model is then deployed in a serving framework to handle real-world requests. If throughput, time to first token, or decode latency in the serving scenario differs significantly from the pure-model baseline, common causes include misaligned service parameters, insufficient resource allocation, abnormal communication configuration, load imbalance, or an excessively large sequence length configuration.

Using a serving inference scenario with lower performance than the pure-model baseline as an example, this document describes how to use msServiceProfiler to collect serving inference pipeline data, identify the source of the performance gap, and provide optimization solutions for parameters, resources, communication, and load balancing.

## Symptoms

### Typical Symptoms

- Throughput is normal in pure-model testing. For example, with input/output lengths of 256/256 and `bs=128`, throughput can reach 1132.24 tokens/s, but throughput decreases significantly after deployment in the serving framework.
- In the serving scenario, `Batch_Size_curve` remains consistently below the configured `maxBatchSize` or `maxPrefillBatchSize` limit.
- `Prefill_Generate_Speed_Latency_curve` and `Decode_Generate_Speed_Latency_curve` are significantly worse than the pure-model baseline.
- P90/P99 latency in `First_Token_Latency_curve` or `Request_Latency_curve` is high and increases rapidly as concurrency increases.
- HCCL communication timeouts, rank connection failures, model loading hangs, or service process termination occur during startup.
- During multi-device deployment of MoE models, some devices have significantly higher execution times, and `moe_analysis` or expert hotspot diagrams indicate load imbalance.

### Impact

| Item | Symptom |
| --- | --- |
| Throughput | Serving inference throughput is significantly lower than the pure-model baseline. |
| Time to first token | Queueing, batch formation, or execution time increases during the prefill stage. |
| Decode latency | Per-step latency increases due to communication waits, lack of compute-communication fusion, or load imbalance. |
| Stability | Insufficient memory, insufficient permissions, or communication timeouts cause service startup failures. |
| Resource utilization | Device utilization decreases because batches are not fully utilized, KVCache is over-allocated, or hotspot experts are concentrated on a small number of devices. |

## Data Collection

### Function Description

Use msServiceProfiler to collect data related to `Request`, `BatchSchedule`, `ModelExecute`, `Communication`, `KVCache`, and expert load during serving inference. Compare the differences between the pure-model baseline and the serving inference pipeline using `batch.csv`, `request.csv`, `forward.csv`, `BatchSchedule.csv`, and visualization curves.

### Precautions

- Complete pure-model baseline testing before optimization to determine the maximum performance with fixed input/output lengths and batch size.
- Reuse the input/output lengths, concurrency level, and model configuration from the pure-model baseline as much as possible during serving stress testing to ensure that the baseline remains comparable.
- Before collecting data in multi-node, multi-device scenarios, verify that the system clocks are synchronized across nodes to avoid time offsets in the Timeline and load-balancing diagrams.
- If communication, task dispatch, or operator execution time needs to be analyzed, enable ACL task-time-related data collection as required, but evaluate the additional overhead.
- Configure the `eplb_observe` domain separately to collect expert hotspot information and avoid excessive data volume.

### Configuration Example

Create `ms_service_profiler_config.json` and collect the `Request`, `BatchSchedule`, `ModelExecute`, `Communication`, and `KVCache` domains as required.

```json
{
    "enable": 1,
    "prof_dir": "${HOME}/.ms_server_profiler",
    "profiler_level": "INFO",
    "domain": "Request;BatchSchedule;ModelExecute;Communication;KVCache",
    "acl_task_time": 1,
    "acl_prof_task_time_level": "L0"
}
```

To collect MoE expert hotspot information, enable the `eplb_observe` domain separately and configure the MindIE environment variables for expert hotspot collection.

```json
{
    "enable": 1,
    "prof_dir": "${HOME}/.ms_server_profiler",
    "profiler_level": "INFO",
    "domain": "eplb_observe"
}
```

Set the collection configuration path before starting the service.

```bash
export SERVICE_PROF_CONFIG_PATH=/path/to/ms_service_profiler_config.json
```

## Baseline Alignment

When serving performance is lower than the pure-model baseline, first verify that the two tests are comparable.

| Check Item | Description |
| --- | --- |
| Input/output lengths | The input and output lengths used for pure-model testing and serving stress testing should be the same, such as 256/256. |
| Batch size and concurrency | The pure-model `bs` should correspond to serving `maxBatchSize`, `maxPrefillBatchSize`, and `concurrency`. |
| Model weights | The model version, quantization method, parallelization strategy, and rank configuration should be the same. |
| Precision and operators | The precision, operator libraries, and graph optimization settings used for pure-model testing and serving inference should be the same. |
| Hardware resources | The number of NPUs, number of machines, CPU resources, and memory resources should be the same or otherwise comparable. |

If the baselines are not aligned, do not directly conclude that the serving framework is the bottleneck. Record the maximum pure-model performance first, and then enable optimization settings one by one on the serving side to verify the performance improvement from each change.

## Troubleshooting Method

1. Check `Batch_Size_curve` and `batch.csv`.
   - If `prefill_batch_size` or `decode_batch_size` remains consistently low, the serving batches are not fully utilized. Check `concurrency`, `maxPrefillBatchSize`, `maxBatchSize`, request distribution, and `supportSelectBatch`.
   - If the batch size is already close to its upper limit but throughput remains low, continue checking ModelExecute, Communication, and device utilization.

2. Check `Request_Status_curve`.
   - If the waiting queue continues to accumulate, the service entry point, batch formation, or execution stage may have insufficient request processing capacity.
   - If the running queue is small but throughput is low, the scheduling policy may be too conservative, or serving parameters may be limiting the effective batch size.

3. Check the prefill- and decode-related curves.
   - If `Prefill_Generate_Speed_Latency` is high, first check `maxPrefillBatchSize`, `maxSeqLen`, KVCache reservation, and input length distribution.
   - If `Decode_Generate_Speed_Latency` is high, first check communication configuration, LCCL, compute-communication fusion, MoE load balancing, and cross-device synchronization waits.

4. Check `Kvcache_usage_percent_curve`.
   - If KVCache utilization remains low while `maxSeqLen` is configured to a large value, excessive reservation may be limiting the batch size and concurrency.
   - If KVCache utilization is close to the upper limit, concurrency or sequence length has reached a memory bottleneck. Reduce `maxSeqLen` or increase resources as appropriate.

## Cause Analysis and Solutions

### Inconsistent Parameter Configuration Between Serving Inference and the Pure-Model Baseline

**Cause**

If parameters such as `maxPrefillBatchSize`, `maxBatchSize`, `concurrency`, and `prefillBatchSize` are not configured according to the actual workload, batches may not be fully utilized, queue scheduling may be overly conservative, or resources may be allocated improperly between prefill and decode, resulting in lower throughput than the pure-model baseline.

**Solution**

- Use the pure-model baseline as the target and establish a correspondence between `bs`, serving concurrency, and `maxBatchSize`.
- Adjust `maxPrefillBatchSize` and `prefillBatchSize` according to the request length distribution to avoid excessively small batches during prefill.
- Gradually increase `concurrency` according to the stress-test concurrency level and observe whether `Batch_Size_curve` approaches the configured upper limit.
- For throughput-oriented scenarios, enable `supportSelectBatch` so that the scheduler preferentially selects batch combinations that are more favorable for throughput.
- Record `Batch_Size_curve`, `Prefill_Generate_Speed_Latency`, and `Request_Latency` before and after optimization to avoid evaluating the result based only on a single throughput measurement.

### Resource Allocation and Insufficient Memory

**Cause**

Compared with pure-model execution, deployment in the serving framework typically requires additional framework processes, queues, KVCache, communication buffers, and monitoring components. Insufficient system memory may cause model loading to hang, processes to be terminated, or abnormally high latency for the first request.

**Solution**

- Before startup, verify that sufficient memory is available. It is recommended that `free_mem` be no less than `(weight size / number of machines) * 1.3`.
- In test environments, release the system cache before starting the service.

```bash
sync
echo 3 > /proc/sys/vm/drop_caches
```

- For containerized deployment, verify privileged mode and path permissions to avoid failures when accessing models, rank tables, shared memory, or device files.

```bash
docker run --privileged=true
```

- If KVCache utilization remains low while memory usage is high, check whether `maxSeqLen` is significantly greater than the maximum sequence length in the actual dataset.
- If the service process terminates, first check system logs, container memory limits, and peak memory usage during weight loading.

### Insufficient Communication and Environment Variable Configuration

**Cause**

Multi-node, multi-device deployment in the serving framework relies on HCCL or LCCL communication. If environment variables are inconsistent between the primary and secondary nodes, the rank table path is incorrect, the container IP is incorrectly configured, or the communication timeout is too short, service startup may fail, decode waits may increase, or cross-device synchronization may take excessively long.

**Solution**

Set the local IP address, rank table, and deterministic communication-related environment variables on the primary and secondary nodes as appropriate.

```bash
export MIES_CONTAINER_IP=<local IP address>
export RANKTABLEFILE=/path/to/rank_table.json
export HCCL_DETERMINISTIC=true
```

For large-scale deployments or environments with slow network initialization, increase the communication connection timeout as appropriate and verify that `WORLD_SIZE` matches the actual number of ranks.

```bash
export HCCL_CONNECT_TIMEOUT=7200
export WORLD_SIZE=32
```

If decode latency is high, consider enabling the LCCL communication library and compute-communication fusion.

```bash
export ATB_LLM_LCOC_ENABLE=1
```

During verification, focus on comparing Communication-domain spans, `Decode_Generate_Speed_Latency`, and P90/P99 of `Request_Latency`.

### MoE Load Imbalance

**Cause**

In MoE models, the access frequency of different experts may vary significantly. If hotspot experts are concentrated on a small number of devices or ranks, some devices may finish early and wait for slower devices, resulting in lower serving throughput than the ideal pure-model baseline.

**Solution**

- Use msServiceProfiler to collect the `eplb_observe` domain and observe expert hotspots and load imbalance curves.
- Use the `msit elb` tool to generate the expert deployment file `expert_map_file`.
- Configure expert load-balancing parameters in `config.json`. For example, set `"level": 1` to enable static load balancing.
- After adjustment, collect `moe_analysis.csv` and the expert load imbalance line chart again to verify that the execution time gap between devices has decreased.

### Excessive Sequence Length Configuration

**Cause**

If `maxSeqLen` is configured based on an extreme upper limit, such as 10,000, while the actual maximum length in the dataset is only 698, KVCache and memory reservations may be excessive. This can limit the available batch size and concurrency, thereby reducing serving throughput.

**Solution**

- Analyze the actual input and output length distribution in production or stress-test datasets, and use P99 or the actual maximum value as the basis for configuring `maxSeqLen`.
- Reduce `maxSeqLen` from an excessively conservative value to one closer to the actual workload. For example, reduce it from 10,000 to 698.
- After adjustment, observe `Kvcache_usage_percent_curve`, `Batch_Size_curve`, and service throughput to verify whether performance improves.
- If a small number of requests may contain extremely long sequences, route those requests separately or handle them with a degraded service level to avoid affecting the main service instances.

## Optimization Verification

Adjust only one aspect at a time and recollect data under the same stress-test conditions. The following verification sequence is recommended.

| Step | Verification Objective | Metrics to Observe |
| --- | --- | --- |
| Pure-model baseline | Determine the theoretical upper bound of model performance | tokens/s, bs, input/output lengths |
| Parameter optimization | Verify whether serving batches are fully utilized | `Batch_Size_curve`, `batch.csv` |
| Memory optimization | Verify whether resources are sufficient | `Kvcache_usage_percent`, process RSS, system `free_mem` |
| Communication optimization | Verify whether decode waits decrease | Communication spans, `Decode_Generate_Speed_Latency` |
| Load balancing | Verify whether the performance gap between fast and slow ranks decreases | `moe_analysis.csv`, expert hotspot diagrams |
| End-to-end verification | Verify user-side performance improvements | `First_Token_Latency`, `Request_Latency`, throughput |

When the optimization is effective, the following results are typically observed:

- `Batch_Size_curve` approaches the configured upper limit of `maxBatchSize` or `maxPrefillBatchSize`.
- Average token latency decreases during both prefill and decode.
- The average, P90, and P99 of `Request_Latency_curve` decrease.
- KVCache utilization better matches the actual workload, reducing memory waste.
- In MoE scenarios, the execution time gap between devices decreases, and the performance gap between fast and slow ranks is reduced.
- Serving throughput gradually approaches the pure-model baseline.

## Recommended Handling Strategies

| Problem Type | Recommended Solution |
| --- | --- |
| Serving batch size remains low | Adjust `concurrency`, `maxBatchSize`, and `maxPrefillBatchSize`, and enable `supportSelectBatch`. |
| High prefill latency | Adjust `prefillBatchSize` and `maxSeqLen`, and check KVCache reservation and input length distribution. |
| High decode latency | Optimize HCCL/LCCL configuration, enable compute-communication fusion, and check cross-device communication waits. |
| Startup failure or process termination | Check `free_mem`, container privileged mode, path permissions, and memory limits. |
| Multi-device load imbalance | Use `msit elb` to generate `expert_map_file` and enable static or dynamic expert load balancing. |
| Large gap from the pure-model baseline persists | Return to baseline alignment and verify that the model version, parallelization strategy, input/output lengths, and hardware resources are consistent. |

## Summary

The key to addressing large performance gaps between serving and pure-model inference lies in baseline alignment, parameter configuration, resource allocation, and communication optimization. During optimization, first use pure-model testing to establish the maximum performance baseline, and then use msServiceProfiler to break down the serving inference pipeline, focusing on `BatchSchedule`, `Request`, `KVCache`, `Communication`, and MoE load-balancing data. For throughput-oriented workloads, prioritize fully utilizing batches and enabling throughput-oriented scheduling policies. If decode tail latency is high, prioritize checking the communication library, compute-communication fusion, and expert load balancing. If resources are insufficient, resolve memory, container permission, and sequence length configuration issues before continuing with serving parameter optimization.
