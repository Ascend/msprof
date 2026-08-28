# IR Interrupt Preemption Analysis

## Background

Interrupts are a core asynchronous response mechanism in Linux systems. They can interrupt the process currently running on a CPU to handle urgent hardware events and are fundamental to responding to peripheral requests, interacting with devices, and scheduling tasks. Interrupt processing at a normal frequency does not affect stable service operation. However, if interrupts are triggered too frequently or take an excessive amount of time to process, they can continuously consume CPU time allocated to service tasks and disrupt execution continuity.  
In model inference and training scenarios on the Ascend platform, the host is responsible for operator queue management and task dispatch, while the NPU executes compute tasks. During service execution, underlying hardware drivers use dedicated interrupts to synchronize task status: the `sq_send_trigger_irq` interrupt is triggered when an operator is enqueued and dispatched, while the `cq_update_irq` interrupt is triggered when operator execution is complete and the queue status is updated. These two types of interrupts are handled by the system kernel. If interrupts are triggered too frequently or their CPU load is unevenly distributed, they can continuously interrupt core service threads responsible for operator dispatch and data transfers, causing dispatch stalls, pipeline bubbles, and task backlogs, degrading service performance across the entire system and cluster.

## Symptoms

The customer's service is deployed on the Ascend + Kunpeng (A+K) platform. Compared with the Ascend + x86 (A+X) platform, the service shows significant degradation in key performance metrics, such as overall throughput and latency, and fails to meet the expected performance targets.  
A preliminary comprehensive investigation ruled out common issues such as abnormal operator execution, NPU compute bottlenecks, memory leaks, network communication anomalies, environment configuration errors, and driver faults. System resource monitoring and kernel-state analysis revealed that some CPU cores on the A+K platform had an abnormally large number of interrupts, with substantial system resources consumed by hardware interrupt processing and continuously preempting CPU resources used by service tasks. This anomaly caused operator dispatch and scheduling on a single node to stall, resulting in an obvious slow node. In multi-rank cluster scenarios, this created a "fast-ranks-wait-for-slow-ranks" bottleneck that degraded overall cluster throughput, causing the A+K platform to perform significantly worse than the A+X platform.

## Troubleshooting Process

To accurately identify the trigger types, load distribution, processing durations, and service impact of high-frequency interrupts, kernel scheduling tracing tools were used together with service performance profiling tools to perform multidimensional correlated analysis and progressively narrow down the root cause. The complete troubleshooting process is as follows:

