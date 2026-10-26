# msProf Typical Cases

This chapter provides performance tuning cases for typical performance tuning scenarios, including the following cases:

**Basic Cases**

- [ResNet50 Inference Model Performance Analysis](basic_cases.md)

**Host Side**

- Model Code
  - [Optimizing Operators with Long Execution Time](code_high_latency_function.md)
  - [Dispatch Performance Below Expectation](dispatch_small_batch_performance.md)
  - [High Latency in Operator Compilation](operator_compilation_latency.md)
  - [Frequent Calls to Synchronous APIs](sync_api_frequent_call.md)
- System scheduling
  - [CPU Cache Miss Resource Contention and Constraints](cpu_cache_miss_conflict.md)
  - [Frequent CPU Thread Switching](cpu_thread_frequent_switching.md)
  - [GIL Contention Analysis](gil_lock_preemption.md)
  - [IRQ Interruption Analysis](irq_interruption.md)
  - [Frequent Kernel Process Switching](kernel_thread_switching.md)
  - [Pthread Lock Wait](pthread_lock_wait.md)
  - [High Python GC Overhead](python_gc_high_latency.md)
  - [High-Latency System Calls](syscall_high_latency.md)

**Device side**

- NPU communication
  - [Performance Degradation Caused by Communication Address Misalignment](communication_address_misalignment.md)

- Memory
  - [Memory Fragmentation](memory_fragmentation.md)
  - [Memory Leak](memory_leak.md)
