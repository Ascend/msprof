# Analysis of Excessive Sampler Execution Time

## Background

In foundation-model inference, the Sampler is one of the core components in the generation stage. It is responsible for calculating the probability distribution from logits and sampling the next token after the model completes a single forward inference. The Sampler execution logic includes multiple steps, such as Softmax normalization, Top-K/Top-P filtering, temperature scaling, random sampling, and greedy selection. When the Sampler takes too long to execute, it directly increases the end-to-end latency of the entire decode stage. This impact is amplified over repeated iterations, especially in the frequently executed decode process, which is triggered in every iteration.

## Source

Inference

## Symptoms

This issue is consistently reproducible under high-concurrency or long-sequence scenarios.
**Issue Type**: Compute bottleneck / host-NPU interaction overhead.

1. **Sampler stage takes excessively long in the timeline**: In the MindStudio Insight timeline, the `sampler`-related operations occupy a significant time span within a request's model execution cycle after `modelExec` (model forward) ends. The execution time is comparable to or even longer than that of the model computation itself.
2. **Execution hangs at the sampling step**: Logs or debugging shows that execution is stuck at code lines such as `sampled_token_ids = sampler_output.sampled_token_ids`, and the program remains unresponsive for an extended period.
3. **Latency increases sharply under high concurrency**: As the number of concurrent requests increases, the token generation latency increases significantly, while the model forward execution time remains relatively stable. The bottleneck is concentrated in the Sampler stage.
4. **NPU and CPU utilization show an inverse relationship**: NPU utilization is relatively low, while the CPU core running the Sampler has high utilization, indicating significant host-side computation or synchronization overhead.

## Troubleshooting Process

### Step 1: Collecting Service Execution Timelines Using `msServiceProfiler`

Use **msServiceProfiler** to collect request-level timeline data, with a focus on the execution time of the Sampler stage. This tool is specifically designed for MindIE Service inference service scenarios and can collect the start and end timestamps of key processes.

1. **Prepare the configuration**: Create `ms_service_profiler_config.json` and enable fine-grained event collection.

   ```json
   {
       "enable": 1,
       "prof_dir": "/path/to/profile_output",
       "record_op_detail": true
   }
   ```

2. **Start data collection**:

   ```bash
   export SERVICE_PROF_CONFIG_PATH=/path/to/ms_service_profiler_config.json
   # Start the MindIE Service
   ```

3. **Reproduce the load**: Use concurrent service requests for stress testing to reproduce the scenario in which the Sampler takes abnormally long to execute.
4. **Parse the data**:

   ```bash
   python3 -m ms_service_profiler.parse --input-path=/path/to/profile_output
   ```

### Step 2: Performing Fine-grained Timeline Analysis Using MindStudio Insight

Import the generated `chrome_tracing.json` into **MindStudio Insight** for visual analysis. MindStudio Insight presents the execution of the entire process in the timeline and supports fine-grained analysis of the inference process.

1. **Locate the Sampler stage**: Locate the Sampler-related tracks in the timeline, such as the `Sampler`, `PostProcess`, or `Sampling` thread.
2. **Observe the execution sequence**: Focus on the interval between `modelExec` and `sampler`, as well as the length of the execution span within the Sampler.
3. **Identify implicit synchronization**: If there is a significant idle gap or wait marker in the Sampler stage, implicit synchronization between the host and device may be occurring. Experience from the vLLM community indicates that `sampler_output.sampled_token_ids` may involve synchronization or data copying from a GPU tensor to the CPU. This overhead can be amplified under high concurrency.
4. **Perform comparative analysis**: Compare Sampler execution time across different `batch_size` values or sequence lengths and determine whether there is a linear growth relationship.

### Step 3: Checking Sampling Parameters and Configuration

Check request parameters and service-side configuration to rule out excessive Sampler computation caused by parameter configuration.

