# Troubleshooting Guide: Framework Scheduling and Dispatch Synchronization

## Scenario Description

In foundation model inference service scenarios, the framework typically needs to perform request queuing, batch scheduling, host-side task dispatch, device-side execution, and cross-node or cross-device synchronization. If task dispatch timing differs across nodes or devices, issues such as performance discrepancies among devices, abnormal synchronization wait duration, first-token latency fluctuations, and reduced throughput may occur.

Using prefill/decode (PD) disaggregation and multi-node concurrent inference as an example, this document describes how to use `msServiceProfiler` to collect scheduling-path data, identify framework scheduling and dispatch synchronization issues, and select optimization solutions based on consistency requirements.

## Symptoms

**Typical Symptoms**

- The model execution start time differs significantly across nodes. Some nodes have already started device execution, while others are still in host-side dispatch or scheduling wait.
- Within the same iteration, the durations of `BatchSchedule`, `forward`, or custom spans differ significantly across devices, resulting in performance discrepancies among devices.
- Long gaps exist before and after synchronization points, with more than 90 ms of synchronization wait in the end-to-end latency.
- Significant P90/P99 fluctuations are observed in `Request_Latency_curve` or `First_Token_Latency_curve`, while average throughput does not meet expectations.
- Under high concurrency, critical scheduling threads are preempted by non-critical threads, increasing the wait duration of the main thread.

**Impact**

| Item | Symptom |
| --- | --- |
| First-token latency | Scheduling wait increases during the prefill stage, resulting in higher first-token P90/P99 latency. |
| Decode throughput | Some faster ranks wait for slower ranks, and decode steps are prolonged by synchronization points. |
| Resource utilization | Device execution gaps increase, resulting in lower AI Core utilization. |
| Stability | Tail latency increases with traffic fluctuations, and cross-node synchronization is more likely to be delayed by slower nodes. |

## Data Collection

### Function Description

Use `msServiceProfiler` to collect `Span`, `Event`, `Metric`, and `Link` data for key stages, including framework scheduling, host-side dispatch, device execution, and synchronization wait. Combine these data with `BatchSchedule.csv`, `forward.csv`, and `Request_Latency_curve` to determine whether scheduling and dispatch are synchronized.

### Precautions

- Reproduce the issue in a stress-testing environment before enabling data collection to avoid affecting online services due to an excessive collection scope.
- In multi-node scenarios, synchronize the system time across nodes before data collection. Otherwise, timeline alignment may be inaccurate.
- To analyze the host-to-device dispatch duration, enable the ACL task duration-related collection options in the configuration. Note that this capability may introduce additional performance overhead.
- If the dynamic collection capability of msProf is already enabled, do not enable collection options that conflict with it.

### Configuration Example

Create `ms_service_profiler_config.json` and enable service profile data collection.

```json
{
    "enable": 1,
    "prof_dir": "${HOME}/.ms_server_profiler",
    "profiler_level": "INFO",
    "acl_task_time": 1,
    "acl_prof_task_time_level": "L0"
}
```

Set the environment variable and start the service.

```bash
export SERVICE_PROF_CONFIG_PATH=/path/to/ms_service_profiler_config.json
```

### Instrumentation Recommendations

Add span collection to key stages in the framework scheduling path.

```C++
auto scheduleSpan = PROF(INFO, SpanStart("FrameworkSchedule"));

// Request dequeue, batch construction, strategy selection, and other scheduling logic

PROF(scheduleSpan.SpanEnd());

auto hostSubmitSpan = PROF(INFO, SpanStart("HostSubmit"));

// Host-side task dispatch, stream binding, communication task submission, and other logic

PROF(hostSubmitSpan.SpanEnd());

auto deviceExecSpan = PROF(INFO, SpanStart("DeviceExecute"));

// Device-side computation or waiting for execution to complete

PROF(deviceExecSpan.SpanEnd());
```

Add `Event` and `Metric` collection for synchronization points and queue backlog.

```C++
PROF(INFO, Event("BeforeGlobalSync"));
PROF(INFO, Metric("dispatchQueueSize", queueSize).MetricScope("scheduler", rankId).Launch());
PROF(INFO, Metric("syncWaitMs", syncWaitMs).MetricScope("rank", rankId).Launch());
PROF(INFO, Event("AfterGlobalSync"));
```

