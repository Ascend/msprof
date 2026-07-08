# GIL Tracer

## Overview

In current AI training and inference scenarios, Python multithreading is widely used for tasks such as operator dispatch and memory movement. However, due to Python's Global Interpreter Lock (GIL), the interpreter can execute only one thread at a time in a multithreaded environment and cannot achieve true parallel execution. Threads must contend for the GIL, which leads to frequent lock contention, unbalanced scheduling, and reduced overall model performance.

To address this issue, the GIL Tracer tool is developed by integrating openEuler [sysTrace](https://gitcode.com/openeuler/sysTrace). It provides a simple and non-intrusive mechanism for GIL behavior monitoring, enabling the analysis of GIL usage in Python multithreaded programs. This helps developers identify performance bottlenecks, optimize thread scheduling strategies, and perform joint analysis with MindStudio Profiler to improve overall application performance.

**Procedure**

  1. Collect Python GIL data using either the command line or APIs.
  2. Process the collected GIL data and convert it into Chrome Trace JSON format.
  3. Import the converted data into [MindStudio Insight](https://gitcode.com/Ascend/msinsight/blob/26.0.0/docs/en/user_guide/overview.md) together with Profiler data for visualization and analysis. For Profiler data collection, see section "Performance Tuning Tools" in [msTT](https://gitcode.com/Ascend/mstt/blob/26.0.0/README_EN.md) or [msIT](https://gitcode.com/Ascend/msit/blob/26.0.0/README_EN.md).

## Preparations

  1. Install `sysTrace`. For details, see the [sysTrace Installation Guide](https://gitcode.com/openeuler/sysTrace/blob/master/docs/zh/installation.md).
  2. Add the `sysTrace` binary to the `PATH` environment variable.

      ```bash
      export PATH=$PATH:<sysTrace_path>/systrace/build
      ```

  3. Configure `sysTrace` environment variables, including:
      1. Runtime dependencies. For details, see the [sysTrace User Guide](https://gitcode.com/openeuler/sysTrace/blob/master/docs/zh/introduction.md).
      2. Logging and data directory configurations. For details, see the [sysTrace Data Collection Guide](https://gitcode.com/openeuler/sysTrace/blob/master/docs/zh/data_collection.md).
  4. Obtain the collection and conversion scripts `gil_trace_record.py` and `gil_trace_convert.py`, and the file utility script `file_manager.py` provided in the repository. Synchronous collection of Profiler and GIL data is recommended.

## GIL Data Collection

You can collect GIL data using either the command line or APIs.

### Method 1: Using the command line

**Description**

Collect GIL data by executing the `gil_trace_record.py` script.

**Syntax**

```bash
python gil_trace_record.py [--pid <PID>] [--duration <duration>]
```

**Command-line Options**

| Option| Mandatory (Yes/No)| Description|
| --- | --- | --- |
| --pid | No| Specifies the list of process PIDs for GIL data collection. Multiple PIDs are separated by commas. Default value: `None` (collects all Python processes running on the NPU).|
| --duration | No| Specifies the data collection duration, in seconds (s). Default value: `-1` (collects continuously until manually stopped using `Ctrl+C`).|

**Examples**

Start a Python model process on the NPU and obtain its PID, such as `12345`. Open a new terminal window to run the `gil_trace_record.py` script (requires root privileges).

+ Example 1: Collect GIL data for a specified process with PID `12345` for 10 seconds.

  ```bash
  python gil_trace_record.py --pid 12345 --duration 10
  ```

+ Example 2: Collect GIL data for all Python processes running on the NPU for 10 seconds.

  ```bash
  python gil_trace_record.py --duration 10
  ```

+ Example 3: Collect GIL data for a specified process with PID `12345` and stop data collection manually.

  ```bash
  python gil_trace_record.py --pid 12345
  # Press Ctrl+C to manually stop data collection.
  ```

**Output Description**

After data collection finishes, the tool generates the corresponding GIL data files in the data directory configured by `sysTrace` (default: `/home/sysTrace/GIL`). The file name format is `GIL_<pid>_rank_<rank_id>.json`, where `<pid>` represents the process ID and `<rank_id>` represents the Rank ID of the model process. The default value of the Rank ID is `0`.

### Method 2: Using APIs

**Description**

The `gil_trace_record.py` script provides APIs to independently control the start and stop of GIL data collection. Developers can embed the collection logic precisely into applications. This is suitable for scenarios requiring dynamic control over collection timing, conditional triggers, or deep integration with business workflows.

**Example**

1. API for starting data collection

   ```python
   gil_trace_record_start(pid_list=None, duration=-1)
   ```

2. API for stopping data collection

    ```python
    gil_trace_record_stop()
    ```

The code sample is as follows:

```python
import os
from gil_trace_record import gil_trace_record_start, gil_trace_record_stop

def train():
    # Collect the GIL data for the current process for 10 seconds.
    gil_trace_record_start(pid_list=[os.getpid()], duration=10)

    profiling_start()

    # Model running...

    profiling_stop()
    gil_trace_record_stop()
```

**Parameters**

| Parameter    | Mandatory (Yes/No)| Description                                                        |
| -------- | --------- | ------------------------------------------------------------ |
| pid_list | No     | Specifies the list of process PIDs for GIL data collection. Default value: `None` (collects all Python processes running on the NPU).|
| duration | No     | Specifies the data collection duration, in seconds (s). Default value: `-1` (collects continuously until you manually call `gil_trace_record_stop()`).|

**Output Description**

After data collection finishes, the tool generates the corresponding GIL data files in the data directory configured by `sysTrace` (default: `/home/sysTrace/GIL`). The file name format is `GIL_<pid>_rank_<rank_id>.json`, where `<pid>` represents the process ID and `<rank_id>` represents the Rank ID of the model process. The default value of the Rank ID is `0`.

## Data Postprocessing

**Description**

The `gil_trace_convert.py` script processes the collected GIL data and converts it into Chrome Trace JSON format. This allows you to import it into MindStudio Insight for joint visualization and analysis.

**Syntax**

```bash
python gil_trace_convert.py --input <input_file> --output <output_file>
```

**Command-line Options**

| Option| Mandatory (Yes/No)| Description|
| --- | --- | --- |
| --input | Yes| Specifies the path to the input GIL data file, including the file name.|
| --output | Yes| Specifies the path for the output Chrome Trace JSON file, including the file name.|

**Example**

```bash
python gil_trace_convert.py --input GIL_12345_rank_0.json --output GIL_12345_rank_0_trace.json
```

## Data Visualization

Import the converted Chrome Trace JSON file into MindStudio Insight to view the collected GIL data.

**Independent Collection**

![GIL Trace](./figures/gil_tracer.PNG "GIL Trace")

Each colored block represents a GIL event, which belongs to one of three types:

+ `take_gil`: indicates that the thread is waiting for the GIL lock.
+ `hold_gil`: indicates that the thread holds the GIL lock.
+ `drop_gil`: indicates that the thread releases the GIL lock.

**Joint Visualization with Profiler Data**

When you use APIs to collect GIL and Profiler data simultaneously, MindStudio Insight can display both datasets together. This facilitates the analysis and optimization of Python multithreading performance issues.

![GIL Trace with Profiler](./figures/profiler_gil_tracer.PNG "GIL Trace with Profiler")

## Security Statement

Due to underlying limitations of the `sysTrace` tool, you must ensure that the current user has root privileges when executing the `gil_trace_record.py` script or calling its APIs. Otherwise, data collection might fail.

The data postprocessing script `gil_trace_convert.py` has no special privilege requirements and can run with normal user privileges.