1. **Check sampling parameters**: Check whether the request contains a complex combination of sampling parameters, such as `top_k`, `top_p`, `temperature`, and `repetition_penalty` being enabled simultaneously, or postprocessing parameters such as `logprobs` and `best_of` that require additional computation.
2. **Check the vocabulary size**: Check the model vocabulary size. For large vocabularies, such as those with more than 50K tokens, the Softmax computation itself can be relatively expensive.
3. **Check the quantization configuration**: Check whether Sampler-related operators are running with the appropriate data types. Mixed precision may introduce additional data type conversion overhead.

## Root Cause

**Implicit synchronization between the host and device or a serial bottleneck in the Sampler implementation**
(These issues fall under the **framework adaptation issues** and **scheduling implementation defects** categories.)

**Detailed explanation:**

1. **GPU-to-CPU data synchronization is a major cause**: Experience from the vLLM community indicates that the seemingly simple assignment `sampled_token_ids = sampler_output.sampled_token_ids` may actually trigger implicit synchronization or data copying from a GPU tensor to the CPU. Under high concurrency or when GPU memory is constrained, PCIe bandwidth may become a bottleneck, causing the sampling step to remain blocked for an extended period.

2. **Large-vocabulary Softmax computation overhead**: When the vocabulary size is large, such as 50K–100K, the computation required to perform Softmax and Top-K/Top-P filtering on logits is non-negligible. If the Sampler performs these operations on the CPU, a large amount of data must be copied from the device to the host. If they are performed on the NPU, they consume compute resources that could otherwise be used for the next decode iteration.

3. **Overly complex sampling parameters**: Enabling multiple sampling parameters simultaneously, such as `top_k`, `top_p`, `temperature`, and `repetition_penalty`, significantly increases the computational complexity of the Sampler, and some processing logic may be serial.

4. **Improper scheduling parameter configuration**: Experience from troubleshooting in the vLLM community indicates that setting `max_num_batched_tokens` too low increases the scheduling frequency, causing the sampling step to be triggered more frequently and amplifying the impact of the overhead from each sampling operation. In addition, when `max_num_seqs` is too large, a large number of sequences must be sampled within a single batch, and the sampling time increases linearly with `batch_size`.

## Summary of the Troubleshooting Methodology

For the **"excessive Sampler execution time"** scenario, the key to troubleshooting is to **distinguish between "actual sampling computation time" and "implicit data transfer/synchronization time"** and avoid misidentifying synchronization overhead as a compute bottleneck.

1. **Do not focus only on average execution time and ignore the timeline pattern**: If only average data is considered, long Sampler execution time may be misidentified as an efficiency issue with the sampling algorithm. You must use MindStudio Insight to observe the specific timeline pattern. If there are significant gaps or wait markers in the Sampler stage, investigate host-device synchronization issues first.

2. **Use pure-model testing to isolate scheduling interference**: According to the MindStudio official optimization guide, first use pure-model testing to evaluate the upper bound of inference performance and identify the bottleneck before analyzing service scheduling issues. You can directly invoke model execution for a single inference without going through the service framework and compare whether the Sampler overhead still occurs. This helps determine whether the issue originates from the Sampler implementation itself or the service scheduling layer.

3. **Check sampling parameters in logs**: Some scenarios with excessively long execution time may be caused by request parameters, such as a request containing an extremely high `top_k` value. It is recommended to add logging for sampling parameters on the service side to facilitate retrospective analysis.

4. **Adjust scheduling parameters for verification**: If you suspect that an excessively high scheduling frequency is amplifying the sampling overhead, try increasing `max_num_batched_tokens` and observe whether P99 latency improves. If it does, this indicates that the sampling step was triggered too frequently under the original configuration.

5. **Use msProf for operator-level analysis**: If you need to further identify which specific operator within the Sampler, such as Softmax or TopK, takes the most time, use msProf to collect operator-level profile data and then use the operator execution time panel in MindStudio Insight to identify the operators with the longest execution time.