For cross-thread or cross-module tracing, use `Link` to associate request IDs and avoid capturing only partial execution durations.

```C++
PROF(INFO, Link("request_in_scheduler", "request_in_executor"));
```

## Troubleshooting Method

1. Observe the start time and durations of the same batch across different processes and devices in `BatchSchedule.csv`.
   - If the start time differs significantly, first investigate scheduling threads, host-side dispatch queues, cross-node clock synchronization, and load-balancing strategies.
   - If the start time is close but the end time differs significantly, first investigate device execution, uneven expert loads, communication wait, and differences in operator duration.

2. Compare the `FrameworkSchedule`, `HostSubmit`, and `DeviceExecute` spans.
   - **Long `FrameworkSchedule` duration:** indicates a bottleneck in framework scheduling, which may be caused by synchronization queues, serialized transactions, lock contention, or batch processing strategies.
   - **Long `HostSubmit` duration:** indicates insufficient host-side dispatch capability, which may be caused by single-threaded submission, stream management blocking, or serialized communication task submission.
   - **A gap before `DeviceExecute`:** indicates that task scheduling has completed but the task has not entered device execution in a timely manner. Check the device queue, synchronization points, and upstream dispatch timing.

3. Check the `syncWaitMs` and `dispatchQueueSize` metrics.
   - If `syncWaitMs` remains high on some ranks, this usually indicates that faster ranks are waiting for slower ranks.
   - If `dispatchQueueSize` periodically builds up, this usually indicates a mismatch between the production rate of scheduling threads and the consumption rate of execution threads.
   - If the queue size is not high but the wait duration is high, cross-node synchronization, communication, or strong-consistency barriers are usually the bottleneck.

4. Combine `Request_Latency_curve` and `First_Token_Latency_curve` to determine the user-side impact.
   - If the average latency changes little but P99 increases significantly, the issue mainly affects tail latency.
   - If both the average latency and P99 increase, the scheduling and dispatch path has become an overall bottleneck.

## Cause Analysis and Solutions

### Distributed Scheduling Differences Causing Performance Discrepancies Among Devices

**Cause**

In PD disaggregation or multi-node deployment scenarios, model execution start time varies across nodes. Faster nodes reach synchronization points earlier and wait for slower nodes, resulting in significant synchronization wait.

**Solution**

- Adjust the scheduling strategy to use unified host-side dispatch and allow devices to execute tasks from their local queues, reducing differences in execution start time across nodes.
- Add time-window constraints to cross-node request allocation to avoid splitting the same batch across nodes with significantly different states.
- Monitor the `HostSubmit` start time by rank and trigger alerts or a degradation strategy when the difference exceeds a threshold.
- Add lightweight alignment logic before strong synchronization stages to prevent individual nodes from entering the wait state too early.

### Scheduling Serialization Causing Bottlenecks

**Cause**

The synchronized scheduling mode requires each partition or round of tasks to be completed strictly in sequence. If a partition contains thousands of tasks, the scheduling thread must process queues, transactions, locks, and state updates serially, amplifying dispatch latency.

**Solution**

- If the workload permits weak consistency, change synchronized scheduling to asynchronous scheduling to reduce unnecessary blocking.
- If strong consistency is required, increase the number of scheduling partitions, M, and use additional scheduling resources to reduce queue wait duration within each partition.
- Split scheduling state into request-level, batch-level, and device-level state to reduce the granularity of global locks.
- Move duration statistics, log persistence, and low-priority state updates out of the critical path.

### Asynchronous Scheduling Lacking Backpressure Control

**Cause**

Asynchronous scheduling can reduce blocking, but without queue limits, priority control, and result collection mechanisms, it may cause task accumulation, increased out-of-order execution, or degraded tail latency.

**Solution**

- Use a `Future` mechanism or dual-thread architecture to decouple the schedule thread from the generator thread.
- The schedule thread handles batch decisions and task submission, while the generator thread handles result consumption, post-processing, and triggering of the next round.
- Use a thread-safe queue to transfer tasks between threads, and use events, condition variables, or lightweight signals to control wake-up.
- Set queue high-water marks and timeout policies to prevent asynchronous tasks from accumulating indefinitely.

