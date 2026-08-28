# Troubleshooting Guide: Asynchronous Scheduling Does Not Take Effect

## Scenario Description

Asynchronous scheduling (also known as asynchronous dual dispatch) aims to maximize the overlap of CPU-side scheduling, host-side task dispatch, NPU-side model execution, and communication during model inference serving, thereby reducing synchronous waits and scheduling gaps. This capability typically requires the environment variables, pipeline optimization, startup parameters, hardware and software versions, and communication configuration to satisfy the required conditions. If the configuration is incomplete or there are conflicts, the service may start normally, but asynchronous scheduling may not be enabled. As a result, there is no significant improvement in throughput, decode latency may not decrease, or CPU/NPU pipeline stages may fail to overlap.

Using a case where asynchronous scheduling does not take effect, this document describes how to use `msServiceProfiler` to collect `BatchSchedule`, `ModelExecute`, and `Communication` data, determine whether asynchronous scheduling takes effect, and troubleshoot issues related to environment variables, pipeline optimization, startup parameters, compatibility, and configuration conflicts.

## Symptoms

**Typical Symptoms**

- After environment variables related to asynchronous scheduling are set, there is no significant improvement in throughput, Time to First Token, or decode latency.
- In the timeline, `BatchSchedule`, host-side dispatch, and `ModelExecute` still appear strictly serial, with no overlap between scheduling and execution.
- The CPU-side scheduling thread waits for NPU execution to complete before submitting the next round of tasks, resulting in obvious pipeline gaps.
- `Decode_Generate_Speed_Latency_curve` does not decrease, and P90/P99 values of `Request_Latency_curve` remain high.
- In single-node prefill/decode (PD) disaggregation or multi-card scenarios, communication waits are not effectively overlapped, and communication duration in the `Communication` domain remains exposed on the critical path.
- The service startup logs do not show that asynchronous scheduling has been enabled, or that the startup parameters or environment variables have been recognized by the framework.

**Impact**

| Item | Symptom |
| --- | --- |
| Throughput | CPU-side scheduling and NPU execution cannot overlap, resulting in limited improvement in tokens/s. |
| Decode latency | The decode stage remains affected by synchronous dispatch and communication waits. |
| End-to-end latency | Request latency P90/P99 is difficult to reduce. |
| Resource utilization | Gaps exist between NPU executions, resulting in discontinuous CPU/NPU pipelines. |
| PD disaggregation performance | Insufficient overlap between KV cache transfer and compute-communication fusion increases cross-stage waits. |

## Data Collection

### Function Description

Uses `msServiceProfiler` to collect data for the `BatchSchedule`, `ModelExecute`, `Communication`, and `Request` domains. It also uses the timeline, `span_info`, `batch.csv`, `forward.csv`, `pd_split_communication.csv`, and visualization curves to help determine whether asynchronous scheduling takes effect.

### Precautions

- Before collection, record the service startup command, environment variables, MindIE version, CANN version, hardware model, and deployment configuration.
- You are advised to collect two sets of data, one with asynchronous scheduling disabled and the other with this feature enabled, and then compare them under the same stress testing conditions.
- If further analysis of host-to-device task dispatch duration is required, ACL task duration collection can be enabled, but the additional overhead should be evaluated.
- In multi-device or PD disaggregation scenarios, the `Communication` domain must also be collected. Otherwise, it cannot be determined whether communication waits are effectively overlapped with other operations through asynchronous scheduling.

### Configuration Example

Create `ms_service_profiler_config.json` to collect scheduling, execution, communication, and request data.

```json
{
    "enable": 1,
    "prof_dir": "${HOME}/.ms_server_profiler",
    "profiler_level": "INFO",
    "domain": "Request;BatchSchedule;ModelExecute;Communication",
    "acl_task_time": 1,
    "acl_prof_task_time_level": "L0"
}
```

Set the collection configuration path.

```bash
export SERVICE_PROF_CONFIG_PATH=/path/to/ms_service_profiler_config.json
```

Export span data during parsing to facilitate observation of whether `BatchSchedule` and `forward` overlap.

```bash
ms_service_profiler_parse --input-path ${PROF_DIR} --output-path ${OUTPUT_DIR} --span
```

## Determining Whether Asynchronous Scheduling Takes Effect

**Characteristics When Asynchronous Scheduling Takes Effect**

When asynchronous scheduling takes effect, you can typically observe the following results:

- In the timeline, the next `BatchSchedule` or host-side dispatch overlaps the previous `ModelExecute`.
- The CPU-side scheduling span no longer waits for NPU-side `forward` execution to complete before starting the next round.
- Decode execution becomes more continuous, and the intervals between adjacent decode executions in `forward.csv` are shortened.
- Communication duration in the `Communication` domain is partially overlapped and is no longer fully exposed on the critical path.
- Observable improvements are seen in `Decode_Generate_Speed_Latency_curve`, `Request_Latency_curve`, or throughput metrics.

**Characteristics When Asynchronous Scheduling Does Not Take Effect**

If asynchronous scheduling does not take effect, the following symptoms are typically observed:

