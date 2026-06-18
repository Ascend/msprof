# PrefixCache未命中

## 问题背景

在LLM推理应用中，长system prompt场景和多轮对话场景非常普遍。长system prompt场景下，不同请求的system prompt相同，对应的KV Cache计算也相同；多轮对话场景中，每一轮对话依赖所有历史轮次的上下文，历史轮次的KV Cache在后续每一轮中都要被重新计算。Prefix Caching（前缀缓存）通过缓存和复用已计算的公共前缀KV Cache，可以显著降低首Token时延（TTFT）和Prefill阶段计算量。Ascend-vLLM默认开启Prefix Caching特性，但在实际使用中，由于缓存命中条件不满足、缓存容量不足被淘汰、跨请求公共前缀token数不足block size等原因，可能出现PrefixCache未命中，导致TTFT升高、Prefill耗时增加。

## 问题来源

推理

## 问题现象

用户通常先看到推理服务的首Token时延（TTFT）出现波动性升高，部分请求的TTFT明显高于同类请求。进一步观察KVCache和请求相关指标时，可能出现：

- 在`request.csv`中可观察到`cache_hit_rate`字段值偏低，`first_token_latency(ms)`偏高。
- 在`batch.csv`中，Prefill类型Batch的`during_time(ms)`和`prefill_scheduled_tokens`均偏高，说明大量前缀token被重新计算而非复用缓存。
- 在KVCache相关指标中，`kvcache_usage_rate`波动较大，`free_blocks`频繁变化，说明缓存在不断被分配和释放，而非稳定复用。
- 在`kvcache.csv`中可观察到频繁的block分配（`blocks_allocated`）和释放（`blocks_freed`）操作，且`blocks_allocated`的值接近请求的完整前缀token数，说明缓存未命中导致全量前缀重新分配。

典型场景：多轮对话中，第二轮及之后的请求TTFT与首轮请求TTFT几乎相同，未体现出PrefixCache应有的加速效果；长system prompt场景下，相同system prompt的不同请求之间TTFT无差异。

## 定位过程

### 步骤 1：先确认KVCache使用情况和TTFT变化

在Grafana中查看`free_kvcache_blocks`、`allocated_kvcache_blocks`、`kvcache_usage_rate`等KVCache相关指标。如果`kvcache_usage_rate`长期处于低位但`allocated_kvcache_blocks`频繁波动，说明缓存在被反复分配释放而非复用。同时观察TTFT指标（`first_token_latency`），如果TTFT波动大且与KVCache使用率变化相关，可初步怀疑PrefixCache命中率问题。

### 步骤 2：用Profiler采集请求和KVCache数据

配置`ms_service_profiler_config.json`，设置`domain`为`"Request; KVCache; Schedule"`，开启数据采集。采集完成后执行解析：

```bash
python3 -m ms_service_profiler.parse --input-path ${PATH}/prof_dir/
```

解析生成`request.csv`、`kvcache.csv`、`batch.csv`等文件。

### 步骤 3：分析request.csv中的缓存命中率和TTFT

在`request.csv`中查看每条请求的`cache_hit_rate`字段。统计整体缓存命中率分布（平均值、P50、P90）。如果命中率普遍偏低，说明PrefixCache未有效工作。

按请求到达时间排序，观察`cache_hit_rate`的时间序列变化。如果命中率随时间波动大，可能是缓存容量不足导致频繁淘汰。对比相同`recv_token_size`（输入长度）的请求，如果输入长度相近但`first_token_latency(ms)`差异大，且低时延请求对应高`cache_hit_rate`，可确认PrefixCache命中率是TTFT的关键影响因素。

### 步骤 4：分析kvcache.csv中的缓存分配模式

在`kvcache.csv`中按时间序列观察`blocks_allocated`和`blocks_freed`的变化。如果每次新请求到达时`blocks_allocated`接近请求的完整前缀token数（而非仅增量部分），说明缓存未命中，全量前缀被重新分配。

统计`total_blocks`和`free_blocks`的变化趋势。如果`total_blocks`配置充足但`free_blocks`频繁大幅波动，说明缓存管理策略存在问题，可能是LRU淘汰策略过于激进或缓存容量配置不足。

### 步骤 5：分析batch.csv中的Prefill调度情况

筛选`batch_type`为`prefill`的行，查看`prefill_scheduled_tokens`和`during_time(ms)`。如果Prefill调度的token数接近请求的完整输入长度（而非增量），说明前缀未被缓存复用。对比相同输入长度的请求在不同时间的Prefill耗时，如果差异大且与缓存命中率相关，可确认根因。

