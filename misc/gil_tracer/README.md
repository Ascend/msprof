# GIL Tracer

## 简介

当前 AI 训练、推理业务场景中，广泛使用 Python 多线程执行算子下发、内存搬运等任务。然而，由于 Python 的全局解释器锁（GIL）的存在，导致在多线程环境下，Python 解释器只能同时执行一个线程，无法实现真正的并行计算，各线程需竞争 GIL 锁，导致锁抢占频繁、线程调度不均，引发模型运行效率下降。

针对以上问题，通过集成 openEuler [sysTrace](https://gitcode.com/openeuler/sysTrace)，我们设计了 GIL Tracer 工具，提供简单易用、无侵入性的 GIL 锁检测能力，用于分析 Python 多线程程序中 GIL 锁的使用情况，帮助开发人员识别系统性能瓶颈，优化线程调度策略，并与 MindStudio Profiler 数据联合分析，助力用户优化业务逻辑，提升应用整体性能。

**使用流程**

  1. 通过命令行或 API 接口两种方式采集 Python GIL 数据。
  2. 将采集的 Python GIL 数据进行后处理，转换为Chrome Trace Json格式。
  3. 联合 Profiler 数据导入[MindStudio Insight](https://gitcode.com/Ascend/msinsight/blob/master/docs/zh/user_guide/overview.md)，进行可视化展示，Profiler 数据采集可参见 [msProf性能数据采集](https://gitcode.com/cann/oam-tools/blob/master/docs/zh/profiling/README.md)。

## 使用前准备

  1. 安装 sysTrace 工具，具体请参见 《[sysTrace 安装指南](https://gitcode.com/openeuler/sysTrace/blob/master/docs/zh/installation.md)》。
  2. 将 sysTrace 二进制工具添加到 PATH 环境变量中。

      ```bash
      export PATH=$PATH:<sysTrace_path>/systrace/build
      ```

  3. 配置 sysTrace 相关环境变量，包括：
      1. 运行时依赖，参见 《[sysTrace 使用指南](https://gitcode.com/openeuler/sysTrace/blob/master/docs/zh/introduction.md)》。
      2. 日志、数据目录配置，参见 《[sysTrace 采集使用指南](https://gitcode.com/openeuler/sysTrace/blob/master/docs/zh/data_collection.md)》。
  4. 获取仓库中提供的采集、转换脚本 [gil_trace_record.py](./gil_trace_record.py)，[gil_trace_convert.py](./gil_trace_convert.py)，以及文件操作相关脚本（[file_manager.py](./file_manager.py)），推荐 Profiler 和 GIL 数据同步采集。

## GIL 数据采集

支持命令行和 API 接口两种方式采集 GIL 数据。

### 方式一：命令行采集

**功能说明**

通过执行 gil_trace_record.py 脚本采集 GIL 数据。

**命令格式**

```bash
python gil_trace_record.py [--pid <PID>] [--duration <duration>]
```

**参数说明**

| 参数 | 可选/必选 | 说明 |
| --- | --- | --- |
| --pid | 可选 | 指定要采集 GIL 数据的进程 PID 列表，多个 PID 之间用逗号分隔。默认值为 None，即采集所有运行在 NPU 上的 Python 进程。 |
| --duration | 可选 | 指定采集数据的持续时间，单位为s。默认值为 -1，表示持续采集，需要手动 `Ctrl+C` 结束采集。 |

**使用示例**

在 NPU 上启动模型 Python 进程，获取其 PID，假设为 12345，新创建一个终端窗口，用于运行 gil_trace_record.py 脚本（需要 root 权限）。

- 示例1：采集指定 PID 为 12345 的进程的 GIL 数据，持续时间为10秒。

  ```bash
  python gil_trace_record.py --pid 12345 --duration 10
  ```

- 示例2：采集所有运行在 NPU 上的 Python 进程的 GIL 数据，持续时间为10秒。

  ```bash
  python gil_trace_record.py --duration 10
  ```

- 示例3：采集指定 PID 为 12345 的进程的 GIL 数据，手动结束采集。

  ```bash
  python gil_trace_record.py --pid 12345
  # 采集过程中，按下 Ctrl+C 键，即可手动结束采集
  ```

**输出说明**

采集结束后，会在 sysTrace 配置的数据目录下（默认 `/home/sysTrace/GIL`）生成对应的 GIL 数据文件，文件名格式为 `GIL_<pid>_rank_<rank_id>.json`，其中 `<pid>` 为进程 ID，`<rank_id>` 为模型进程所在的 Rank ID，默认值为 0。

### 方式二：API 接口采集

**功能说明**

gil_trace_record.py 中提供独立控制 GIL 数据采集启停的 API 接口，开发者可以将数据采集逻辑精细地嵌入到应用程序中，适合需要动态控制采集时机、条件触发采集或与业务逻辑深度集成的场景。

**使用示例**

1. 开始采集接口

   ```python
   gil_trace_record_start(pid_list=None, duration=-1)
   ```

2. 结束采集接口

    ```python
    gil_trace_record_stop()
    ```

示例代码如下：

```python
import os
from gil_trace_record import gil_trace_record_start, gil_trace_record_stop

def train():
    # 采集当前进程的 GIL 数据，持续时间为10秒
    gil_trace_record_start(pid_list=[os.getpid()], duration=10)

    profiling_start()

    # 模型运行...

    profiling_stop()
    gil_trace_record_stop()
```

**参数说明**

| 参数     | 可选/必选 | 说明                                                         |
| -------- | --------- | ------------------------------------------------------------ |
| pid_list | 可选      | 指定要采集 GIL 数据的进程 PID 列表。默认值为 None，即采集所有运行在 NPU 上的 Python 进程。 |
| duration | 可选      | 指定采集数据的持续时间，单位为s。默认值为 -1，表示持续采集，要手动调用 `gil_trace_record_stop()` 结束采集。 |

**输出说明**

采集结束后，会在 sysTrace 配置的数据目录下（默认 `/home/sysTrace/GIL`）生成对应的 GIL 数据文件，文件名格式为 `GIL_<pid>_rank_<rank_id>.json`，其中 `<pid>` 为进程 ID，`<rank_id>` 为模型进程所在的 Rank ID，默认值为 0。

## 数据后处理

**功能说明**

gil_trace_convert.py 脚本用于将采集的 GIL 数据进行加工处理，转换为 Chrome Trace Json 格式，以便导入 MindStudio Insight 可视化工具联合展示与分析。

**命令格式**

```bash
python gil_trace_convert.py --input <input_file> --output <output_file>
```

**参数说明**

| 参数 | 可选/必选 | 说明 |
| --- | --- | --- |
| --input | 必选 | 指定输入的 GIL 数据文件路径，需要指定到文件名。 |
| --output | 必选 | 指定输出的 Chrome Trace Json 文件路径，需要指定到文件名。 |

**使用示例**

```bash
python gil_trace_convert.py --input GIL_12345_rank_0.json --output GIL_12345_rank_0_trace.json
```

## 数据可视化

将转换后的 Chrome Trace Json 文件导入 MindStudio Insight 可视化工具，即可展示 GIL 数据的采集情况。

**单独采集**

![GIL Trace](./figures/gil_tracer.PNG "GIL Trace")

其中，每个色块表示一个 GIL 事件，分为三种类型：

- `take_gil`：表示线程等待 GIL 锁。
- `hold_gil`：表示线程持有 GIL 锁。
- `drop_gil`：表示线程释放 GIL 锁。

**与 Profiler 数据联合呈现**

使用 API 接口同时采集 GIL 和 Profiler 数据，可将两者的采集数据联合展示在 MindStudio Insight 中，方便分析和优化 Python 多线程性能问题。

![GIL Trace with Profiler](./figures/profiler_gil_tracer.PNG "GIL Trace with Profiler")

## 安全说明

由于 **sysTrace** 工具底层限制，执行 **gil_trace_record.py** 脚本或调用 **gil_trace_record.py** 中的 API 接口时，需要确保当前用户有足够的权限（root 权限），否则可能会导致采集数据失败。

数据后处理脚本 **gil_trace_convert.py** 无特殊权限要求，可在普通用户权限下运行。