The example workflow is as follows:

```text
Request queuing -> Schedule thread constructs batches -> HostSubmit asynchronously dispatches tasks -> Generator thread consumes results -> Trigger next decode round
```

### Mismatched Pipeline Scheduling Strategy

**Cause**

Synchronous pipelines provide better memory efficiency but can easily be delayed by slower stages. Asynchronous pipelines provide higher statistical efficiency but may introduce weight-version differences or discrepancy correction costs.

**Solution**

- For scenarios with high device memory pressure, prioritize synchronous pipelines and optimize memory scheduling through dynamic programming.
- For training or generation scenarios where throughput is prioritized and some statistical differences are acceptable, use asynchronous pipelines together with learning rate scheduling, discrepancy correction, or inter-stage buffering.
- For complex dependency graphs, use BSP superstep synchronization to control global consistency and use asynchronous execution within each superstep to improve resource utilization.

### Improper Thread QoS Resource Allocation

**Cause**

Under high concurrency, critical scheduling threads, host-side dispatch threads, or result collection threads may be preempted by logging, monitoring, or low-priority post-processing threads, increasing the wait duration of the main thread.

**Solution**

- Increase the QoS level of the schedule thread, HostSubmit thread, and communication submission thread.
- Lower the priority of non-critical threads such as logging, periodic monitoring, and statistical aggregation.
- Bind critical threads to stable CPU cores to reduce thread migration.
- Use Metric collection to monitor the queue length, wake-up interval, and synchronization wait duration of critical threads and evaluate the benefits of QoS adjustments.

## Optimization Verification

After optimization, collect profile data again under the same stress-testing traffic and compare the following metrics.

| Metric | Expected Result |
| --- | --- |
| `FrameworkSchedule` duration | Average duration decreases, and P99 becomes more stable. |
| HostSubmit start time difference | The difference in start time across ranks decreases. |
| `syncWaitMs` | Synchronization wait decreases significantly, and the performance discrepancy between faster and slower ranks is reduced. |
| `Request_Latency_curve` | End-to-end P90/P99 latency decreases. |
| `First_Token_Latency_curve` | First-token tail latency decreases. |
| Device utilization | Execution gaps decrease and utilization increases. |

If synchronization wait remains high after optimization, continue investigating the following items.

- Check whether the system time is synchronized across nodes.
- Check whether network communication duration increases around synchronization points.
- Check whether any devices have abnormal operator durations or uneven expert loads.
- Check whether the number of scheduling partitions (`M`) is insufficient.
- Check whether backpressure occurs in asynchronous queues or results are not collected in a timely manner.

## Recommended Strategies

| Workload Requirement | Recommended Solution |
| -------------------- | ------------------- |
| Weak consistency is acceptable | Prioritize asynchronous scheduling and use a Future mechanism or dual-thread architecture to reduce synchronization on the critical path. |
| Strong consistency is required | Retain synchronization barriers, increase the number of scheduling partitions (`M`), and optimize unified host-side dispatch and device execution alignment. |
| Significant performance discrepancies among devices across nodes | Prioritize optimization of the distributed scheduling strategy and reduce differences in `HostSubmit` and `DeviceExecute` start time across ranks. |
| Complex dependency scenarios | Use BSP superstep synchronization to control consistency and combine work stealing or local asynchronous execution within each superstep. |
| Abnormal tail latency under high concurrency | Adjust the QoS of critical threads, split global locks, and add queue backpressure and priority controls. |

## Summary

The core issue with framework scheduling and dispatch synchronization is the conflict between the strictness of synchronization mechanisms and performance requirements. During troubleshooting, first use `msServiceProfiler` to break down the scheduling path into stages such as `FrameworkSchedule`, `HostSubmit`, `DeviceExecute`, and `SyncWait`, and then combine `BatchSchedule`, `Request_Latency`, and `First_Token_Latency` to identify the bottleneck. During optimization, select a solution based on service consistency requirements: prioritize asynchronous scheduling in weak-consistency scenarios, while in strong-consistency scenarios, prioritize optimizing distributed dispatch, increasing scheduling partitions, and reducing differences in execution start time across ranks.
