# High Python GC Overhead

## 1. Background

During GPT-like LLM training in a multi-node distributed environment, the customer observed frequent performance jitter.

## 2. Source

Training

## 3. Symptoms

The issue can be reproduced consistently. Long-running analysis shows that the training step duration across nodes exhibits highly consistent fluctuation patterns and frequencies. The duration periodically fluctuates between approximately 2770 ms and 2900 ms, as shown in the following figure.

![Python GC Train Case](../figures/profiler_case_gc_train.png)

## 4. Troubleshooting Process

1. Use the Ascend PyTorch Profiler with GC detection enabled to collect and analyze model profile data. The analysis shows that, in the abnormal steps where training duration fluctuates, a significant `free` duration is observed on the timeline. This `free` operation occurs in a Python function call stack on the framework side and is not directly associated with NPU hardware execution or HCCL communication operators. After multiple profiling sessions and cross-validation, the abnormal `free` duration is consistently observed in the execution path of the same `nn.Module`, and a GC event is detected during the same period.

   Use the following configuration:

   ```python
   import torch_npu
   experimental_config = torch_npu.profiler._ExperimentalConfig(
       profiler_level=torch_npu.profiler.ProfilerLevel.Level0,
       aic_metrics=torch_npu.profiler.AiCMetrics.AiCoreNone,
       data_simplification=False,
       gc_detect_threshold=1,    # Enable the GC detection option and set the GC detection threshold to 1 ms.
   )
   
   # Add the basic profiling configuration parameters. For details, see the parameter descriptions below.
   with torch_npu.profiler.profile(
       activities=[
           torch_npu.profiler.ProfilerActivity.CPU,
           torch_npu.profiler.ProfilerActivity.NPU
       ],
       schedule=torch_npu.profiler.schedule(wait=0, warmup=0, active=1, repeat=1, skip_first=0),    # Used together with prof.step()
       on_trace_ready=torch_npu.profiler.tensorboard_trace_handler("./result"),
       experimental_config=experimental_config) as prof:
   
       # Start profile data collection.
       for step in range(steps):    # Training iterations
           train_one_step()         # Training function
           prof.step()              # Used together with schedule
   ```

   The profile data is shown in the following figures.

   ![Python GC Train Case](../figures/profiler_case_gc_profiler.png)

   ![Python GC Train Case](../figures/profiler_case_gc_trace_view.png)

2. Print the Python GC count.

   Print the Python GC count at each training step. The Python GC count increases significantly in abnormal steps, resulting in fluctuations in training duration.

   ```python
   import gc
   print(gc.get_count())
   ```

   The results are shown in the following figure.

   ![Python GC Train Case](../figures/profiler_case_gc_count.png)

3. Manually disable Python GC.

   Disable Python GC before starting model training.

   ```python
   import gc
   gc.disable()
   ```

   Result: The performance jitter is eliminated.

4. Adjust the Python GC reclamation threshold.

   Adjust the Python GC thresholds before starting model training. For details, see the [Python GC thresholds](https://docs.python.org/3/library/gc.html#gc.set_threshold) documentation.

   ```python
   import gc
   gc.set_threshold(700, 10, 1000)
   ```

   Result: The performance jitter persists but occurs less frequently.

## 5. Root Cause

As an interpreted language, Python uses an interpreter implemented in C to parse and execute bytecode instructions. In Python's memory management mechanism, each object maintains a reference count to track whether it is referenced. When an object's reference count reaches 0, indicating that the object is no longer referenced, Python immediately reclaims the memory occupied by the object.

However, when circular references form between Python objects (that is, a cyclic dependency where A references B and B references A), the reference counting mechanism alone cannot reduce the reference counts of these objects to zero, and this portion of memory cannot be reclaimed automatically. To address the memory leak caused by circular references, Python introduces an automatic Garbage Collection (GC) mechanism. When the number of accumulated objects that form circular references exceeds the configured reclamation threshold, Python automatically triggers a GC event, which scans and releases these useless objects involved in circular references through the `mark-and-sweep` algorithm.

It is important to note that GC is a compute-intensive operation that can incur significant execution overhead, and it can block the Python process during execution due to its Stop-the-World behavior. In foundation-model training scenarios, where performance stability is critical, the most typical external symptom of a GC event is a sudden increase in the duration of an individual training step, resulting in significant periodic performance fluctuations throughout the training process.

## 6. Troubleshooting Methodology Summary

For this scenario, first use the Ascend PyTorch Profiler to collect model profile data and check whether GC events occur in the timeline. If a GC event is present, analyze its trigger conditions, impact, and duration, and adjust the Python GC thresholds to reduce the frequency of GC events.

## 7. Suggestions for Tool Improvement

None.
