# KVCache传输影响模型性能

## 问题背景

在PD分离（Prefill-Decode分离）部署场景下，Prefill节点完成输入序列的KV Cache计算后，需要将KV Cache传输到Decode节点进行后续的逐Token生成。KV Cache的跨节点传输是PD分离架构的关键路径，传输延迟直接影响Decode节点的首Token生成时间和整体请求延迟。在分布式KV Cache池化场景中，跨节点的KV Cache加载和写入同样涉及大量数据传输。昇腾通过HIXL单边通信库和MemFabric跨节点内存统一编址技术优化KV Cache传输路径，但在实际部署中，网络带宽不足、传输协议开销大、内存拷贝次数多、传输与计算未充分重叠等问题仍可能导致KV Cache传输成为性能瓶颈。

## 问题来源

推理

## 问题现象

用户通常先看到PD分离场景下Decode节点的请求等待时间（`queue_wait_time`）显著偏高，首Token时延（TTFT）中Prefill执行完成后到Decode开始生成之间的间隔时间过长。进一步观察PD分离相关指标时，可能出现：

- 在`pd_split_communication.csv`中，`prefill_res_time`到`request_end_time`之间的时间差过大，说明KV Cache传输占用了大量时间。
- 在`pd_split_kvcache.csv`中，`during_time(ms)`（KV Cache从P节点传输到D节点的耗时）偏高，且与请求的`seq_len`（序列长度）呈正相关。序列越长，KV Cache数据量越大，传输耗时越长。
- 在Timeline视图中，可观察到Prefill节点的ModelExecute色块结束后，Decode节点存在明显的等待间隙，直到KV Cache传输完成后才开始Decode执行。如果传输与计算未重叠，这个等待间隙直接转化为端到端延迟的增加。
- 在分布式KV Cache池化场景中，跨节点缓存加载的耗时导致请求的Prefill阶段耗时增加，即使本地已有部分缓存，远程加载的延迟仍可能抵消缓存命中带来的收益。

## 定位过程

### 步骤 1：用Profiler采集PD分离场景的通信和KVCache数据

在PD分离部署的多机多卡场景中，需要在各节点统一配置`ms_service_profiler_config.json`，设置`domain`为`"Request; KVCache; Communication; Schedule; ModelExecute"`，确保采集Communication和KVCache domain数据。采集完成后执行解析：

```bash
python3 -m ms_service_profiler.parse --input-path ${PATH}/prof_dir/
```

解析生成`pd_split_communication.csv`、`pd_split_kvcache.csv`、`request.csv`、`batch.csv`、`chrome_tracing.json`等文件。

### 步骤 2：分析pd_split_kvcache.csv中的KV Cache传输耗时

在`pd_split_kvcache.csv`中查看每条请求的KV Cache传输数据。关键字段包括：

- `seq_len`：请求序列长度，决定KV Cache数据量
- `during_time(ms)`：KV Cache从P节点传输到D节点的实际耗时
- `block_tables`：传输的block信息

按`seq_len`分组统计`during_time(ms)`的分布（平均值、P90、P99）。计算传输带宽（`seq_len * 模型KV维度 * 层数 * 数据类型字节数 / during_time`），与理论网络带宽对比。如果实际传输带宽远低于理论带宽，说明传输效率存在问题。

按`rank`（设备ID）分组统计，判断是否存在特定设备或链路的传输慢问题。

### 步骤 3：分析pd_split_communication.csv中的端到端通信时序

在`pd_split_communication.csv`中查看关键时间节点：

- `http_req_time(ms)`：请求到达时间
- `send_request_time(ms)`：P节点开始向D节点发送请求时间
- `send_request_succ_time(ms)`：请求发送成功时间
- `prefill_res_time(ms)`：Prefill执行完成时间
- `request_end_time(ms)`：请求执行完毕时间

计算各阶段耗时：

- 请求转发耗时 = `send_request_succ_time` - `send_request_time`
- Prefill执行耗时 = `prefill_res_time` - `send_request_succ_time`
- KV Cache传输+Decode执行耗时 = `request_end_time` - `prefill_res_time`

如果KV Cache传输+Decode执行耗时在总耗时中占比过高，且与`seq_len`强相关，可确认KV Cache传输是瓶颈。

### 步骤 4：通过Timeline视图确认传输与计算的重叠情况

打开`chrome_tracing.json`，在Timeline视图中定位PD分离场景的KV Cache传输过程。观察Prefill节点的ModelExecute色块结束时间与Decode节点开始执行时间之间的间隙。如果这个间隙与`pd_split_kvcache.csv`中的`during_time(ms)`吻合，可确认KV Cache传输是Decode等待的直接原因。

在Timeline中观察是否存在KV Cache传输与计算重叠的情况。如果传输完全串行在Prefill之后、Decode之前，说明传输与计算未重叠，存在优化空间。

### 步骤 5：在Grafana中观察PD分离场景指标

