# Host 诊断调优工具

## 简介

当前 AI 训练、推理业务场景中，Host侧（CPU）的任务下发（如算子调度、内存分配）与Device侧（NPU）的任务执行是异步进行的。当Host侧任务下发耗时超过Device侧任务执行耗时时，Device会因等待新任务而处于空闲状态，形成性能瓶颈，即HostBound问题。

针对以上问题，我们设计了Host侧的诊断调优工具，提供简单、易用的绑核能力，通过将进程和线程分别绑定在不同CPU上执行以减少互相之间的干扰与资源竞争。

## 使用前准备

1. 获取仓库中提供的参数解析、绑核脚本：[entrance.py](./entrance.py)，[cpu_binder.py](./cpu_binder.py)

## 功能介绍

### 自定义绑核功能

#### 功能说明

提供自定义绑核能力，根据用户输入json中的配置方案完成进程/线程级绑核；缺省输入时以经验最优方案进行绑核: 

  - 为每张卡的关键线程 acl_thread/release_thread 单独分配一个 CPU 核，dev[i]_sq_task 单独分配一个 CPU 核（宿主机可绑，未查询到时跳过），算子相关中断 sq_send_trigger_irq 和 cq_update_irq 各单独分配一个 CPU 核（需要拥有/proc目录写权限，权限不足时跳过），其余推理线程共同分配到 6 个 CPU 核上，即每张 NPU 卡绑定到 11 个 CPU 核（分配 CPU 时考虑 NPU 亲和及跨 NUMA 内存访问时延）。

#### 命令格式

```bash
python3 entrance.py bind [-l] [-c <config_path>]
```

#### 参数说明

| 参数             | 可选/必选 | 说明                                              |
|----------------|-------|-------------------------------------------------|
| -l/--log-level | 可选    | 指定打印时的日志等级，类型为int，可选值为[0, 1, 2, 3]，默认值为1        |
| -c/--config    | 可选    | 指定自定义绑核所需的json配置文件，缺省时按默认策略进行绑核，类型为str，默认值为None |

#### json配置文件参数说明

| 参数               | 可选/必选 | 说明                                                                                                                                          |
|------------------|-------|---------------------------------------------------------------------------------------------------------------------------------------------|
| custom_bind      | 必选    | json键值，类型为str，对应value需要为List[Dict]，将每一组绑核对象存放在List中                                                                                         |
| process_name     | 可选    | 需要绑定的进程或线程名称，类型为str，由is_thread参数决定是进程或线程，指定非NPU卡对应进程或线程时进程数或线程数应等于cpu_list参数长度 <br/>默认值为None                                                |
| pid              | 可选    | 需要绑定的PID号，可以是一个或多个PID，该参数长度应等于cpu_list参数长度，类型为List[int] <br/>默认值为[]                                                                         |
| cpu_list         | 必选    | 绑定CPU列表，可以是一个或多个CPU区间，类型为List[str] <br/>默认值为[]                                                                                              |
| mem_bind         | 可选    | 选择是否需要在绑定CPU后将内存迁移至对应NUMA节点，类型为bool，取值为：<br/>&bull; true：表示绑核后将内存迁移至对应NUMA节点<br/>&bull; false：表示绑核后不需要将内存迁移至对应NUMA节点<br/>默认值为false      |
| is_thread        | 可选    | 选择绑定的ID是进程ID还是线程ID，类型为bool，取值为：<br/>&bull; true：表示绑定线程ID<br/>&bull; false：表示绑定进程ID<br/>默认值为false                                            |
| is_irq           | 可选    | 选择绑定的是否为硬件中断，类型为bool，取值为：<br/>&bull; true：表示绑定的是硬件中断<br/>&bull; false：表示绑定的不是硬件中断<br/>默认值为false                                             |
| irq_id           | 可选    | 需要绑定的硬件中断的中断号，类型为List[int]                                                                                                                  |
| bind_sub_process | 可选    | 选择绑核时是否需要绑定进程ID或线程ID的所有子线程，类型为bool，取值为：<br/>&bull; true：表示绑核时需要同时绑定进程ID或线程ID的所有子线程<br/>&bull; false：表示绑核时不需要绑定进程ID或线程ID的所有子线程<br/>默认值为false |