### 步骤 6：用多维度解析工具获取请求维度统计

```bash
msserviceprofiler analyze --input-path=/path/to/input
```

查看`request_summary.csv`中`first_token_latency(ms)`的P50/P90/P99分位数和`input_token_num`分布，判断TTFT是否存在长尾。

## 问题根因

PrefixCache未命中的常见根因包括：

1. **跨请求公共前缀token数不足block size**：Ascend-vLLM的Prefix Caching基于PagedAttention的block机制，仅当跨请求公共前缀token数大于等于block size时，才会复用公共前缀的KV Cache。如果公共前缀较短（如简短的system prompt），无法触发缓存复用。

2. **缓存容量不足导致频繁淘汰**：KVCache总block数（`total_blocks`）配置不足，或并发请求数过多导致缓存被快速占满。当新请求到达时，LRU策略淘汰了本可复用的前缀缓存，导致后续相同前缀请求无法命中。

3. **Prefix Caching特性未正确启用**：虽然Ascend-vLLM默认开启Prefix Caching，但如果同时配置了`ascend_scheduler_config`，两者存在冲突，Prefix Caching会被禁用。此外，多模态模型当前不支持Prefix Caching。

4. **请求前缀哈希冲突或缓存索引失效**：在分布式部署或多实例场景下，不同实例的缓存相互隔离，请求被路由到不同实例时无法复用其他实例的缓存。PD分离场景下，Prefill节点和Decode节点的缓存独立管理，跨节点缓存共享机制未启用。

5. **模型限制**：当前仅Qwen2.5和Qwen3系列模型支持Prefix Caching特性，其他模型使用该特性可能无效。

6. **Chunked Prefill与Prefix Caching的交互**：Prefix Caching生效时Chunked Prefill也同时生效，分块Prefill可能导致缓存块边界与请求前缀边界不对齐，影响缓存命中。

## 解决方法

- 公共前缀不足block size：确保system prompt或对话历史长度大于block size，或调整block size配置。
- 缓存容量不足：增加KVCache总block数配置，或降低单实例并发。
- Prefix Caching未启用：检查是否同时配置了`ascend_scheduler_config`，如有冲突则移除；确认模型在支持列表中。
- 分布式缓存隔离：在分布式场景启用跨节点KV Cache池化（如HIXL+MemFabric方案），实现缓存共享。
- Chunked Prefill交互：调整Chunked Prefill的分块大小，使其与block size对齐。

处理后需要回看`cache_hit_rate`是否提升、TTFT是否下降、`blocks_allocated`是否从全量分配变为增量分配。

## 定位方法论总结

该场景的完整判断链是：先用ms-service-metric观察KVCache使用率波动和TTFT变化趋势；再用msServiceProfiler采集Request和KVCache domain数据，通过`request.csv`的`cache_hit_rate`字段直接确认缓存命中率；然后通过`kvcache.csv`分析缓存分配释放模式，判断是容量不足还是命中条件不满足；最后结合`batch.csv`的Prefill调度token数确认前缀是否被重新计算。

核心判断逻辑：如果`cache_hit_rate`低且`blocks_allocated`接近完整前缀token数，则PrefixCache未命中。进一步判断：如果`free_blocks`长期接近0，则是容量不足；如果`free_blocks`充足但命中率仍低，则需检查公共前缀长度是否满足block size要求、Prefix Caching是否被正确启用、模型是否支持该特性。

## 对工具的改进建议

### ms-service-metric

当前在线监控已能查看KVCache水位和TTFT指标。建议增加PrefixCache命中率在线监控指标，直接展示`cache_hit_rate`的时间序列，便于实时观察缓存效果。增加PrefixCache未命中告警，当命中率持续偏低时自动输出风险提示。增加公共前缀长度分布统计，帮助用户判断请求前缀是否满足block size要求。

### msServiceProfiler

当前Profiler已能通过`request.csv`的`cache_hit_rate`字段确认缓存命中率。建议在`request.csv`中增加`cache_hit_blocks`和`cache_miss_blocks`字段，区分命中block数和未命中block数，便于精确分析缓存效果。在`kvcache.csv`中增加缓存淘汰原因字段（如LRU淘汰、请求结束释放、手动失效等），帮助分析缓存未命中的具体原因。在解析结果中增加PrefixCache诊断报告，自动检测公共前缀长度是否满足block size、是否存在`ascend_scheduler_config`冲突等常见配置问题。
