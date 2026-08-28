# FAQ

## 1. Troubleshooting

- **Q: Why is nothing displayed when I drag the `PROF` directory into MindStudio Insight?**
- A: Check whether the `mindstudio_profiler_output` directory exists in the PROF directory. If the directory does not exist, run `msprof --export=on --output={PROF path}` to parse the data.

- **Q: Where are the PLOG logs stored? How can I configure them?**
- A: Process LOG (PLOG) records detailed information about data collection and parsing. It is the preferred source for troubleshooting issues such as data collection failures and no data being generated.
- The default PLOG log path is `~/ascend/log`.
- Log levels are described in the following table.

  | Level | Description |
  | --- | --- |
  | 0 - DEBUG | Debug information. Outputs the most detailed logs for in-depth troubleshooting. |
  | 1 - INFO | General information. Records key steps during tool execution. |
  | 2 - WARNING | Warning information. Indicates potential issues that do not affect the main workflow. |
  | 3 - ERROR | Error information. Records exceptions that occur during data collection or parsing. |
  | 4 - NULL | Disables log output. |

- You can use environment variables to adjust the log level and output path: `export ASCEND_GLOBAL_LOG_LEVEL=0` sets the log level, and `export ASCEND_PROCESS_LOG_PATH=/path/to/plog` specifies a custom output path.
- When data collection fails or the host directory is empty, check the ERROR/WARNING logs in PLOG first to quickly identify the cause.
- Reference: [Ascend Environment Variables](https://www.hiascend.com/document/detail/en/CANNCommunityEdition/910/maintenref/envvar/envref_07_0001.html)

- **Q: Why is the host directory empty when I use msProf to collect operator data?**
- A: Check the PLOG error messages first. Possible causes include:
  - Insufficient drive space
  - Insufficient permissions for the directory

- **Q: Why is no data collected in the current path after I configure msProf for dynamic data collection?**
- A: Check the `prof_dir` path first. If it is a relative path, it is relative to the path of the script, not the path of `profiler_config.json`.

- **Q: What should I do if `Not enough space left in /root` is displayed when I run `mindstudio-profiler_26.1.0_aarch64.run --install`?**
- A: During the extraction phase, the installation package uses `/root` as the temporary directory by default. If there is insufficient space in `/root`, the installation fails. This does not indicate that the `.run` package itself is corrupted.
  - Clean up unnecessary files under `/root` to ensure sufficient free space, and then run the installation again.
  - If you cannot free up space under `/root`, set `TMPDIR` to switch the temporary extraction directory to another location with more available space. For example:

  ```bash
  mkdir -p /path/to/tmpdir
  export TMPDIR=/path/to/tmpdir
  ./mindstudio-profiler_26.1.0_aarch64.run --install
  ```
