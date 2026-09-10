# Host Diagnostic and Tuning Tool

## Introduction

In current AI training and inference scenarios, task dispatching on the host side (CPU), such as operator scheduling and memory allocation, and task execution on the device side (NPU) are performed asynchronously. When task dispatching on the host side takes longer than task execution on the device side, the device may become idle while waiting for new tasks, resulting in a performance bottleneck known as a HostBound issue.

To address this issue, we developed a host-side diagnostic and tuning tool that provides easy-to-use CPU core binding capabilities. By binding processes and threads to different CPU cores, the tool reduces mutual interference and resource contention.

## Prerequisites

1. Obtain the parameter parsing and CPU core binding scripts provided in the repository: [entrance.py](./entrance.py) and [cpu_binder.py](./cpu_binder.py).

## Function Description

### Custom CPU Core Binding

#### Description

Provides the custom CPU core binding capability based on the configuration specified in the user-provided JSON file to bind processes or threads to specified CPU cores. If no configuration is provided, an empirically optimized strategy is used for CPU core binding:

  - For each NPU, one CPU core is allocated to each key thread, `acl_thread` and `release_thread`, and one CPU core is allocated to `dev[i]_sq_task` (which can be bound on the host; skipped if the process cannot be found). One CPU core is allocated to each operator-related interrupt, `sq_send_trigger_irq` and `cq_update_irq` (write permission to the `/proc` directory is required; skipped if the required permissions are unavailable). The remaining inference threads are jointly allocated to six CPU cores. Therefore, each NPU is bound to 11 CPU cores. NPU affinity and the latency of cross-NUMA memory access are considered during CPU allocation.

#### Syntax

```bash
python3 entrance.py bind [-l] [-c <config_path>]
```

#### Command-line Options

| Option | Required (Yes/No) | Description |
| --- | --- | --- |
| `-l/--log-level` | No | Specifies the log level to be printed. The type is `int`. Valid values: `0`, `1`, `2`, or `3`. Default value: `1`. |
| `-c/--config` | No | Specifies the JSON configuration file required for custom CPU core binding. If this parameter is not specified, CPU core binding is performed according to the default strategy. The type is `str`. Default value: `None`. |

#### JSON Configuration File Parameters

| Parameter | Required (Yes/No) | Description |
| --- | --- | --- |
| `custom_bind` | Yes | A JSON key of type `str`. The corresponding value must be a `List[Dict]`, with each group of CPU core binding objects stored as an item in the list. |
| `process_name` | No | Specifies the name of the process or thread to be bound. Whether the name refers to a process or thread is determined by `is_thread`. If the specified process or thread is not associated with an NPU, the number of processes or threads must be equal to the length of `cpu_list`. Default value: `None`. |
| `pid` | No | Specifies one or more PIDs to be bound. The number of PIDs must be equal to the length of `cpu_list`. The type is `List[int]`. Default value: `[]`. |
| `cpu_list` | Yes | Specifies the list of CPUs to which the target is bound. One or more CPU ranges can be specified. The type is `List[str]`. Default value: `[]`. |
| `mem_bind` | No | Specifies whether to migrate memory to the corresponding NUMA node after CPU core binding. The type is `bool`. The value can be one of the following:<br>• `true`: Migrates memory to the corresponding NUMA node after CPU core binding.<br>• `false`: Does not migrate memory to the corresponding NUMA node after CPU core binding.<br>Default value: `false`. |
| `is_thread` | No | Specifies whether the ID to be bound is a process ID or thread ID. The type is `bool`. The value can be one of the following:<br>• `true`: Binds a thread ID.<br>• `false`: Binds a process ID.<br>Default value: `false`. |
| `is_irq` | No | Specifies whether the target to be bound is a hardware interrupt. The type is `bool`. The value can be one of the following:<br>• `true`: The target is a hardware interrupt.<br>• `false`: The target is not a hardware interrupt.<br>Default value: `false`. |
| `irq_id` | No | Specifies the IDs of the hardware interrupts to be bound. The type is `List[int]`. |
| `bind_sub_process` | No | Specifies whether to also bind all child threads of the specified process or thread. The type is `bool`. The value can be one of the following:<br>• `true`: Also binds all child threads of the specified process or thread.<br>• `false`: Does not bind the child threads of the specified process or thread.<br>Default value: `false`. |

#### JSON Configuration File Example

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

#### Usage Examples

- Example 1

  ```python
  python3 entrance.py bind -c ./bind_design.json
  ```

  Perform CPU core binding according to the configuration in the [JSON Configuration File Example](#json-configuration-file-example). Assume that only NPU 4, 5, 6, and 7 are running:
    1. Match the process named `VLLM::Worker_TP` on NPU 4, 5, 6, and 7 and bind them to CPUs `4-10`, `16-22`, `52-58`, and `64-70`, respectively. All child threads of each process are also bound.
    2. Match the thread named `acl_thread` on NPU 4, 5, 6, and 7 and bind them to CPUs `11`, `23`, `59`, and `71`, respectively.
    3. Match the thread named `release_thread` on NPU 4, 5, 6, and 7 and bind them to CPUs `12,13`, `24,25`, `60,61`, and `72,73`, respectively.
    4. Match the process named `VLLM::EngineCore` in the environment and bind it to CPU `44`.
    5. Bind PIDs `110351`, `110352`, `110353`, and `110354` to CPUs `32`, `33`, `34`, and `35`, respectively.
    6. Bind interrupts `3008`, `3009`, `3264`, `3265`, `2496`, `2497`, `2752`, and `2753` to CPUs `15`, `16`, `27`, `28`, `64`, `65`, `76`, and `77`, respectively.
- Example 2

  ```python
  python3 entrance.py bind
  ```

    1. Allocate a corresponding range of CPUs to each NPU based on NPU affinity and the number of accessible CPUs. The CPU range allocated to each NPU is within a single NUMA node.
    2. Bind `sq_send_trigger_irq` and `cq_update_irq` (hardware interrupts for operator dispatch) corresponding to each NPU to the first two CPU cores in its CPU range. Write permission to `/proc/irq/<irq_id>/smp_affinity` is required; otherwise, the binding fails.
    3. Bind `dev[i]_sq_task` (the NPU driver process) corresponding to each NPU to the third CPU core in its CPU range. The process must be queryable on the host; otherwise, the binding fails.
    4. Bind the main process and all child threads corresponding to each NPU to the fourth through ninth CPU cores in its CPU range.
    5. Bind `acl_thread` (the operator dispatch thread) corresponding to each NPU to the tenth CPU core in its CPU range.
    6. Bind `release_thread` (the resource release thread) corresponding to each NPU to the eleventh CPU core in its CPU range.

#### Output Example

The following information is displayed for Example 1 to show the binding results:

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