#### json配置文件示例

```json
{
  "custom_bind": [
    {
      "process_name": "VLLM::Worker_TP",
      "cpu_list": ["4-10","16-22","52-58","64-70"],
      "bind_sub_process": true
    },
    {
      "process_name": "acl_thread",
      "cpu_list": ["11","23","59","71"],
      "mem_bind": true,
      "is_thread": true
    },
    {
      "process_name": "release_thread",
      "cpu_list": ["12,13","24,25","60,61","72,73"],
      "is_thread": true
    },
    {
      "process_name": "VLLM::EngineCore",
      "cpu_list": ["44"],
      "bind_sub_process": false,
      "mem_bind": true
    },
    {
      "pid": [110351, 110352, 110353, 110354],
      "cpu_list": ["32-35"]
    },
    {
      "irq_id": [3008, 3009, 3264, 3265, 2496, 2497, 2752, 2753],
      "cpu_list": ["15", "16", "27", "28", "64", "65", "76", "77"],
      "is_irq": true
    }
  ]
}
```

#### 使用示例

- 使用示例1

  ```python
  python3 entrance.py bind -c ./bind_design.json
  ```
  
  按照[json配置文件](#json配置文件示例)中的方案进行绑核，假设当前仅NPU4,5,6,7在运行，则：
    1. 以进程名"VLLM::Worker_TP"匹配NPU4,5,6,7中的该进程，并分别绑定到CPU "4-10","16-22","52-58","64-70"，绑定时会同时绑定该进程下的所有子线程
    2. 以线程名"acl_thread"匹配NPU4,5,6,7中的该线程，并分别绑定到CPU "11","23","59","71"
    3. 以线程名"release_thread"匹配NPU4,5,6,7中的该线程，并分别绑定到CPU "12,13","24,25","60,61","72,73"
    4. 以进程名"VLLM::EngineCore"匹配环境中的该进程，并绑定到CPU "44"
    5. 分别绑定pid "110351","110352","110353","110354"到CPU "32","33","34","35"
    6. 分别绑定中断 "3008","3009","3264","3265","2496","2497","2752","2753"到CPU "15", "16", "27", "28", "64", "65", "76", "77"
- 使用示例2

  ```python
  python3 entrance.py bind
  ```
  
    1. 结合NPU亲和性以及可访问CPU数量，为每张NPU卡分配对应的CPU区间（每张卡分配的CPU区间都会在一个NUMA节点内）
    2. 绑定每张NPU卡对应的sq_send_trigger_irq和cq_update_irq（算子下发硬件中断）到其CPU区间的前两个CPU核，需要有/proc/irq/\<irq_id\>/smp_affinity文件的写权限，否则会执行失败
    3. 绑定每张NPU卡对应的dev[i]_sq_task（NPU驱动进程）到其CPU区间的第三个CPU核，需要在宿主机才能查询到此进程，否则会执行失败
    4. 绑定每张NPU卡对应的主进程和所有子线程到其CPU区间的第四到第九个CPU核
    5. 绑定每张NPU卡对应的acl_thread（算子下发线程）到其CPU区间的第十个CPU核
    6. 绑定每张NPU卡对应的release_thread（资源释放线程）到其CPU区间的第十一个CPU核

#### 输出示例

以使用示例1为例，会输出以下信息展示绑定结果：

  ```text
  [2026-03-13 10:34:27,406] [INFO]:Start binding core round 1: {"process_name": "VLLM::Worker_TP", "cpu_list": ["4-10","16-22","52-58","64-70"], "bind_sub_process": true}
  [2026-03-13 10:34:28,490] [INFO]:Bind the target (pid=1603713) to CPU4,5,6,7,8,9,10
  [2026-03-13 10:34:28,498] [INFO]:Bind the target (pid=1603714) to CPU16,17,18,19,20,21,22
  [2026-03-13 10:34:28,506] [INFO]:Bind the target (pid=1603715) to CPU52,53,54,55,56,57,58
  [2026-03-13 10:34:28,514] [INFO]:Bind the target (pid=1603716) to CPU64,65,66,67,68,69,70
  [2026-03-13 10:34:28,522] [INFO]:===== Round 1 of core binding has ended =====
  [2026-03-13 10:34:28,533] [INFO]:Start binding core round 2: {"process_name": "acl_thread", "cpu_list": ["11","23","59","71"], "mem_bind": true, "is_thread": true}
  [2026-03-13 10:34:29,592] [INFO]:Bind the target (pid=1648512) to CPU11
  [2026-03-13 10:34:29,603] [INFO]:Bind the target (pid=1648576) to CPU23
  [2026-03-13 10:34:29,613] [INFO]:Bind the target (pid=1648615) to CPU59
  [2026-03-13 10:34:29,623] [INFO]:Bind the target (pid=1648667) to CPU71
  [2026-03-13 10:34:29,633] [INFO]:===== Round 2 of core binding has ended =====
  [2026-03-13 10:34:29,633] [INFO]:Start binding core round 3: {"process_name": "release_thread", "cpu_list": ["12","24","60","72"], "is_thread": true}
  [2026-03-13 10:34:30,644] [INFO]:Bind the target (pid=1648513) to CPU12
  [2026-03-13 10:34:30,729] [INFO]:Bind the target (pid=1648577) to CPU24
  [2026-03-13 10:34:30,736] [INFO]:Bind the target (pid=1648616) to CPU60
  [2026-03-13 10:34:30,743] [INFO]:Bind the target (pid=1648668) to CPU72
  [2026-03-13 10:34:30,750] [INFO]:===== Round 3 of core binding has ended =====
  [2026-03-13 10:34:30,757] [INFO]:Start binding core round 4: {"process_name": "VLLM::EngineCore", "cpu_list": ["44"], "bind_sub_process": false, "mem_bind": true}
  [2026-03-13 10:34:31,490] [INFO]:Bind the target (pid=1603113) to CPU44
  [2026-03-13 10:34:31,522] [INFO]:===== Round 4 of core binding has ended =====
  [2026-03-13 10:34:31,348] [INFO]:Start binding core round 5: {"pid": [110351, 110352, 110353, 110354], "cpu_list": ["32-35"]}
  [2026-03-13 10:34:32,356] [INFO]:Bind the target (pid=110351) to CPU32,33,34,35
  [2026-03-13 10:34:32,363] [INFO]:Bind the target (pid=110352) to CPU32,33,34,35
  [2026-03-13 10:34:32,370] [INFO]:Bind the target (pid=110353) to CPU32,33,34,35
  [2026-03-13 10:34:32,378] [INFO]:Bind the target (pid=110354) to CPU32,33,34,35
  [2026-03-13 10:34:32,385] [INFO]:===== Round 5 of core binding has ended =====
  [2026-03-13 10:34:32,406] [INFO]:Start binding core round 6: {"irq_id": [3008, 3009, 3264, 3265, 2496, 2497, 2752, 2753], "cpu_list": ["15", "16", "27", "28", "64", "65", "76", "77"], "is_irq": true}
  [2026-03-13 10:34:33,540] [INFO]:Bind the interrupt of IRQ-3008 to CPU15
  [2026-03-13 10:34:33,540] [INFO]:Bind the interrupt of IRQ-3009 to CPU16
  [2026-03-13 10:34:33,540] [INFO]:Bind the interrupt of IRQ-3264 to CPU27
  [2026-03-13 10:34:33,540] [INFO]:Bind the interrupt of IRQ-3265 to CPU28
  [2026-03-13 10:34:33,540] [INFO]:Bind the interrupt of IRQ-2496 to CPU64
  [2026-03-13 10:34:33,541] [INFO]:Bind the interrupt of IRQ-2497 to CPU65
  [2026-03-13 10:34:33,541] [INFO]:Bind the interrupt of IRQ-2752 to CPU76
  [2026-03-13 10:34:33,541] [INFO]:Bind the interrupt of IRQ-2753 to CPU77
  [2026-03-13 10:34:33,542] [INFO]:===== Round 6 of core binding has ended =====
  ```