1. **Comprehensive kernel interrupt and scheduling data collection**  
Use the official `ftrace` tools to collect comprehensive kernel scheduling, hard interrupt, and soft interrupt data during stable model execution. Capture key information about the trigger frequency, CPU distribution, and processing duration of the `sq_send_trigger_irq` and `cq_update_irq` interrupts. For details, see [`ftrace_tools`](https://gitcode.com/Ascend/msinsight/tree/26.1.0/scripts/ftrace_tools).  
2. **Cross-analysis of multiple data sources**  
Align and correlate the interrupt logs collected by `ftrace` with the service Profiler's profile data, CPU utilization data, operator dispatch duration, and cluster rank duration data on a common timeline. Precisely correlate periods of high interrupt activity with periods of degraded service performance, stalled operator dispatch, and increased cluster latency to distinguish normal system interrupt activity from abnormally high-frequency interrupts associated with Ascend hardware operations.  
3. **Interrupt load and CPU hotspot identification**  
Compare the interrupt load across CPU cores through data analysis. The results show that interrupt processing is not evenly distributed, with a single CPU core handling a large number of `sq_send_trigger_irq` and `cq_update_irq` interrupts. This core is continuously preempted by interrupt processing, causing frequent interruptions to service threads, discontinuous operator dispatch, and a large number of scheduling bubbles.  
4. **Cluster slow-rank and performance bottleneck identification**  
Use MindStudio Insight cluster data to analyze the execution status of each rank and confirm that the service node corresponding to the CPU with excessive interrupt load is the slow rank in the cluster. Continuous interrupt preemption on this node delays operator dispatch and causes task backlogs, while other normal ranks continue to wait for the node to complete synchronization, degrading overall cluster performance.
5. **optimization**  
To address the core issues of uneven CPU interrupt load, excessive interrupt load on a single core, and frequent interruption of service threads, apply CPU affinity optimization. For details, see [CPU affinity script](../../../misc/host_analyzer/README_EN.md).
Configure CPU affinity to isolate service processes from interrupt processing: bind core service threads, such as model operator dispatch and data transfer threads, to designated idle CPU cores, while configuring interrupt affinity for high-frequency hardware interrupts (`sq_send_trigger_irq` and `cq_update_irq`) to distribute interrupt processing across other CPU cores. This eliminates resource contention caused by a single CPU core simultaneously handling a large number of interrupts and core service tasks, prevents frequent interruption of service threads, and ensures continuous operator dispatch.

## Root Causes

1. **Inherent hardware interaction interrupt mechanism**  
The host and NPU on the Ascend platform use interrupts for task dispatch and status synchronization. The `sq_send_trigger_irq` and `cq_update_irq` interrupts are essential hardware interaction interrupts during model execution. Their trigger frequency naturally increases under high-concurrency task dispatch, resulting in inherent scheduling overhead.  
2. **Uneven interrupt load distribution (core root cause)**  
The default interrupt scheduling strategy on the A+K platform is not specifically optimized for these workloads. The two types of core service interrupts are excessively concentrated on individual CPU cores, without achieving balanced load distribution across multiple cores. A single CPU core continuously handles hardware interrupts at a high frequency, repeatedly preempting CPU time allocated to core service threads responsible for model operator dispatch and data transfers and disrupting continuous service execution.  
3. **Architectural adaptation differences exacerbating the issue**  
Compared with the x86 platform, the Kunpeng ARM architecture has differences in interrupt scheduling response and multicore load distribution mechanisms. The default scheduling configuration is less optimized for high-frequency, short-duration hardware interrupts, making it more susceptible to single-core interrupt overload and frequent preemption of service threads. As a result, the A+K platform performs significantly worse than the A+X platform.  
4. **Cascading cluster performance degradation**  
Single-core CPU interrupt overload causes dispatch stalls and slow-rank issues on the affected node. In multi-rank cluster scenarios, this creates a global performance bottleneck: all normal ranks must wait for the slow rank to complete task synchronization, significantly increasing overall cluster synchronization wait duration and inference latency and reducing service throughput.

## Methodology Summary

For suspected interrupt-related interference in Ascend model workloads, such as performance degradation in cross-platform comparisons, fast-rank/slow-rank discrepancies, operator dispatch stalls, and high CPU kernel overhead, a standardized troubleshooting process can be established to quickly identify IR interrupt-related performance bottlenecks:

1. **Initial feature screening**  
Compare performance across platforms and nodes to determine whether there are symptoms such as a single node becoming a performance bottleneck, high CPU kernel-mode overhead, or an increase in pipeline bubbles in the dispatch path. Rule out common issues related to operators, hardware, communication, and code.  
2. **Dedicated interrupt data collection**  
Use `ftrace` to collect system-wide hard interrupt, soft interrupt, and CPU scheduling data, with a focus on the trigger frequency and CPU distribution of Ascend-specific interrupts such as `sq_send_trigger_irq` and `cq_update_irq`.  
3. **Correlation with service data**  
Align Profiler operator dispatch data and cluster rank duration data with periods of high interrupt activity to determine whether high interrupt frequency is strongly correlated with service stalls and performance degradation, distinguishing normal interrupt overhead from abnormal overload bottlenecks.
4. **Precise identification and optimization**  
Identify CPU cores with excessive interrupt load and high-frequency abnormal interrupts, and determine whether there are issues such as uneven load distribution or unsuitable scheduling strategies. Then, perform targeted optimization of interrupt affinity, CPU affinity, and scheduling parameters.

## Suggestions for Tool Improvements

`ftrace` can comprehensively collect basic interrupt and CPU scheduling data, but its dedicated analysis capabilities for Ascend-specific service interrupts are insufficient, resulting in relatively high manual filtering and correlation costs. The following enhancements are recommended to improve troubleshooting efficiency for similar issues:

1. **Adding Ascend-specific interrupt identification and aggregation**  
The tool currently provides only general interrupt statistics. Add dedicated support for Ascend hardware interrupts such as `sq_send_trigger_irq` and `cq_update_irq`, including statistics on trigger frequency, per-CPU load, and average processing duration. Automatically aggregate service-related interrupt data and filter out unrelated system interrupts.
2. **Adding interrupt-to-service-path correlation**  
Support automatic correlation between periods of high interrupt activity and operator dispatch, task queues, and cluster synchronization. Provide a visual representation of the timing and duration of interrupt-induced service-thread interruptions, eliminating the need for manual timeline comparison and enabling rapid identification of affected service paths.
3. **Adding interrupt load-balance detection and alerts**  
Add statistics for the distribution of interrupt load across CPU cores to automatically identify abnormal scenarios such as single-core interrupt overload and uneven load distribution. Support threshold-based alerts to identify potential slow-rank risks in advance and support pre-deployment checks and routine monitoring.
4. **Generating optimization recommendation reports**  
Based on interrupt distribution data, automatically generate recommendations for interrupt affinity configuration, CPU affinity, and scheduling parameter tuning to reduce the manual analysis and optimization effort and improve end-to-end issue resolution efficiency.
