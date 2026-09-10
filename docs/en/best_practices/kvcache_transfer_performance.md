# Impact of KV Cache Transfer on Model Performance

## Background

In prefill/decode (PD) disaggregation deployments, after the prefill node completes computing the KV cache for the input sequence, the KV cache must be transferred to the decode node for subsequent token-by-token generation. Cross-node KV cache transfer is a critical path in PD disaggregation, and its latency directly affects the time to first token on the decode node and overall request latency. In distributed KV cache pooling scenarios, loading and writing KV cache across nodes also involves substantial data transfer. Ascend uses the HIXL one-sided communication library and MemFabric's unified cross-node memory addressing technology to optimize the KV cache transfer path. However, insufficient network bandwidth, high communication protocol overhead, excessive memory copies, and insufficient overlap between data transfer and computation can still cause KV cache transfer to become a performance bottleneck in actual deployments.

## Source

Inference

## Symptoms

Users typically first observe extended request wait time on decode nodes rather than a direct indication of "KV cache transfer overhead." In PD disaggregation scenarios, `queue_wait_time` may be significantly higher, and the interval between the completion of prefill execution and the start of token generation on the decode node may be excessively long. Further observation of PD disaggregation metrics may reveal the following:

- In `pd_split_communication.csv`, the time difference between `prefill_res_time` and `request_end_time` is excessively long, indicating that KV cache transfer may account for a substantial portion of the execution time.
- In `pd_split_kvcache.csv`, `during_time(ms)`, which indicates the time required to transfer the KV cache from the prefill node to the decode node, is high and positively correlated with `seq_len`. The longer the sequence, the more KV cache data is transferred and the longer the transfer takes.
- In the Timeline view, a noticeable gap can be observed between the end of the `ModelExecute` block on the prefill node and the start of decode execution on the decode node. decode starts only after KV cache transfer is complete. If data transfer and computation do not overlap, this waiting gap directly increases end-to-end latency.
- In distributed KV cache pooling scenarios, the time required to load cached data from remote nodes increases prefill latency. Even when some cache data is available locally, the latency of remote loading may offset the benefits of cache hits.

## Troubleshooting Process

### Step 1: Collecting Communication and KV Cache Data with Profiler

In a multi-node, multi-device PD disaggregation deployment, configure `ms_service_profiler_config.json` consistently on all nodes and set `domain` to `"Request; KVCache; Communication; Schedule; ModelExecute"` to ensure that `Communication` and `KVCache` domain data is collected. After collection is complete, run the following command to parse the data:

```bash
python3 -m ms_service_profiler.parse --input-path ${PATH}/prof_dir/
```

The parsing process generates files such as `pd_split_communication.csv`, `pd_split_kvcache.csv`, `request.csv`, `batch.csv`, and `chrome_tracing.json`.

### Step 2: Analyzing KV Cache Transfer Duration in `pd_split_kvcache.csv`

In `pd_split_kvcache.csv`, examine the KV cache transfer data for each request. Key fields include:

- `seq_len`: request sequence length, which determines the amount of KV cache data transferred.
- `during_time(ms)`: actual time required to transfer the KV cache from the prefill node to the decode node.
- `block_tables`: information about the blocks being transferred.

Group requests by `seq_len` and analyze the distribution of `during_time(ms)`, including the average, P90, and P99 values. Calculate the effective transfer bandwidth based on the KV cache data volume and `during_time(ms)`, and compare it with the theoretical network bandwidth. If the effective transfer bandwidth is substantially lower than the theoretical bandwidth, investigate network bandwidth, protocol overhead, memory copies, and other transfer efficiency issues.

Group the data by `rank` (device ID) to determine whether transfer performance is abnormal on specific devices or links.

### Step 3: Analyzing End-to-End Communication Timing in `pd_split_communication.csv`

In `pd_split_communication.csv`, examine the following key timestamps:

- `http_req_time(ms)`: time when the request arrives.
- `send_request_time(ms)`: time when the prefill node starts sending the request to the decode node.
- `send_request_succ_time(ms)`: time when the request is successfully sent.
- `prefill_res_time(ms)`: time when prefill execution is completed.
- `request_end_time(ms)`: time when request execution is completed.

Calculate the duration of each stage:

- Request forwarding duration = `send_request_succ_time(ms)` - `send_request_time(ms)`
- Prefill execution duration = `prefill_res_time(ms)` - `send_request_succ_time(ms)`
- KV cache transfer plus decode execution duration = `request_end_time(ms)` - `prefill_res_time(ms)`

If the KV cache transfer plus decode execution duration accounts for an excessive proportion of the total duration and is strongly correlated with `seq_len`, further investigate KV cache transfer as a potential bottleneck. The `during_time(ms)` field in `pd_split_kvcache.csv` should be used to confirm whether KV cache transfer itself accounts for a substantial portion of this duration.

### Step 4: Confirming Transfer and Computation Overlap in the Timeline View

Open `chrome_tracing.json` and locate the KV cache transfer process for the PD disaggregation scenario in the Timeline view. Observe the gap between the end of the `ModelExecute` block on the prefill node and the start of execution on the decode node. If this gap is consistent with `during_time(ms)` in `pd_split_kvcache.csv`, KV cache transfer can be identified as a direct cause of decode-side waiting.

Check whether KV cache transfer overlaps with computation in the Timeline. If transfer occurs entirely after prefill execution and before decode execution, transfer and computation do not overlap, indicating an optimization opportunity.

### Step 5: Monitoring PD Disaggregation Metrics in Grafana

