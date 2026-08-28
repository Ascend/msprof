# CPU Cache Miss Resource Contention and Constraints

## Background

A customer service model is deployed and running on two architectures: Ascend + Kunpeng (A+K) and Ascend + x86 (A+X). Under equivalent service workloads, model parameters, and hardware specifications, the overall service performance of the A+K platform is significantly worse than that of the A+X platform. Core throughput and latency metrics fail to meet the requirements. To identify the root cause of the performance difference, end-to-end performance decomposition, scheduling trace analysis, and a dedicated investigation of underlying CPU anomalies were conducted.

## Symptoms

Compared with the A+X platform, the A+K platform falls behind across all core service performance metrics. In particular, time per output token (TPOT) and time to first token (TTFT) degrade significantly. Overall inference throughput is low, and request latency fluctuates considerably, failing to meet the performance requirements for service deployment. Preliminary investigation found no operator errors, NPU computation anomalies, or communication link errors, indicating that the overall performance bottleneck is caused by host-side CPU scheduling or system-level anomalies.

## Troubleshooting Process

This investigation followed a step-by-step approach to narrow down the root cause: "performance decomposition → cluster rank comparison → slow-rank identification → idle-period tracing → kernel scheduling analysis → PMU hardware metric verification → solution validation." The complete troubleshooting process is as follows:

