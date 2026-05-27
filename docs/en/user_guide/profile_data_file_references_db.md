# Profile Data File Reference (DB)<a name="EN-US_TOPIC_0000002477463220"></a>

Once the `msprof` command execution is complete, a `msprof_{Timestamp}.db` database file is generated to summarize all profile data. You are advised to use MindStudio Insight or a database development tool such as Navicat Premium to open the file. The profile data summarized by the .db file is as follows:

>[!NOTE]NOTE
>Profile data is displayed in tables in a .db file, and all data is mapped in numbers (for example, the operator name in the `opName` field is displayed as `194`). The mapping table between numbers and names is [STRING_IDS](#en-us_topic_0000002076410600_section116561584178).

**Units<a name="en-us_topic_0000002076410600_section1053014715533"></a>**

1. Time: local Unix time, in nanoseconds (ns)
2. Memory: Bytes
3. Bandwidth: Bytes/s
4. Frequency: MHz

**ENUM\_API\_TYPE<a name="en-us_topic_0000002076410600_section8466235152619"></a>**

Enumeration table.

There is no enable or disable option for this table. It is generated automatically during the export of `msprof_{timestamp}.db`.

**Table 1** Format

|Field|Type|Description|
|--|--|--|
|id|INTEGER|Index ID|
|name|TEXT|API type|

**Table 2** Content

|id|name|
|--|--|
|20000|acl|
|15000|model|
|10000|node|
|5500|communication|
|5000|runtime|
|50001|op|
|50002|queue|
|50003|trace|
|50004|mstx|

**ENUM\_MODULE<a name="en-us_topic_0000002076410600_section15914131919911"></a>**

Enumeration table.

There is no enable or disable option for this table. It is generated automatically during the export of `msprof_{timestamp}.db`.

**Table 3** Format

|Field|Type|Description|
|--|--|--|
|id|INTEGER|Index ID|
|name|TEXT|Component name|

**Table 4** Content

|id|name|
|--|--|
|0|SLOG|
|1|IDEDD|
|2|SCC|
|3|HCCL|
|4|FMK|
|5|CCU|
|6|DVPP|
|7|RUNTIME|
|8|CCE|
|9|HDC|
|10|DRV|
|11|NET|
|22|DEVMM|
|23|KERNEL|
|24|LIBMEDIA|
|25|CCECPU|
|27|ROS|
|28|HCCP|
|29|ROCE|
|30|TEFUSION|
|31|PROFILING|
|32|DP|
|33|APP|
|34|TS|
|35|TSDUMP|
|36|AICPU|
|37|LP|
|38|TDT|
|39|FE|
|40|MD|
|41|MB|
|42|ME|
|43|IMU|
|44|IMP|
|45|GE|
|47|CAMERA|
|48|ASCENDCL|
|49|TEEOS|
|50|ISP|
|51|SIS|
|52|HSM|
|53|DSS|
|54|PROCMGR|
|55|BBOX|
|56|AIVECTOR|
|57|TBE|
|58|FV|
|59|MDCMAP|
|60|TUNE|
|61|HSS|
|62|FFTS|
|63|OP|
|64|UDF|
|65|HICAID|
|66|TSYNC|
|67|AUDIO|
|68|TPRT|
|69|ASCENDCKERNEL|
|70|ASYS|
|71|ATRACE|
|72|RTC|
|73|SYSMONITOR|
|74|AMP|
|75|ADETECT|
|76|MBUFF|
|77|CUSTOM|

**ENUM\_HCCL\_DATA\_TYPE<a name="en-us_topic_0000002076410600_section899062216107"></a>**

Enumeration table.

There is no enable or disable option for this table. It is generated automatically during the export of `msprof_{timestamp}.db`.

**Table 5** Format

|Field|Type|Description|
|--|--|--|
|id|INTEGER|Index ID|
|name|TEXT|Communication data type|

**Table 6** Content

|id|name|
|--|--|
|0|INT8|
|1|INT16|
|2|INT32|
|3|FP16|
|4|FP32|
|5|INT64|
|6|UINT64|
|7|UINT8|
|8|UINT16|
|9|UINT32|
|10|FP64|
|11|BFP16|
|12|INT128|
|255|RESERVED|
|65534|N/A|
|65535|INVALID_TYPE|

**ENUM\_HCCL\_LINK\_TYPE<a name="en-us_topic_0000002076410600_section1843687151113"></a>**

Enumeration table.

There is no enable or disable option for this table. It is generated automatically during the export of `msprof_{timestamp}.db`.

**Table 7** Format

|Field|Type|Description|
|--|--|--|
|id|INTEGER|Index ID|
|name|TEXT|Communication link type|

**Table 8** Content

|id|name|
|--|--|
|0|ON_CHIP|
|1|HCCS|
|2|PCIE|
|3|ROCE|
|4|SIO|
|5|HCCS_SW|
|6|STANDARD_ROCE|
|255|RESERVED|
|65534|N/A|
|65535|INVALID_TYPE|

**ENUM\_HCCL\_TRANSPORT\_TYPE<a name="en-us_topic_0000002076410600_section64765193176"></a>**

Enumeration table.

There is no enable or disable option for this table. It is generated automatically during the export of `msprof_{timestamp}.db`.

**Table 9** Format

|Field|Type|Description|
|--|--|--|
|id|INTEGER|Index ID|
|name|TEXT|Communication transmission type|

**Table 10** Content

|id|name|
|--|--|
|0|SDMA|
|1|RDMA|
|2|LOCAL|
|255|RESERVED|
|65534|N/A|
|65535|INVALID_TYPE|

**ENUM\_HCCL\_RDMA\_TYPE<a name="en-us_topic_0000002076410600_section2375181481718"></a>**

Enumeration table.

There is no enable or disable option for this table. It is generated automatically during the export of `msprof_{timestamp}.db`.

**Table 11** Format

|Field|Type|Description|
|--|--|--|
|id|INTEGER|Index ID|
|name|TEXT|Communication RDMA type|

**Table 12** Content

|id|name|
|--|--|
|0|RDMA_SEND_NOTIFY|
|1|RDMA_SEND_PAYLOAD|
|255|RESERVED|
|65534|N/A|
|65535|INVALID_TYPE|

**ENUM\_MSTX\_EVENT\_TYPE<a name="en-us_topic_0000002076410600_section1752102012146"></a>**

Enumeration table.

There is no enable or disable option for this table. It is generated automatically during the export of `msprof_{timestamp}.db`.

**Table 13** Format

|Field| Type    |Description|
|--|--------|--|
|id| INTEGER |Index, which is the ID for the event type in the TX instrumentation data on the host|
|name| TEXT   |Event type of the TX instrumentation data on the host|

**Table 14** Content

|id|name|
|--|--|
|0|marker|
|1|push/pop|
|2|start/end|
|3|marker_ex|

**ENUM\_MEMCPY\_OPERATION<a name="en-us_topic_0000002076410600_section1663131744913"></a>**

Enumeration table.

There is no enable or disable option for this table. It is generated automatically during the export of `msprof_{timestamp}.db`.

**Table 15** Format

|Field|Type|Description|
|--|--|--|
|id|INTEGER|Primary key ID|
|name|TEXT|Copy type|

**Table 16** Content

|id|name|
|--|--|
|0|host to host|
|1|host to device|
|2|device to host|
|3|device to device|
|4|managed memory|
|5|addr device to device|
|6|host to device ex|
|7|device to host ex|
|65535|other|

**STRING\_IDS<a name="en-us_topic_0000002076410600_section116561584178"></a>**

Stores ID-to-string mappings.

There is no enable or disable option for this table.

**Table 17** Format

|Field|Type|Description|
|--|--|--|
|id|INTEGER|Index, string ID|
|value|TEXT|String value|

**SESSION\_TIME\_INFO<a name="en-us_topic_0000002076410600_section153758441710"></a>**

Stores the start and end time of the profile data. If the collection process does not exit normally, no end time is recorded.

There is no enable or disable option for this table.

**Table 18** Format

|Field|Type|Description|
|--|--|--|
|startTimeNs|INTEGER|Unix time when a task starts (ns)|
|endTimeNs|INTEGER|Unix time when a task ends (ns)|

**NPU_INFO<a name="en-us_topic_0000002076410600_section966211019179"></a>**

Stores the chip models corresponding to the device IDs.

There is no enable or disable option for this table.

**Table 19** Format

|Field|Type|Description|
|--|--|--|
|id|INTEGER|Device ID. The value `–1` indicates that the device ID was not collected.|
|name|TEXT|Chip model of the device.|

**HOST\_INFO<a name="en-us_topic_0000002076410600_section57348569166"></a>**

Stores the unique host IDs and names.

There is no enable or disable option for this table.

**Table 20** Format

|Field|Type|Description|
|--|--|--|
|hostUid|TEXT|Unique host ID|
|hostName|TEXT|Host name, such as localhost|

**TASK<a name="en-us_topic_0000002076410600_section16993252171612"></a>**

Stores information about all operator tasks executed by the hardware.

This table is controlled by `--task-time`.

**Table 21** Format

|Field|Type|Description|
|--|--|--|
|startNs|INTEGER|Start time of the operator task (ns). This field and `globalTaskId` form a composite index named `TaskIndex`.|
|endNs|INTEGER|End time of the operator task (ns).|
|deviceId|INTEGER|Device ID corresponding to the operator task.|
|connectionId|INTEGER|ID of the host-device connection.|
|globalTaskId|INTEGER|Global task ID that uniquely identifies a global operator task. This field and `startNs` form a composite index named `TaskIndex`.|
|globalPid|INTEGER|PID of the operator task.|
|taskType|INTEGER|Type of the accelerator that executes the operator.|
|contextId|INTEGER|ID used to distinguish small operators within a subgraph, which is common for MIX operators and FFTS+ tasks.|
|streamId|INTEGER|Stream ID of the operator task.|
|taskId|INTEGER|ID of the operator task.|
|modelId|INTEGER|Model ID of the operator task.|

**COMPUTE\_TASK\_INFO<a name="en-us_topic_0000002076410600_section1724311477163"></a>**

Stores information about compute operators.

This table is controlled by `--task-time`.

**Table 22** Format

|Field|Type|Description|
|--|--|--|
|name|INTEGER|Operator name, which maps to `STRING_IDS(name)`.|
|globalTaskId|INTEGER|Global operator task ID, which is used as an index to associate with the `TASK` table.|
|blockNum|INTEGER|Number of operator task blocks, which corresponds to the number of cores used during operator execution.|
|mixBlockNum|INTEGER|`BlockNum` value of the secondary accelerator of the MIX operator.|
|taskType|INTEGER|Type of the accelerator that executes the operator on the host. The value maps to `STRING_IDS(taskType)`.|
|opType|INTEGER|Operator type, which maps to `STRING_IDS(opType)`.|
|inputFormats|INTEGER|Input data formats of the operator. The value maps to `STRING_IDS(inputFormats)`.|
|inputDataTypes|INTEGER|Input data types of the operator. The value maps to `STRING_IDS(inputDataTypes)`.|
|inputShapes|INTEGER|Input shapes of the operator. The value maps to `STRING_IDS(inputShapes)`.|
|outputFormats|INTEGER|Output data formats of the operator. The value maps to `STRING_IDS(outputFormats)`.|
|outputDataTypes|INTEGER|Output data types of the operator. The value maps to `STRING_IDS(outputDataTypes)`.|
|outputShapes|INTEGER|Output shapes of the operator. The value maps to `STRING_IDS(outputShapes)`.|
|attrInfo|INTEGER|Operator attribute information used to describe the operator shape and custom operator parameters. The value maps to `STRING_IDS(attrInfo)`.|
|opState|INTEGER|Indicates whether an operator is dynamic or static. Valid values: `dynamic`, `static`, and `N/A` (unidentified scenario or operator). The value maps to `STRING_IDS(opState)`.|
|hf32Eligible|INTEGER|Indicates whether the HF32 precision flag is used. Valid values: `YES` (used), `NO` (not used), and `N/A` (unidentified scenario or operator). The value maps to `STRING_IDS(hf32Eligible)`.|

**COMMUNICATION\_TASK\_INFO<a name="en-us_topic_0000002076410600_section1656344215167"></a>**

Stores information about small communication operators.

The `--task-time`, `--hccl`, and `--ascendcl` options control the collection of the corresponding data. Data is valid when `--task-time` is set to a value other than `l0`. This table is generated automatically when communication data is available.

**Table 23** Format

|Field|Type| Description                                                                    |
|--|--|------------------------------------------------------------------------|
|name|INTEGER| Operator name, which maps to `STRING_IDS(name)`.                                                  |
|globalTaskId|INTEGER| Global operator task ID, which is used as an index named `CommunicationTaskIndex` to associate with the `TASK` table.                      |
|taskType|INTEGER| Operator task type, which maps to `STRING_IDS(taskType)`.                                             |
|planeId|INTEGER| Network plane ID.                                                                |
|groupName|INTEGER| Communication group name, which maps to `STRING_IDS(groupName)`.                                             |
|notifyId|INTEGER| Unique notify ID.                                                            |
|rdmaType|INTEGER| RDMA type. Valid values: `RDMASendNotify` or `RDMASendPayload`. The value maps to `ENUM_HCCL_RDMA_TYPE(rdmaType)`.|
|srcRank|INTEGER| Source rank.                                                                 |
|dstRank|INTEGER| Destination rank.                                                                |
|transportType|INTEGER| Transmission type. Valid values: `LOCAL`, `SDMA`, and `RDMA`. The value maps to `ENUM_HCCL_TRANSPORT_TYPE(transportType)`.       |
|size|INTEGER| Data size (bytes).                                                            |
|dataType|INTEGER| Data type, which maps to `ENUM_HCCL_DATA_TYPE(dataType)`.                                    |
|linkType|INTEGER| Link type. Valid values: `HCCS`, `PCIe`, and `RoCE`. The value maps to `ENUM_HCCL_LINK_TYPE(linkType)`.                  |
|opId|INTEGER| ID of the corresponding large operator. This field is used to associate with the `COMMUNICATION_OP` table.                                        |
|isMaster|INTEGER| Indicates whether the operator is a primary or secondary stream communication operator. The primary stream operator is used for analysis. Valid values:<br>`0`: secondary stream<br>`1`: primary stream                                    |
|bandwidth|NUMERIC| Bandwidth data of the small communication operator (bytes/s).                                                |

**COMMUNICATION\_OP<a name="en-us_topic_0000002076410600_section1238183491611"></a>**

Stores information about large communication operators.

The `--task-time` and `--hccl` options control the collection of the corresponding data. This table is generated automatically when communication data is available.

**Table 24** Format

| Field        |Type|Description|
|-------------|--|--|
| opName      |INTEGER|Operator name, which maps to `STRING_IDS(opName)`, such as `hcom_allReduce__428_0_1`.|
| startNs     |INTEGER|Start time of the large communication operator (ns).|
| endNs       |INTEGER|End time of the large communication operator (ns).|
| connectionId |INTEGER|ID of the host-device connection.|
| groupName   |INTEGER|Communication group name, which maps to `STRING_IDS(groupName)`, such as `10.170.22.98%enp67s0f5_60000_0_1708156014257149`.|
| opId        |INTEGER|ID of the corresponding large communication operator, which is used as an index to associate with the `COMMUNICATION_TASK_INFO` table.|
| relay       |INTEGER|Rail borrowing flag.|
| retry       |INTEGER|Retransmission flag.|
| dataType    |INTEGER|Data type transmitted by the large communication operator (such as INT8 and FP32). The value maps to `ENUM_HCCL_DATA_TYPE(dataType)`.|
| algType     |INTEGER|Algorithm used by the communication operator, which may consist of multiple phases (such as `HD-MESH`). The value maps to `STRING_IDS(algType)`.|
| count       |NUMERIC|Volume of the `dataType` data transmitted by the operator.|
| opType      |INTEGER|Operator type, which maps to `STRING_IDS(opType)`, such as `hcom_broadcast_`.|
| deviceId    |INTEGER|Device ID.|

**CANN\_API<a name="en-us_topic_0000002076410600_section2740528151615"></a>**

Stores data about CANN APIs.

This table is controlled by `--ascendcl`.

**Table 25** Format

|Field|Type|Description|
|--|--|--|
|startNs|INTEGER|Start time of the API call (ns).|
|endNs|INTEGER|End time of the API call (ns).|
|type|INTEGER|API type, which maps to `ENUM_API_TYPE(type)`.|
|globalTid|INTEGER|Global thread ID (TID) of the API. High-order 32 bits: PID; low-order 32 bits: TID.|
|connectionId|INTEGER|Connection ID, which is used as an index to associate the `TASK` table with the `COMMUNICATION_OP` table.|
|name|INTEGER|API name, which maps to `STRING_IDS(name)`.|

**QOS<a name="en-us_topic_0000002076410600_section74321720145411"></a>**

Stores QoS data.

This table is controlled by `--sys-hardware-mem` and `--sys-hardware-mem-freq`.

**Table 26** Format

|Field|Type|Description|
|--|--|--|
|deviceId|INTEGER|Device ID|
|eventName|NUMERIC|QoS event name, which maps to `STRING_IDS(eventName)`|
|bandwidth|NUMERIC|Bandwidth at the time of the QoS event (bytes/s)|
|timestampNs|NUMERIC|Local time (ns)|

**AICORE\_FREQ<a name="en-us_topic_0000002076410600_section12814131518375"></a>**

Stores AI Core frequency information.

There is no enable or disable option for this table. It is generated automatically during the export of `msprof_{timestamp}.db`.

**Table 27** Format

| Field        |Type|Description|
|-------------|--|--|
| deviceId    |INTEGER|Device ID|
| timestampNs |NUMERIC|Local timestamp of frequency change (ns)|
| freq        |INTEGER|AI Core frequency (MHz)|

**ACC\_PMU<a name="en-us_topic_0000002076410600_section68271524191613"></a>**

Stores the ACC\_PMU data.

This table is controlled by `--sys-hardware-mem` and `--sys-hardware-mem-freq`.

**Table 28** Format

|Field|Type|Description|
|--|--|--|
|accId|INTEGER|Accelerator ID|
|readBwLevel|INTEGER|Read bandwidth level of the DVPP and DSA accelerators|
|writeBwLevel|INTEGER|Write bandwidth level of the DVPP and DSA accelerators|
|readOstLevel|INTEGER|Read concurrency level of the DVPP and DSA accelerators|
|writeOstLevel|INTEGER|Write concurrency level of the DVPP and DSA accelerators|
|timestampNs|NUMERIC|Local time (ns)|
|deviceId|INTEGER|Device ID|

**SOC\_BANDWIDTH\_LEVEL<a name="en-us_topic_0000002076410600_section14551151619160"></a>**

Stores SoC bandwidth level information.

This table is controlled by `--sys-hardware-mem` and `--sys-hardware-mem-freq`.

**Table 29** Format

|Field|Type|Description|
|--|--|--|
|l2BufferBwLevel|INTEGER|L2 buffer bandwidth level|
|mataBwLevel|INTEGER|Mata bandwidth level|
|timestampNs|NUMERIC|Local time (ns)|
|deviceId|INTEGER|Device ID|

**NIC<a name="en-us_topic_0000002076410600_section65681562168"></a>**

Stores NIC information for each time node.

Control options:

- `--sys-io-profiling` and `--sys-io-sampling-freq` of the `msprof` command
- `sys_io` of Ascend PyTorch Profiler

**Table 30** Format

|Field|Type|Description|
|--|--|--|
|deviceId|INTEGER|Device ID|
|timestampNs|INTEGER|Local time (ns)|
|bandwidth|INTEGER|Bandwidth (bytes/s)|
|rxPacketRate|NUMERIC|Packet receiving rate (packets/s)|
|rxByteRate|NUMERIC|Byte receiving rate (bytes/s)|
|rxPackets|INTEGER|Total number of received packets|
|rxBytes|INTEGER|Total number of received bytes|
|rxErrors|INTEGER|Total number of received error packets|
|rxDropped|INTEGER|Total number of received dropped packets|
|txPacketRate|NUMERIC|Packet transmission rate (packets/s)|
|txByteRate|NUMERIC|Byte transmission rate (bytes/s)|
|txPackets|INTEGER|Total number of transmitted packets|
|txBytes|INTEGER|Total number of transmitted bytes|
|txErrors|INTEGER|Total number of transmitted error packets|
|txDropped|INTEGER|Total number of transmitted dropped packets|
|funcId|INTEGER|Port number|

**ROCE<a name="en-us_topic_0000002076410600_section1246285716152"></a>**

Stores the bandwidth of the RoCE communication interface.

Control options:

- `--sys-io-profiling` and `--sys-io-sampling-freq` of the `msprof` command
- `sys_io` of Ascend PyTorch Profiler

**Table 31** Format

|Field|Type|Description|
|--|--|--|
|deviceId|INTEGER|Device ID|
|timestampNs|INTEGER|Local time (ns)|
|bandwidth|INTEGER|Bandwidth (bytes/s)|
|rxPacketRate|NUMERIC|Packet receiving rate (packets/s)|
|rxByteRate|NUMERIC|Byte receiving rate (bytes/s)|
|rxPackets|INTEGER|Total number of received packets|
|rxBytes|INTEGER|Total number of received bytes|
|rxErrors|INTEGER|Total number of received error packets|
|rxDropped|INTEGER|Total number of received dropped packets|
|txPacketRate|NUMERIC|Packet transmission rate (packets/s)|
|txByteRate|NUMERIC|Byte transmission rate (bytes/s)|
|txPackets|INTEGER|Total number of transmitted packets|
|txBytes|INTEGER|Total number of transmitted bytes|
|txErrors|INTEGER|Total number of transmitted error packets|
|txDropped|INTEGER|Total number of transmitted dropped packets|
|funcId|INTEGER|Port number|

**LLC<a name="en-us_topic_0000002076410600_section119051348181519"></a>**

Stores the L3 cache bandwidth.

This table is controlled by `--sys-hardware-mem` and `--sys-hardware-mem-freq`.

**Table 32** Format

|Field|Type|Description|
|--|--|--|
|deviceId|INTEGER|Device ID.|
|llcId|INTEGER|L3 cache ID.|
|timestampNs|INTEGER|Local time (ns).|
|hitRate|NUMERIC|L3 cache hit rate (100%).|
|throughput|NUMERIC|L3 cache throughput (bytes/s).|
|mode|INTEGER|Mode, which is used to distinguish reads and writes. The value maps to `STRING_IDS(mode)`.|

**TASK\_PMU\_INFO<a name="en-us_topic_0000002076410600_section72694413158"></a>**

Stores the PMU data of compute operators.

Control options:

- The `--ai-core` and `--aic-mode=task-based` options of the `msprof` command control the generation of this table, while the `--aic-metrics` option controls the specific data to be collected.
- `aic_metrics` of Ascend PyTorch Profiler
- `aic_metrics` of MindSpore Profiler

Only Atlas 200I/500 A2 inference products, Atlas A2 training products, and Atlas A2 inference products support the collection of such data.

**Table 33** Format

|Field|Type|Description|
|--|--|--|
|globalTaskId|INTEGER|Global operator task ID, which is used to associate with the `TASK` table|
|name|INTEGER|PMU metric name, which maps to `STRING_IDS(name)`|
|value|NUMERIC|Value of the corresponding metric|

**SAMPLE\_PMU\_TIMELINE<a name="en-us_topic_0000002076410600_section166020382157"></a>**

Stores sample-based PMU data, which is used to display timeline data.

Control options:

- The `--ai-core` and `--aic-mode=sample-based` options of the `msprof` command control the generation of this table, while the `--aic-metrics` option controls the specific data to be collected.
- `aic_metrics` of Ascend PyTorch Profiler
- `aic_metrics` of MindSpore Profiler

**Table 34** Format

|Field|Type|Description|
|--|--|--|
|deviceId|INTEGER|Device ID|
|timestampNs|INTEGER|Local time (ns)|
|totalCycle|INTEGER|Number of cycles of the corresponding core in the time slice|
|usage|NUMERIC|Utilization of the corresponding core in the time slice (%)|
|freq|NUMERIC|Frequency of the corresponding core in the time slice (MHz)|
|coreId|INTEGER|Core ID|
|coreType|INTEGER|Core type (AIC or AIV), which maps to `STRING_IDS(coreType)`|

**SAMPLE\_PMU\_SUMMARY<a name="en-us_topic_0000002076410600_section233113319159"></a>**

Stores sample-based PMU data, which is used to display summary data.

Control options:

- The `--ai-core` and `--aic-mode=sample-based` options of the `msprof` command control the generation of this table, while the `--aic-metrics` option controls the specific data to be collected.
- `aic_metrics` of Ascend PyTorch Profiler
- `aic_metrics` of MindSpore Profiler

**Table 35** Format

|Field|Type|Description|
|--|--|--|
|deviceId|INTEGER|Device ID|
|metric|INTEGER|PMU metric name, which maps to `STRING_IDS(metric)`|
|value|NUMERIC|Value of the corresponding metric|
|coreId|INTEGER|Core ID|
|coreType|INTEGER|Core type (AIC or AIV), which maps to `STRING_IDS(coreType)`|

**NPU\_MEM<a name="en-us_topic_0000002076410600_section29991927141518"></a>**

Stores NPU memory usage data.

This table is controlled by `--sys-hardware-mem` and `--sys-hardware-mem-freq`.

**Table 36** Format

|Field|Type|Description|
|--|--|--|
|type|INTEGER|Event type (`app` or `device`), which maps to `STRING_IDS(type)`|
|ddr|NUMERIC|DDR usage (bytes)|
|hbm|NUMERIC|On-chip memory usage (bytes)|
|timestampNs|INTEGER|Local time (ns)|
|deviceId|INTEGER|Device ID|

**NPU\_MODULE\_MEM<a name="en-us_topic_0000002076410600_section1612492317150"></a>**

Stores memory usage of the NPU component.

This table is controlled by `--sys-hardware-mem` and `--sys-hardware-mem-freq`.

**Table 37** Format

|Field|Type|Description|
|--|--|--|
|moduleId|INTEGER|Component type, which maps to `ENUM_MODULE(moduleId)`|
|timestampNs|INTEGER|Local time (ns)|
|totalReserved|NUMERIC|Memory usage (bytes)|
|deviceId|INTEGER|Device ID|

**NPU\_OP\_MEM<a name="en-us_topic_0000002076410600_section138661911511"></a>**

Stores memory usage of CANN operators (supported only by GE operators)

This table is controlled by `--task-memory`.

**Table 38** Format

|Field|Type|Description|
|--|--|--|
|operatorName|INTEGER|Operator name, which maps to `STRING_IDS(operatorName)`.|
|addr|INTEGER|Initial address for memory allocation and release.|
|type|INTEGER|Type used to distinguish memory allocation and release. The value maps to `STRING_IDS(type)`.|
|size|INTEGER|Size of the allocated memory (bytes).|
|timestampNs|INTEGER|Local time (ns).|
|globalTid|INTEGER|Global TID of the record. High-order 32 bits: PID; low-order 32 bits: TID.|
|totalAllocate|NUMERIC|Total size of the allocated memory (bytes).|
|totalReserve|NUMERIC|Total size of the held memory (bytes).|
|component|INTEGER|Component name, which maps to `STRING_IDS(component)`.|
|deviceId|INTEGER|Device ID.|

**HBM<a name="en-us_topic_0000002076410600_section1045421312151"></a>**

Stores on-chip memory read/write speed data.

This table is controlled by `--sys-hardware-mem` and `--sys-hardware-mem-freq`.

**Table 39** Format

|Field|Type|Description|
|--|--|--|
|deviceId|INTEGER|Device ID.|
|timestampNs|INTEGER|Local time (ns).|
|bandwidth|NUMERIC|Bandwidth (bytes/s).|
|hbmId|INTEGER|ID of the on-chip memory access unit.|
|type|INTEGER|Type used to distinguish reads and writes. The value maps to `STRING_IDS(type)`.|

**DDR<a name="en-us_topic_0000002076410600_section56231221154"></a>**

Stores on-chip memory read/write speed data.

This table is controlled by `--sys-hardware-mem` and `--sys-hardware-mem-freq`.

**Table 40** Format

|Field|Type|Description|
|--|--|--|
|deviceId|INTEGER|Device ID|
|timestampNs|INTEGER|Local time (ns)|
|read|NUMERIC|Memory read bandwidth (bytes/s)|
|write|NUMERIC|Memory write bandwidth (bytes/s)|

**HCCS<a name="en-us_topic_0000002076410600_section81241447141420"></a>**

Stores HCCS bandwidth data.

Control options:

- `--sys-interconnection-profiling` and `--sys-interconnection-freq` of the `msprof` command
- `sys_interconnection` of Ascend PyTorch Profiler

**Table 41** Format

|Field|Type|Description|
|--|--|--|
|deviceId|INTEGER|Device ID|
|timestampNs|INTEGER|Local time (ns)|
|txThroughput|NUMERIC|Transmit bandwidth (bytes/s)|
|rxThroughput|NUMERIC|Receive bandwidth (bytes/s)|

**PCIE<a name="en-us_topic_0000002076410600_section760424014146"></a>**

Stores PCIe bandwidth data.

Control options:

- `--sys-interconnection-profiling` and `--sys-interconnection-freq` of the `msprof` command
- `sys_interconnection` of Ascend PyTorch Profiler

**Table 42** Format

|Field|Type|Description|
|--|--|--|
|deviceId|INTEGER|Device ID|
|timestampNs|INTEGER|Local time (ns)|
|txPostMin|NUMERIC|Minimum bandwidth for transmitting PCIe posted data at the TX side (bytes/s)|
|txPostMax|NUMERIC|Maximum bandwidth for transmitting PCIe posted data at the TX side (bytes/s)|
|txPostAvg|NUMERIC|Average bandwidth for transmitting PCIe posted data at the TX side (bytes/s)|
|txNonpostMin|NUMERIC|Minimum bandwidth for transmitting PCIe non-posted data at the TX side (bytes/s)|
|txNonpostMax|NUMERIC|Maximum bandwidth for transmitting PCIe non-posted data at the TX side (bytes/s)|
|txNonpostAvg|NUMERIC|Average bandwidth for transmitting PCIe non-posted data at the TX side (bytes/s)|
|txCplMin|NUMERIC|Minimum throughput of completion packets received at the TX side for write requests (bytes/s)|
|txCplMax|NUMERIC|Maximum throughput of completion packets received at the TX side for write requests (bytes/s)|
|txCplAvg|NUMERIC|Average throughput of completion packets received at the TX side for write requests (bytes/s)|
|txNonpostLatencyMin|NUMERIC|Minimum transmission latency in PCIe Non-Post mode at the TX side (ns)|
|txNonpostLatencyMax|NUMERIC|Maximum transmission latency in PCIe Non-Post mode at the TX side (ns)|
|txNonpostLatencyAvg|NUMERIC|Average transmission latency in PCIe Non-Post mode at the TX side (ns)|
|rxPostMin|NUMERIC|Minimum bandwidth for receiving PCIe posted data at the RX side (bytes/s)|
|rxPostMax|NUMERIC|Maximum bandwidth for receiving PCIe posted data at the RX side (bytes/s)|
|rxPostAvg|NUMERIC|Average bandwidth for receiving PCIe posted data at the RX side (bytes/s)|
|rxNonpostMin|NUMERIC|Minimum bandwidth for receiving PCIe non-posted data at the RX side (bytes/s)|
|rxNonpostMax|NUMERIC|Maximum bandwidth for receiving PCIe non-posted data at the RX side (bytes/s)|
|rxNonpostAvg|NUMERIC|Average bandwidth for receiving PCIe non-posted data at the RX side (bytes/s)|
|rxCplMin|NUMERIC|Minimum throughput of completion packets received at the RX side for write requests (bytes/s)|
|rxCplMax|NUMERIC|Maximum throughput of completion packets received at the RX side for write requests (bytes/s)|
|rxCplAvg|NUMERIC|Average throughput of completion packets received at the RX side for write requests (bytes/s)|

**META\_DATA<a name="en-us_topic_0000002076410600_section13792933201413"></a>**

Stores basic data. Currently, only version information is stored.

There is no enable or disable option for this table. It is generated automatically during the export of `msprof_{timestamp}.db`.

**Table 43** Format

|Field|Type|Description|
|--|--|--|
|name|TEXT|Field|
|value|TEXT|Value|

**Table 44** Content

|Name|Description|
|--|--|
|SCHEMA_VERSION|Full version number, such as `1.0.2`.|
|SCHEMA_VERSION_MAJOR|Major version number, such as `1`. The value changes only when the database format is rewritten or reconstructed.|
|SCHEMA_VERSION_MINOR|Minor version number, such as `0`. When the column or type is changed, compatibility issues occur.|
|SCHEMA_VERSION_MICRO|Micro version number, such as `2`. The version number changes when the table is updated. There is no compatibility issue.|

**MSTX\_EVENTS<a name="en-us_topic_0000002076410600_section13564192610142"></a>**

Stores host-side data collected by mstx APIs. Device-side data is aggregated in the `TASK` table.

The `--msproftx` option controls the table output, while the mstx APIs control the collection of such data.

**Table 45** Format

| Field         |Type|Description|
|--------------|--|--|
| startNs      |INTEGER|Start time of host-side TX instrumentation data (ns).|
| endNs        |INTEGER|End time of host-side TX instrumentation data (ns).|
| eventType    |INTEGER|Type of host-side TX instrumentation data. The value maps to `ENUM_MSTX_EVENT_TYPE(eventType)`.|
| rangeId      |INTEGER|Range ID of the host-side range-type TX instrumentation data.|
| category     |INTEGER|Category ID of the host-side TX instrumentation data.|
| message      |INTEGER|Message carried in host-side TX instrumentation data. The value maps to `STRING_IDS(message)`.|
| globalTid    |INTEGER|Global TID of the thread that started host-side TX instrumentation.|
| endGlobalTid |INTEGER|Global TID of the thread that ended host-side TX instrumentation.|
| domainId     |INTEGER|Domain ID for host-side TX instrumentation data.|
| connectionId |INTEGER|Connection ID for host-side TX instrumentation data. The value maps to `TASK(connectionId)`.|

**COMMUNICATION\_SCHEDULE\_TASK\_INFO<a name="en-us_topic_0000002076410600_section16366112743116"></a>**

Stores communication scheduling information. Currently, only the description of AICPU communication operators is supported.

There is no enable or disable option for this table. It is generated automatically during the export of `msprof_{timestamp}.db`. The collection environment must contain AICPU communication operators.

**Table 46** Format

|Field|Type|Description|
|--|--|--|
|name|INTEGER|Operator name, which maps to `STRING_IDS(name)`.|
|globalTaskId|INTEGER|Global operator task ID, which is used as the primary key to associate with the `TASK` table.|
|taskType|INTEGER|Type of the accelerator that executes the operator on the host. The value maps to `STRING_IDS(taskType)`.|
|opType|INTEGER|Operator type, which maps to `STRING_IDS(opType)`.|

**MEMCPY\_INFO<a name="en-us_topic_0000002076410600_section190813398529"></a>**

Stores information about the copied data size and copy direction of memcpy operators.

This table is controlled by `--runtime-api`.

**Table 47** Format

|Field|Type|Description|
|--|--|--|
|globalTaskId|NUMERIC|Global operator task ID, which is used as the primary key to associate with the `TASK` table|
|size|NUMERIC|Copied data size|
|memcpyOperation|NUMERIC|Copy type, which maps to `STRING_IDS(memcpyDirection)`|

**CPU\_USAGE<a name="en-us_topic_0000002076410600_section730161613343"></a>**

Stores information about host-side CPU utilization.

This table is controlled by `--host-sys=cpu`.

**Table 48** Format

|Field|Type|Description|
|--|--|--|
|timestampNs|NUMERIC|Local sampling time (ns)|
|cpuId|NUMERIC|CPU ID|
|usage|NUMERIC|CPU utilization (%)|

**HOST\_MEM\_USAGE<a name="en-us_topic_0000002076410600_section17298155016408"></a>**

Stores information about host-side memory usage.

This table is controlled by `--host-sys=mem`.

**Table 49** Format

|Field|Type|Description|
|--|--|--|
|timestampNs|NUMERIC|Local sampling time (ns)|
|usage|NUMERIC|Memory usage (%)|

**HOST\_DISK\_USAGE<a name="en-us_topic_0000002076410600_section32136221402"></a>**

Stores information about host-side drive I/O usage.

This table is controlled by `--host-sys=disk`.

**Table 50** Format

|Field|Type|Description|
|--|--|--|
|timestampNs|NUMERIC|Local sampling time (ns)|
|readRate|NUMERIC|Drive read rate (bytes/s)|
|writeRate|NUMERIC|Drive write rate (bytes/s)|
|usage|NUMERIC|Drive usage (%)|

**HOST\_NETWORK\_USAGE<a name="en-us_topic_0000002076410600_section223093643915"></a>**

Stores information about host-side system-level network I/O usage.

This table is controlled by `--host-sys=network`.

**Table 51** Format

|Field|Type|Description|
|--|--|--|
|timestampNs|NUMERIC|Local sampling time (ns)|
|usage|NUMERIC|Network I/O usage (%)|
|speed|NUMERIC|Network usage rate (bytes/s)|

**OSRT\_API<a name="en-us_topic_0000002076410600_section7238121863614"></a>**

Stores host-side `syscall` and `pthreadcall` data.

This table is controlled by `--host-sys=osrt`.

**Table 52** Format

|Field|Type|Description|
|--|--|--|
|name|INTEGER|OS runtime API name.|
|globalTid|NUMERIC|Global TID of the thread that executes the API call. High-order 32 bits: PID; low-order 32 bits: TID.|
|startNs|INTEGER|Start time of the API call (ns).|
|endNs|INTEGER|End time of the API call (ns).|

**NETDEV\_STATS<a name="en-us_topic_0000002076410600_section2080324381117"></a>**

Stores hardware-sampled bandwidth metrics to identify communication issues. Specifically, these metrics serve as overview items for preliminary troubleshooting. For example, if the communication duration is abnormal, prioritize checking for network congestion.

Control options:

- `--sys-io-profiling` and `--sys-io-sampling-freq` of the `msprof` command
- `sys_io` of Ascend PyTorch Profiler
- `sys_io` of MindSpore Profiler

**Table 53** Format

|Field|Type|Description|
|--|--|--|
|deviceId|INTEGER|Device ID|
|timestampNs|INTEGER|Local sampling time (ns)|
|macTxPfcPkt|INTEGER|Number of PFC frames transmitted by the Media Access Control (MAC) layer|
|macRxPfcPkt|INTEGER|Number of PFC frames received by the MAC layer|
|macTxByte|INTEGER|Number of bytes transmitted by the MAC layer|
|macTxBandwidth|NUMERIC|TX bandwidth at the MAC layer (bytes/s)|
|macRxByte|INTEGER|Number of bytes received by the MAC layer|
|macRxBandwidth|NUMERIC|RX bandwidth at the MAC layer (bytes/s)|
|macTxBadByte|INTEGER|Total bytes of bad packets transmitted by the MAC layer|
|macRxBadByte|INTEGER|Total bytes of bad packets received by the MAC layer|
|roceTxPkt|INTEGER|Number of packets transmitted by the RoCEE|
|roceRxPkt|INTEGER|Number of packets received by the RoCEE|
|roceTxErrPkt|INTEGER|Total number of bad packets transmitted by the RoCEE|
|roceRxErrPkt|INTEGER|Number of bad packets received by the RoCEE|
|roceTxCnpPkt|INTEGER|Number of CNP packets transmitted by the RoCEE|
|roceRxCnpPkt|INTEGER|Number of CNP packets received by the RoCEE|
|roceNewPktRty|INTEGER|Number of packets retransmitted by the RoCEE due to timeout|
|nicTxByte|INTEGER|Number of bytes transmitted by the NIC|
|nicTxBandwidth|NUMERIC|TX bandwidth of the NIC (bytes/s)|
|nicRxByte|INTEGER|Number of bytes received by the NIC|
|nicRxBandwidth|NUMERIC|RX bandwidth of the NIC (bytes/s)|

**RANK\_DEVICE\_MAP<a name="en-us_topic_0000002076410600_section75444171535"></a>**

Stores the mapping between `rankId` and `deviceId`.

There is no enable or disable option for this table. It is generated automatically during the export of `ascend_pytorch_profiler_{Rank_ID}.db`.

**Table 54** Format

|Field|Type|Description|
|--|--|--|
|rankId|INTEGER|The value is fixed at `–1`.|
|deviceId|INTEGER|Device ID on the node. The value `–1` indicates that the device ID was not collected.|