In a PD disaggregation deployment, monitor the prefill and decode nodes separately. If `waiting_batch_size` remains high and `batch_size` remains low on the decode node while the prefill node is operating normally, the decode node may be waiting for KV cache transfer. Examine the waiting/running state transitions of the decode node in `request_status.csv` to determine whether the waiting time is correlated with KV cache transfer.

### Step 6: Obtaining Request-Level Statistics with the Multidimensional Analysis Tool

```bash
msserviceprofiler analyze --input-path=/path/to/input
```

Check the statistics for `first_token_latency(ms)` and `waiting_time(ms)` in `request_summary.csv` to determine the proportion of TTFT attributable to waiting time.

## Root Causes

Common causes of KV cache transfer affecting model performance include:

1. **High communication protocol overhead**: When traditional two-sided communication protocols, such as HCCL collective communication, are used for KV cache transfer, the sender and receiver require multiple handshakes and confirmations. This protocol overhead is amplified in the high-frequency, small-packet, one-way communication pattern of PD disaggregation.

2. **Excessive memory copies**: KV cache data may undergo multiple memory copies from the prefill-side computation buffer along the network transmission path to decode-side working memory. Each copy consumes DMA bandwidth and memory bus resources. Ascend HIXL uses zero-copy transfer, in which RDMA writes directly to remote memory, to eliminate intermediate copies. However, if HIXL is not enabled or is configured incorrectly, the traditional transfer path may still be used.

3. **Insufficient network bandwidth or link congestion**: KV cache transfer in PD disaggregation requires substantial network bandwidth. Insufficient bandwidth or contention among multiple flows can significantly increase transfer latency. In distributed KV cache pooling scenarios, simultaneous remote-memory reads and writes across multiple nodes may also cause network congestion.

4. **Insufficient overlap between transfer and computation**: KV cache transfer occurs serially after prefill execution and before decode execution rather than overlapping with computation. Ideally, KV cache transfer should run in parallel with subsequent prefill computation or decode initialization to hide transfer latency.

5. **Excessive sequence length increasing the amount of transferred data**: In long-sequence scenarios, KV cache data volume increases proportionally with sequence length. For million-token contexts, the KV cache can reach tens of GB, and a single transfer may take several seconds.

6. **Excessively long data paths in distributed cache pooling architectures**: Solutions such as Mooncake may involve long call chains, with each layer introducing additional latency. Ascend uses a KV Connector to connect directly to the backend, HIXL for zero-copy transfer, and MemFabric for unified memory addressing to shorten the data path.

## Solutions

- **High communication protocol overhead**: Enable the Ascend HIXL one-sided communication library instead of traditional two-sided communication protocols.
- **Excessive memory copies**: Use MemFabric's unified cross-node memory addressing to reduce data movement and enable HIXL zero-copy transfer.
- **Insufficient network bandwidth**: Increase network bandwidth or use a high-speed RDMA network.
- **Insufficient overlap between transfer and computation**: Optimize the overlap between transfer and computation, such as through pipeline parallelism, so that transfer runs in parallel with subsequent prefill computation or decode initialization.
- **Excessive data volume**: Quantize or compress the KV cache to reduce the amount of data transferred.
- **Excessively long data path**: In distributed KV cache pooling scenarios, use the KV Connector to connect directly to the backend and shorten the call chain.

After applying the changes, recheck `during_time(ms)` in `pd_split_kvcache.csv`, decode-node waiting time, and TTFT to determine whether performance has improved.

## Methodology Summary

The complete troubleshooting path for this scenario is as follows: first confirm whether the deployment uses PD disaggregation or distributed KV cache pooling; then use `msServiceProfiler` to collect Communication and KVCache domain data and obtain KV cache transfer duration directly from `pd_split_kvcache.csv`; next analyze end-to-end timing with `pd_split_communication.csv` and determine the proportion of the transfer stage in the total duration; finally, use the Timeline view to confirm whether transfer overlaps with computation.

The core diagnostic logic is as follows: if `during_time(ms)` in `pd_split_kvcache.csv` is high and strongly correlated with `seq_len`, while `request_end_time(ms) - prefill_res_time(ms)` in `pd_split_communication.csv` accounts for an excessive proportion of the total duration, KV cache transfer is likely a bottleneck. If the effective transfer bandwidth is substantially lower than the theoretical bandwidth, investigate network, protocol, and data-transfer efficiency. If the effective transfer bandwidth is normal but the transfer duration remains long, investigate excessive KV cache data volume.

## Suggestions for Tool Improvements

### `msServiceProfiler`

Currently, `msServiceProfiler` can analyze KV cache transfer duration through `pd_split_kvcache.csv` and `pd_split_communication.csv`. Add a transfer bandwidth field to `pd_split_kvcache.csv` to automatically calculate effective transfer bandwidth and facilitate comparison with theoretical bandwidth. Add a transfer-path identifier field, such as HIXL, MemFabric, HCCL, or TCP, to `pd_split_kvcache.csv` to help identify the transfer method currently in use. Add more granular communication-stage breakdowns to `pd_split_communication.csv`, such as "KV cache serialization duration," "network transfer duration," "KV cache deserialization duration," and "memory copy duration." Add a KV cache transfer track to the Timeline view to display the transfer process separately and facilitate analysis of transfer and computation overlap.

### `ms-service-metric`

Currently, `ms-service-metric` can display PD disaggregation metrics by node. Add a decode-node waiting-time metric for PD disaggregation scenarios and distinguish between "waiting for KV cache transfer" and "waiting for scheduling." Add an online KV cache transfer latency metric to facilitate real-time monitoring of transfer performance.