1. **Decompose profile data from both platforms to identify the communication bottleneck.**  
Collect full profile data for the model on both the A+K and A+X platforms. Perform a layered comparison of computation time, communication time, and CPU idle time. The results show that communication accounts for 40% of the execution time on the A+X platform, compared with as much as **51%** on the A+K platform. The communication overhead is significantly higher on the A+K platform, indicating a significant cross-NPU communication bottleneck and serving as the primary symptom of the performance degradation.
2. **Visualize cluster data to identify the slow-rank node.**  
Import multi-rank profile data from the A+K cluster into MindStudio Insight and compare the execution time of communication operators across all ranks. The analysis shows an imbalance in execution time among ranks: one rank has significantly lower communication time, while most other ranks have significantly higher communication time. The cluster exhibits a clear "fast-ranks-wait-for-slow-ranks" scheduling pattern. All normal ranks must wait for the abnormal rank to complete its communication tasks before proceeding to the next stage. This identifies the abnormal rank as the slow rank in the cluster.
3. **Perform in-depth analysis of the slow rank to identify operator-dispatch idle periods.**  
Decompose the operator execution path of the identified slow rank segment by segment and compare it with the execution characteristics of normal ranks. The slow rank has long operator dispatch durations and a discontinuous dispatch path, with numerous CPU idle periods between consecutive operator dispatches. No effective tasks are scheduled or executed during these periods, resulting in substantial wasted time slices. This directly blocks the overall pipeline and causes communication tasks to accumulate, further increasing cluster waiting time.
4. **Collect kernel ftrace data to investigate the root cause of CPU thread anomalies.**
To identify the underlying cause of CPU idle periods and operator dispatch stalls, use the official **ftrace collection tool** to collect kernel scheduling data. For details about the tool, see [ftrace_tools](https://gitcode.com/Ascend/msinsight/tree/26.1.0/scripts/ftrace_tools).
Collect CPU scheduling, thread switching, and interrupt events throughout model execution to analyze system behavior during idle periods.  
For the observed stalls during idle periods, investigate four potential causes: (1) the thread is preempted and blocked by hard or soft interrupts; (2) the thread experiences lock contention or waits while holding a lock; (3) `CPU Cache Miss` is abnormally high; and (4) a service thread executes an unusually long-running branch of logic.  
Cross-check the ftrace scheduling logs against the service runtime logs. After ruling out interrupt preemption and lock contention, as well as unusually long-running branches in the service code, `CPU Cache Miss` is identified as the primary suspected root cause.
5. **Collect CPU PMU data, including metrics such as `Cache Miss`.**  
To verify the suspected cache miss anomaly, use the performance monitoring tool to collect CPU PMU hardware performance metrics. For details about the tool, see [Function Monitor](../../../misc/function_monitor/README_EN.md).  
Precisely correlate the anomalous periods of operator dispatch idle time and CPU stalls with the corresponding time periods. The `CPU Cache Miss` metric rises sharply during these periods, showing a high degree of correlation. It is confirmed that frequent `CPU Cache Miss` events significantly reduce CPU instruction execution efficiency and cause thread scheduling stalls. These issues, in turn, create a large number of idle periods.
6. **Use CPU core binding to reduce cache misses.**  
The cache miss anomaly is essentially caused by multiple processes contending for CPU resources and frequent context switches, which result in cache invalidation. Perform CPU affinity binding based on the host CPU core binding optimization script. For details about the tool, see [CPU Core Binding Script](../../../misc/host_analyzer/README_EN.md). By binding service processes to designated CPU cores, contention from unrelated processes is isolated and frequent CPU scheduling switches are reduced. This effectively avoids cache invalidation and resolves the high cache miss issue at the underlying layer.

## 4. Root Cause

1. **Direct cause**: The A+K deployment environment does not isolate CPU resources. Service processes contend with other system processes for CPU cores, causing frequent CPU context switches, significantly degrading the `CPU Cache Miss` hit rate and substantially reducing CPU instruction execution efficiency.
2. **Bottleneck propagation**: CPU scheduling stalls delay model operator dispatch and create numerous idle periods in the dispatch path, increasing the execution time of operator dispatch and communication tasks on individual ranks.
3. **Cluster-level degradation**: The slow rank on a single node becomes the bottleneck for the cluster, triggering a cluster-wide blocking effect in which **multiple ranks wait for a single rank**. As a result, the proportion of communication time increases sharply. Core performance metrics such as TPOT and TTFT consequently degrade, falling below the performance level of the A+X platform.

## Methodology Summary

For issues involving A+K architecture performance degradation, imbalanced execution time across cluster ranks, high communication overhead, and CPU idle periods and stalls, a standardized troubleshooting process can quickly narrow down the root cause.

1. **Baseline comparison**: Establish equivalent environments on both platforms with the same workload and model. Perform a layered decomposition of profile data (computation, communication, and idle time) to quickly identify the performance bottleneck area.
2. **Slow-rank identification**: Compare multi-rank data horizontally using MindStudio Insight to identify differences between fast and slow ranks and determine whether a single-node bottleneck is dragging down overall performance.
3. **Application-layer execution path analysis**: Decompose the operator dispatch timeline of the slow node and investigate application-layer anomalies such as dispatch delays, task idle periods, and pipeline stalls.
4. **Kernel-layer scheduling tracing**: Use ftrace to collect kernel scheduling, thread, and interrupt events and progressively rule out interference from interrupts, lock contention, abnormal code execution, and other factors.
5. **Hardware metric verification**: Collect PMU performance metrics to precisely identify underlying hardware bottlenecks such as cache miss and CPU scheduling issues.
6. **Targeted optimization validation**: Implement optimizations such as CPU core binding and resource isolation to address CPU contention and cache invalidation, and verify performance recovery.

## Suggestions for Tool Improvements

Based on this end-to-end troubleshooting experience, the following improvements to the existing operations and analysis toolchain are recommended. These improvements can increase the efficiency of troubleshooting similar issues.

1. **Add automatic anomaly detection to the ftrace tool**: The current tool supports only raw scheduling data collection, requiring manual analysis of thread, interrupt, and context-switch anomalies. Add automatic detection and statistics for CPU idle periods, frequent context switches, and thread blocking, and directly output the anomalous periods and threads to reduce the manual analysis effort.
2. **Add scenario-based correlation analysis to the function_monitor tool**: Automatically correlate PMU `Cache Miss` metrics with model operator dispatch and communication tasks along the timeline, enabling automatic matching of "hardware anomalies" with "service execution time" without manually comparing time periods.
3. **Add pre-check and recommendation capabilities to the CPU core binding tool**: Add CPU utilization heatmap analysis for service processes to automatically recommend optimal CPU core binding ranges. Also add comparative statistics for cache misses and scheduling time before and after CPU core binding to clearly demonstrate the optimization benefits.
4. **Integrate the toolchain**: Integrate the data formats of ftrace, function_monitor, and profiling tools, and support quick import of multi-source data into MindStudio Insight for joint visualization, enabling unified analysis of application-layer, kernel-layer, and hardware-layer data.
