# GIL Contention Analysis

## Background

Python uses a core mechanism called the Global Interpreter Lock (GIL) that restricts Python bytecode execution to a single thread at any given time, preventing multithreaded applications from achieving true parallel execution of computational tasks.  
The customer's service model is developed in Python and extensively uses a multithreaded architecture to perform high-frequency operations such as operator dispatch, memory copies, and task scheduling in parallel. In concurrent multithreaded scenarios, multiple application threads continuously compete for the single GIL, frequently resulting in lock contention, thread blocking, and context switching. In particular, a thread that holds the GIL for an extended period can further exacerbate uneven thread scheduling and reduce effective compute utilization, degrading overall model performance.

## Symptoms

After the customer's existing Python model was migrated to and deployed on the Ascend platform, its overall performance failed to meet the expected targets, with long execution durations and insufficient task throughput.  
Preliminary investigation confirmed that the customer's service had no operator errors, data anomalies, or hardware resource bottlenecks. Further investigation based on the characteristics of the service code revealed extensive multithreaded concurrent operations in the core service path. This indicated a performance bottleneck caused by abnormal GIL contention in Python multithreading.

## Troubleshooting Process

To accurately locate abnormal code paths and thread behavior related to GIL contention, holding, and release, the self-developed `gil_tracer` tool was used for dedicated data analysis. The complete troubleshooting process is as follows:

1. **Data collection:** Use the [`gil_tracer` tool](../../../misc/gil_tracer/README_EN.md) to continuously capture data from the customer's process, recording comprehensive behavior data for GIL contention, holding, voluntary release, involuntary switching, blocking, and waiting across all service threads, and export standardized `gil_trace` log data.  
2. **Joint data analysis:** Cross-reference and correlate the GIL behavior data in `gil_trace` with the service Profiler's profile data to determine the relationships among thread execution duration, CPU utilization, task blocking periods, and GIL status.  
3. **Abnormal code path identification:** Accurately identify abnormal behavior through data analysis. Some service threads hold the GIL for an extended period without voluntarily releasing it, causing other ready threads to remain blocked while waiting for the GIL and preventing them from being scheduled normally. This creates a severe thread scheduling bottleneck and identifies the core code logic responsible for the performance issue.

## Root Causes

1. **Inherent mechanism-level bottleneck:** The fact that only one thread can execute Python bytecode under the GIL at a time prevents pure Python multithreading from utilizing multicore compute resources in parallel. Lock contention overhead is therefore inherent in highly concurrent multithreaded scenarios.  
2. **Service code implementation defect (core root cause):** The customer's multithreaded service logic was not designed appropriately. Some worker threads held the GIL for an extended period while executing time-consuming logic without voluntarily releasing it, and no appropriate lock release or thread-yielding mechanism was provided.  
3. **Cascading performance impact:** Abnormal GIL holding causes other service threads to continuously contend for the GIL and remain blocked while waiting. This results in imbalanced thread scheduling and frequent context switching, with substantial compute resources wasted on lock contention and thread waiting. Consequently, overall model execution efficiency is significantly reduced, causing the migrated model to fail to meet its performance targets.

## Methodology Summary

For Python-based model migration involving multithreaded services, performance degradation, and low throughput, a standardized troubleshooting process can be established to quickly identify GIL-related performance issues:
 
1. **Initial screening:** Determine whether the service contains extensive multithreaded concurrent logic, and rule out common performance bottlenecks related to hardware, operators, data paths, and environment configuration.
2. **Dedicated data collection:** Prioritize the use of the `gil_tracer` tool to collect comprehensive data on GIL contention, holding, release, blocking, and waiting in the customer's process.
3. **Joint analysis:** Combine the data with the Profiler's profile data to correlate GIL behavior with execution bottlenecks and distinguish normal lock contention from abnormal lock holding and prolonged lock waiting.
4. **Precise identification:** Identify abnormal code segments involving prolonged lock holding, frequent lock contention, or severe thread blocking to guide service code optimization.

## Suggestions for Tool Improvements

The `gil_tracer` tool can comprehensively collect GIL behavior data, but its thread identification capabilities are insufficient. To further improve troubleshooting efficiency, the following enhancements are recommended:

1. **Collecting and displaying thread names:** Add service thread names and thread IDs to trace logs and visualization results so that abnormal threads can be directly associated with their service functions, such as data transfer threads and operator dispatch threads. This eliminates the need for manual code tracing and enables rapid identification of abnormal functional threads.
2. **Optimizing abnormality aggregation and statistics:** Add statistics for prolonged GIL holding, frequent lock contention, and thread blocking time, and automatically aggregate data for abnormal threads to provide an intuitive view of bottlenecks and reduce the manual analysis effort.
