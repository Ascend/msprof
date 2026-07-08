# Function Monitor

## Overview

In foundation model training or inference scenarios, host-side performance jitter is one of the typical issues affecting model execution efficiency. It may cause operator dispatch latency in AI frameworks such as PyTorch, thereby slowing down model execution. The root cause lies in complex host-side software stack interactions, such as the Python interpreter, AI framework, and CANN software stack, which are difficult to accurately locate.

To address this issue, the Function Monitor tool is developed by integrating the lightweight Linux performance profiling capability of openEuler [libkperf](https://gitcode.com/openeuler/libkperf). This tool monitors function execution duration on the host, collects CPU PMU metrics (such as `Cache Miss` and `Page Faults`) with low overhead during the execution of specified functions, and enables joint data analysis with Ftrace and MindStudio Profiler. This helps users efficiently identify and optimize host-side performance issues.

 **Procedure**

1. Use a Python decorator or `with` statement to collect function execution duration and CPU PMU metrics. The tool supports data filtering based on execution duration thresholds and persistently writes the collected results to log files.
2. Process the collected log files and convert them into Chrome Trace JSON format.
3. Import the data into [MindStudio Insight](https://gitcode.com/Ascend/msinsight/blob/26.0.0/docs/en/user_guide/overview.md) for visualization to analyze the relationship between function execution duration and CPU PMU metrics.

## Precautions

+ This tool is supported only in the **PyTorch** framework. Other AI frameworks are not supported.
+ When you set the `ENABLE_LIBKPERF` environment variable to `True` and collect CPU PMU metrics, ensure that the current user has root privileges.
+ You can use the `@function_monitor` decorator and `FunctionMonitorContext` context manager provided in `function_monitor.py` **together** in different functions or code blocks to collect data. However, you **must not stack them within the same function or code block**. If a function is already decorated with `@function_monitor` and you use `FunctionMonitorContext` inside it, **data collection may become abnormal**.

## Preparations

1. Build and install the `libkperf` Python wheel package. For details, see the [libkperf Build Guide](https://gitcode.com/openeuler/libkperf#%E7%BC%96%E8%AF%91).
2. Obtain the collection and conversion scripts [function_monitor.py](./function_monitor.py) and [log2trace.py](./log2trace.py), and the file operation script [file_manager.py](./file_manager.py) provided in the repository.

## Data Collection

You can collect data using either a Python function decorator or `with` statement. The following table describes the configuration of environment variables applicable to both methods.

| Environment Variable| Mandatory (Yes/No)| Description|
| --- | --- | --- |
| ENABLE_FUNCTION_MONITOR | No| Specifies whether to enable function monitoring and data collection. Valid values: `True` (enables Function Monitor) or `False` (disables it). Default value: `False`.|
| ENABLE_LIBKPERF | No| Specifies whether to use `libkperf` to collect CPU PMU metrics. Valid values: `True` (enables `libkperf`) or `False` (disables it). Default value: `False`.|
| FUNCTION_MONITOR_LOG_PATH | No| Specifies the storage path for Function Monitor logs. If you do not specify a path, logs are stored in the `function_monitor_log` directory under the home directory of the current user, such as `/home/user/function_monitor_log`.|

When you set `ENABLE_LIBKPERF` to `True`, you can specify the list of perf events to collect by using the `evt_list` parameter of the `PerformanceMonitor` class in `function_monitor.py`. The default value is `['cycles', 'instructions', 'LLC-load-misses', 'LLC-loads', 'page-faults']`.

You can run the `perf list` command to view all supported perf events. For details, see the [perf event official documentation](https://man7.org/linux/man-pages/man1/perf-list.1.html). You can select perf events as needed.

![Perf Event](./figures/perf_event.png "Perf Event")

### Method 1: Using a function decorator

The `@function_monitor` decorator provided in `function_monitor.py` embeds data collection logic into specified application functions to collect function execution duration and CPU PMU metrics.

**Parameters**

| Parameter| Mandatory (Yes/No)| Description|
| --- | --- | --- |
| func_name | No| Specifies the name of the function to be monitored. If omitted, the tool uses the name of the decorated function by default.|
| process_name | No| Specifies the name of the process to be monitored. If omitted, the tool uses the current process name by default.|
| threshold_ms | No| Specifies the execution duration threshold for data collection in milliseconds (ms). The tool records functions in the log file only when their execution duration exceeds this threshold. Default value: `1`.|

**Example**

1. Set environment variables.

    ```bash
    export ENABLE_FUNCTION_MONITOR=True
    export ENABLE_LIBKPERF=True
    export FUNCTION_MONITOR_LOG_PATH=${HOME}/function_monitor_log
    ```

2. Modify the `evt_list` parameter of the `PerformanceMonitor` class in `function_monitor.py` to specify the perf events to collect. The default value is **`['cycles', 'instructions', 'LLC-load-misses', 'LLC-loads', 'page-faults']`**.

    ```python
    class PerformanceMonitor:

        def __init__(self, evt_list=None, pid_list=None, cpu_list=None):
            # ...
            try:
                self._kperf = importlib.import_module('kperf')
                # Set perf event list
                self.evt_list = evt_list or [
                    'cycles', 'instructions', 'LLC-load-misses', 'LLC-loads', 'page-faults'
                ]
            except Exception as e:
                self.logger.error(f"Failed to import kperf module: {e}")
                self.monitor_enabled = False
    ```

3. Apply the `@function_monitor` decorator in the PyTorch model script to functions that require data collection.

    ```python
    import os
    import torch
    from function_monitor import function_monitor
    
    @function_monitor(func_name='model_run', threshold_ms=1)
    def model_run():
        size = 1024
        A = torch.rand(size, size, dtype=torch.float32, requires_grad=False).npu()
        B = torch.rand(size, size, dtype=torch.float32, requires_grad=False).npu()
    
        for i in range(10):
            C = torch.matmul(A, B)
            D = torch.nn.functional.relu(C)
            E = torch.nn.functional.layer_norm(D, D.size()[1:])
    
    if __name__ == '__main__':
        for i in range(10):
            model_run()
    ```

**Output Description**

After data collection finishes, the tool generates a log file in the path specified by the `FUNCTION_MONITOR_LOG_PATH` environment variable. By default, this is the `function_monitor_log` directory under the home directory of the current user. The file name format is `function_monitor_<pid>.log`, where `<pid>` represents the process ID.

### Method 2: Using a `with` statement

The `function_monitor.py` script also provides the `FunctionMonitorContext` context manager. You can encapsulate data collection logic within a `with` statement block to collect execution duration and CPU PMU metrics for a specified code block.

**Parameters**

| Parameter| Mandatory (Yes/No)| Description|
| --- | --- | --- |
| func_name | Yes| Specifies the name of the function to be monitored.|
| process_name | No| Specifies the name of the process to be monitored. If omitted, the tool uses the current process name by default.|
| threshold_ms | No| Specifies the execution duration threshold for data collection in milliseconds (ms). The tool records functions in the log file only when their execution duration exceeds this threshold. Default value: `1`.|

**Example**

1. Set environment variables.

    ```bash
    export ENABLE_FUNCTION_MONITOR=True
    export ENABLE_LIBKPERF=True
    export FUNCTION_MONITOR_LOG_PATH=${HOME}/function_monitor_log
    ```

2. Modify the `evt_list` parameter of the `PerformanceMonitor` class in `function_monitor.py` to specify the perf events to collect. The default value is **`['cycles', 'instructions', 'LLC-load-misses', 'LLC-loads', 'page-faults']`**.

    ```python
    class PerformanceMonitor:

        def __init__(self, evt_list=None, pid_list=None, cpu_list=None):
            # ...
            try:
                self._kperf = importlib.import_module('kperf')
                # Set perf event list
                self.evt_list = evt_list or [
                    'cycles', 'instructions', 'LLC-load-misses', 'LLC-loads', 'page-faults'
                ]
            except Exception as e:
                self.logger.error(f"Failed to import kperf module: {e}")
                self.monitor_enabled = False
    ```

3. Import the `FunctionMonitorContext` context manager in the PyTorch model script, and encapsulate the target code block within a `with` statement block.

    ```python
    import os
    import torch
    from function_monitor import FunctionMonitorContext
    
    def model_run():
        size = 1024
        A = torch.rand(size, size, dtype=torch.float32, requires_grad=False).npu()
        B = torch.rand(size, size, dtype=torch.float32, requires_grad=False).npu()
    
        with FunctionMonitorContext(func_name='torch_operator_run', threshold_ms=1):
            for i in range(10):
                C = torch.matmul(A, B)
                D = torch.nn.functional.relu(C)
                E = torch.nn.functional.layer_norm(D, D.size()[1:])
    
    if __name__ == '__main__':
        for i in range(10):
            model_run()
    ```

**Output Description**

After data collection finishes, the tool generates a log file in the path specified by the `FUNCTION_MONITOR_LOG_PATH` environment variable. By default, this is the `function_monitor_log` directory under the home directory of the current user. The file name format is `function_monitor_<pid>.log`, where `<pid>` represents the process ID.

## Data Postprocessing

After data collection finishes, you can use the `log2trace.py` script to convert the collected log files into Chrome Trace JSON format. This allows you to import them into MindStudio Insight for joint visualization and analysis.

**Usage**

```bash
python log2trace.py --input <input_file> --output <output_file>
```

**Command-line Options**

| Option| Mandatory (Yes/No)| Description|
| --- | --- | --- |
| --input | Yes| Specifies the path to the input `function_monitor` log file, including the file name.|
| --output | No| Specifies the path for the output Chrome Trace JSON file, including the file name. If omitted, the tool generates a file in the current directory using the input file name with a `_trace.json` suffix.|

**Example**

```bash
python log2trace.py --input function_monitor_12345.log --output function_monitor_12345_trace.json
```

## Data Visualization

Import the converted Chrome Trace JSON file into the MindStudio Insight visualization tool to view the function execution duration and collected CPU PMU metrics.

![Function Monitor](./figures/function_monitor.png "Function Monitor")

Each colored block represents a function execution event, containing the function name and its execution duration. The `Args` section contains CPU PMU metrics collected during the execution of that function, such as page faults and LLC cache misses.

## Security Statement

Due to underlying limitations of the `libkperf` library, you must ensure that the current user has root privileges when calling decorators or interfaces defined in `function_monitor.py`. Otherwise, data collection might fail.

The data postprocessing script `log2trace.py` has no special privilege requirements and can run with normal user privileges.
