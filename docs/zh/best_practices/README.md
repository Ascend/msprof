# msProf 典型案例

本章节通过性能调优的典型场景提供对应的性能调优案例，包含如下案例。

**基础案例**

- [ResNet50推理模型性能分析](basic_cases.md)

**Host侧**

- 模型代码类
  - [代码高耗时函数段优化](code_high_latency_function.md)
  - [下发性能不及预期](dispatch_small_batch_performance.md)
  - [算子编译高耗时](operator_compilation_latency.md)
  - [同步接口频繁调用](sync_api_frequent_call.md)
- 系统调度类
  - [CPU Cache Miss资源冲突与受限](cpu_cache_miss_conflict.md)
  - [CPU 线程频繁切换](cpu_thread_frequent_switching.md)
  - [GIL锁抢占问题分析](gil_lock_preemption.md)
  - [IRQ中断打断问题分析](irq_interruption.md)
  - [内核进程频繁切换](kernel_thread_switching.md)
  - [Pthread线程锁等待](pthread_lock_wait.md)
  - [Python GC回收高耗时](python_gc_high_latency.md)
  - [系统调用高耗时函数](syscall_high_latency.md)

**Device侧**

- NPU通信
  - [通信地址不对齐导致性能下降](communication_address_misalignment.md)

- 内存
  - [内存碎片](memory_fragmentation.md)
  - [内存泄漏](memory_leak.md)