- In `BatchSchedule.csv`, scheduling always starts after the previous `forward` execution ends.
- Obvious gaps exist between adjacent executions in `forward.csv`.
- Communication duration and ModelExecute are arranged serially and do not overlap with the computation stage.
- The `Batch_Size_curve` is similar with and without asynchronous scheduling, but throughput and decode latency remain almost unchanged.
- The service logs do not show that asynchronous scheduling or the pipeline queue has been enabled, or that the relevant startup parameters have taken effect.

## Troubleshooting Methods

1. Check that environment variables are set before the service starts.
   - Verify that `MINDIE_ASYNC_SCHEDULING_ENABLE=1` is configured in the startup script.
   - Verify that the variables are set before the service process starts, rather than being set temporarily in an interactive terminal after startup.
   - In container deployments, verify that the environment variables are passed into the container and can be read by the service process.

2. Check that pipeline optimization is also enabled.
   - Asynchronous scheduling typically requires `TASK_QUEUE_ENABLE=2` to be configured as well.
   - If only asynchronous scheduling is enabled without pipeline optimization, CPU/NPU overlap may not reach the expected level.
   - Use the timeline to observe whether host-side dispatch and device-side execution form a continuous pipeline.

3. Check that startup parameters are explicitly enabled.
   - Some frameworks rely not only on environment variables but also require asynchronous scheduling parameters in the startup command.
   - In vLLM-Ascend scenarios, confirm whether `--async-scheduling` is passed.
   - If the framework startup script encapsulates parameters, check the actual startup command that takes effect.

4. Check hardware and software versions.
   - Verify that the hardware model supports the asynchronous scheduling feature, such as whether the target Ascend device model is within the supported range.
   - Verify that the CANN, MindIE, and inference framework versions support the current asynchronous scheduling capability.
   - For earlier versions with known compatibility issues, you are advised to upgrade the version and verify that asynchronous scheduling takes effect.

## Root Cause Analysis and Solutions

### Environment Variables Not Correctly Configured

**Cause**

If the core switch for asynchronous scheduling is not set, is set to an incorrect value, is set after the service starts, or is not inherited by the container or startup script, asynchronous scheduling will not take effect.

**Solution**

Configure the environment variables before the service starts.

```bash
export MINDIE_ASYNC_SCHEDULING_ENABLE=1
```

In container deployments, add the variables to the container startup command or service startup script, and confirm in the service logs that the variables have been read. When using `systemd`, Kubernetes, or platform-based orchestration, check the environment variables that reach the service process, not just those in the current shell.

### Pipeline Optimization Not Enabled

**Cause**

Asynchronous scheduling needs to work together with the pipeline queue capability to overlap task dispatch, scheduling, and execution. If `TASK_QUEUE_ENABLE` is not set to the recommended value, task submission may still be synchronous and serial.

**Solution**

Enable pipeline optimization before startup.

```bash
export TASK_QUEUE_ENABLE=2
```

Recollect the timeline after enabling pipeline optimization. If `BatchSchedule` and `ModelExecute` are still completely serial, continue to check the startup parameters, versions, and conflicting configurations.

### Startup Parameters Not Enabled

**Cause**

Some frameworks require asynchronous scheduling to be explicitly enabled in the startup command. When only environment variables are configured, the framework layer may not enter the asynchronous scheduling branch.

**Solution**

Check the service startup command. For frameworks that require explicit parameters, add the asynchronous scheduling parameter when starting the service.

```bash
--async-scheduling
```

If a script wraps the startup command, verify that the parameter is not overwritten or filtered out by the configuration template. You are advised to print the final startup parameters in the logs for future review.

### Hardware or Software Version Incompatibility

**Cause**

The asynchronous scheduling capability depends on hardware, CANN, MindIE, and framework versions. Earlier versions may not support this capability or may have compatibility issues related to asynchronous scheduling, communication, or memory management.

**Solution**

- Verify that the hardware model supports asynchronous scheduling, such as whether the target environment uses an Ascend device model that supports this feature.
- Verify that the CANN version, MindIE version, and inference framework version meet the requirements for asynchronous scheduling.
- Upgrade to a stable version that supports asynchronous scheduling and verify that asynchronous scheduling takes effect.
- Collect `msServiceProfiler` data before and after the upgrade, and compare decode latency and timeline overlap under the same stress testing conditions.

### Communication or Memory Configuration Conflicts

**Cause**

Some communication- or memory-related environment variables can change the execution graph, communication path, or task submission method, which may prevent asynchronous scheduling from being enabled or cause its benefits to be offset by synchronous communication waits.

**Solution**

Check whether the following configurations conflict with the current asynchronous scheduling scenario.

```bash
unset HCCL_OP_EXPANSION_MODE
unset ATB_LLM_HCCL_ENABLE
```

In single-node PD disaggregation scenarios, you are advised to enable compute-communication fusion.

```bash
export ATB_LLM_LCOC_ENABLE=1
```