在PD分离部署场景中，分别观察Prefill节点和Decode节点的指标。如果Decode节点的`waiting_batch_size`持续偏高而`batch_size`偏低，且Prefill节点负载正常，说明Decode节点在等待KV Cache传输。观察`request_status.csv`中Decode节点的waiting/running状态变化，判断等待时间是否与KV Cache传输相关。

### 步骤 6：用多维度解析工具获取请求维度统计

```bash
msserviceprofiler analyze --input-path=/path/to/input
```

查看`request_summary.csv`中`first_token_latency(ms)`和`waiting_time(ms)`的统计，判断TTFT中等待时间的占比。

## 问题根因

KVCache传输影响模型性能的常见根因包括：

1. **通信协议开销大**：使用传统双边通信协议（如HCCL集合通信）进行KV Cache传输时，发送方和接收方需要多次握手确认，协议开销在高频、小包、单向的PD分离通信模式下被放大。

2. **内存拷贝次数多**：KV Cache从Prefill侧计算缓冲区到网络传输再到Decode侧工作内存，经历多次内存拷贝，每次拷贝消耗DMA带宽和内存总线资源。昇腾HIXL单边通信库通过零拷贝（RDMA直写远程内存）可消除中间拷贝，但若未启用HIXL或配置不当，仍使用传统路径。

3. **网络带宽不足或链路拥塞**：PD分离场景下KV Cache传输对网络带宽要求高，如果网络带宽不足或存在多流竞争，传输延迟显著增加。跨节点KV Cache池化场景中，多节点同时读写远程内存可能造成网络拥塞。

4. **传输与计算未重叠**：KV Cache传输串行在Prefill执行之后、Decode执行之前，未与计算阶段重叠。理想情况下，KV Cache传输应与Prefill的后续计算或Decode的初始化过程并行，隐藏传输延迟。

5. **序列长度过大导致传输数据量激增**：长序列场景下KV Cache数据量与序列长度成正比，百万级Token上下文的KV Cache可达数十GB，单次传输耗时可能超过数秒。

6. **分布式缓存池化架构中数据路径过长**：使用Mooncake等方案时，调用链冗长，每层引入额外延迟。昇腾通过KV Connector直接对接后端、HIXL零拷贝传输、MemFabric统一内存编址等优化缩短数据路径。

## 解决方法

- 通信协议开销大：启用昇腾HIXL单边通信库替代传统双边通信协议。
- 内存拷贝多：使用MemFabric跨节点内存统一编址减少数据搬运，启用HIXL零拷贝传输。
- 网络带宽不足：增加网络带宽或使用RDMA高速网络。
- 传输与计算未重叠：优化传输与计算的重叠（如Pipeline并行），让传输与Prefill后续计算或Decode初始化并行。
- 数据量过大：对KV Cache进行量化压缩减少传输数据量。
- 数据路径过长：在分布式缓存池化场景中使用KV Connector直接对接后端缩短调用链。

处理后需要回看`pd_split_kvcache.csv`中`during_time(ms)`是否下降、Decode节点等待时间是否减少、TTFT是否恢复。

## 定位方法论总结

该场景的完整判断链是：先确认是否为PD分离或分布式KV Cache池化部署场景；再用msServiceProfiler采集Communication和KVCache domain数据，通过`pd_split_kvcache.csv`直接获取KV Cache传输耗时；然后通过`pd_split_communication.csv`分析端到端时序，计算传输阶段在总耗时中的占比；最后通过Timeline视图确认传输与计算的重叠情况。

核心判断逻辑：如果`pd_split_kvcache.csv`中`during_time(ms)`偏高且与`seq_len`强相关，同时`pd_split_communication.csv`中`request_end_time - prefill_res_time`占比过高，则KV Cache传输是瓶颈。进一步判断：如果实际传输带宽远低于理论带宽，则是网络或协议效率问题；如果传输带宽正常但总耗时长，则是数据量过大问题。

## 对工具的改进建议

### msServiceProfiler

当前Profiler已能通过`pd_split_kvcache.csv`和`pd_split_communication.csv`分析KV Cache传输耗时。建议在`pd_split_kvcache.csv`中增加传输带宽字段，自动计算实际传输带宽，便于与理论带宽对比。在`pd_split_kvcache.csv`中增加传输路径标识字段（如HIXL/MemFabric/HCCL/TCP），帮助判断当前使用的传输方式。在`pd_split_communication.csv`中增加更细粒度的通信阶段拆解，如"KV Cache序列化耗时"、"网络传输耗时"、"KV Cache反序列化耗时"、"内存拷贝耗时"等。在Timeline视图中增加KV Cache传输泳道，将传输过程独立展示，便于观察传输与计算的重叠情况。

### ms-service-metric

当前在线监控已能按节点观察PD分离场景指标。建议增加PD分离场景的Decode节点等待时间监控指标，区分"等待KV Cache传输"和"等待调度"两种等待类型。增加KV Cache传输延迟的在线监控指标，便于实时观察传输性能。