In multi-device scenarios, ensure that the HCCL/LCCL configuration is correct to prevent issues such as communication initialization, rank configuration, or network problems that may prevent the benefits of asynchronous scheduling from being realized.

### Lack of Collaborative Features Such as Prefix Cache and LCCL

**Cause**

Asynchronous scheduling can only reduce the wait time between scheduling and execution. If prefill computation is repeated, KV cache transfer is slow, or the communication library is not optimized, overall performance may still show little improvement.

**Solution**

- In single-node PD disaggregation scenarios, use prefix cache to reduce repeated prefill computation.
- Enable the LCCL communication library to reduce KV cache transfer and decode communication overhead.
- Collect data from the `Communication` domain to determine whether communication duration is hidden by the computation stage.
- If communication is still exposed on the critical path, prioritize optimizing the communication configuration and then evaluate the benefits of asynchronous scheduling.

## Instrumentation Recommendations

If the framework allows custom instrumentation, add spans and events at key stages of asynchronous scheduling to help verify whether asynchronous task submission and completion are occurring.

```C++
auto submitSpan = PROF(INFO, SpanStart("AsyncSubmit"));

// Asynchronously submit the next round of tasks on the CPU side

PROF(submitSpan.SpanEnd());

PROF(INFO, Event("AsyncTaskQueued"));

auto waitSpan = PROF(INFO, SpanStart("AsyncWait"));

// Wait for asynchronous tasks to complete or results to be collected

PROF(waitSpan.SpanEnd());
```

Collect queue depth and asynchronous dispatch counts.

```C++
PROF(INFO, Metric("asyncQueueDepth", asyncQueueDepth).MetricScope("scheduler", rankId).Launch());
PROF(INFO, MetricInc("asyncDispatchCount", 1).MetricScope("rank", rankId).Launch());
```

If `asyncDispatchCount` remains 0 for an extended period, the asynchronous branch is not being entered. If `asyncQueueDepth` remains 0, tasks are not forming an effective pipeline.

## Optimization Validation

You are advised to validate in the following order, changing only one variable at a time.

| Step | Validation Content | Metrics to Observe |
| --- | --- | --- |
| Baseline collection | Disable asynchronous scheduling and collect synchronous execution data. | Timeline, `BatchSchedule.csv`, `forward.csv` |
| Enable asynchronous variable | Configure `MINDIE_ASYNC_SCHEDULING_ENABLE=1`. | Whether the asynchronous branch is entered, decode latency |
| Enable pipeline optimization | Configure `TASK_QUEUE_ENABLE=2`. | Whether `BatchSchedule` and `ModelExecute` overlap |
| Add startup parameters | Configure `--async-scheduling`. | Framework logs, timeline overlap |
| Eliminate conflicting configurations | Remove conflicting communication or memory variables. | Communication duration, request latency |
| Collaborative optimization | Enable LCCL, prefix cache, or compute-communication fusion. | Decode latency, throughput, PD communication duration |

When the optimization is effective, the following results are typically observed:

- `BatchSchedule` and `ModelExecute` overlap in the timeline.
- The intervals between adjacent decode executions in `forward.csv` are shortened.
- Duration in the `Communication` domain is partially overlapped.
- `Decode_Generate_Speed_Latency` decreases.
- P90/P99 of `Request_Latency` decrease.
- Throughput improves, and gaps in NPU execution are reduced.

## Recommended Handling Strategies

| Issue Type | Recommended Solution |
| --- | --- |
| Asynchronous switch does not take effect | Set `MINDIE_ASYNC_SCHEDULING_ENABLE=1` before startup and verify that the environment variable is readable by the service process. |
| Switch enabled but no benefit | Also set `TASK_QUEUE_ENABLE=2` and check whether overlap appears in the timeline. |
| Framework not entering asynchronous branch | Explicitly configure `--async-scheduling` in the startup command and check the final startup parameters. |
| Compatibility issues with older versions | Upgrade CANN, MindIE, or the inference framework to a stable version that supports asynchronous scheduling. |
| Communication configuration offsetting benefits | Check the HCCL/LCCL configuration, remove conflicting variables, and enable compute-communication fusion if necessary. |
| Insufficient benefit in single-node PD disaggregation | Use in combination with `ATB_LLM_LCOC_ENABLE=1`, prefix cache, and the LCCL communication library. |
| Multi-device scenario still serial | Check rank configuration, network communication, communication domain duration, and cross-device synchronization points. |

## Summary

Asynchronous scheduling failing to take effect is not caused by a single switch but by a combination of environment variables, pipeline optimization, framework startup parameters, hardware and software versions, and communication configuration. When troubleshooting, first collect `BatchSchedule`, `ModelExecute`, and `Communication` data using `msServiceProfiler` to determine whether scheduling, execution, and communication overlap in the timeline. During optimization, first verify that `MINDIE_ASYNC_SCHEDULING_ENABLE=1`, `TASK_QUEUE_ENABLE=2`, and the framework startup parameters are effective. Then, check version compatibility, conflicting environment variables, HCCL/LCCL communication configuration, compute-communication fusion in PD disaggregation, prefix cache, and other collaborative capabilities.
