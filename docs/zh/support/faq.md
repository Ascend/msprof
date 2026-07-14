# FAQ

## 一、故障排查

- **Q：为什么PROF文件夹拖入insight没有任何呈现？**
- A：确认PROF目录下是否存在mindstudio_profiler_output目录，不存在相关目录使用msprof --export=on --output={PROF路径}进行数据解析。

- **Q：PLOG日志在哪里？如何配置？**
- A：PLOG（Process LOG）记录数据采集和解析过程中的详细信息，是排查采集失败、数据为空等问题的首选依据。
- 默认PLOG日志路径：`~/ascend/log`。
- 日志等级说明：

  | 等级 | 说明 |
  |------|------|
  | 0 - DEBUG | 调试信息，输出最详细的日志，用于深入定位问题 |
  | 1 - INFO | 一般信息，记录工具运行的关键步骤 |
  | 2 - WARNING | 警告信息，提示可能存在异常但不影响主流程 |
  | 3 - ERROR | 错误信息，记录采集或解析过程中的异常 |
  | 4 - NULL | 关闭日志输出 |

- 可通过环境变量调整日志等级和输出路径：`export ASCEND_GLOBAL_LOG_LEVEL=0`（设置日志等级），`export ASCEND_PROCESS_LOG_PATH=/path/to/plog`（自定义输出路径）。
- 当遇到数据采集失败、host目录为空等问题时，建议优先查看PLOG中的ERROR/WARNING日志以快速定位原因。
- 参考文档：[昇腾环境变量参考](https://www.hiascend.com/document/detail/zh/canncommercial/900/maintenref/envvar/envref_07_0122.html)。

- **Q：使用msprof采集算子数据，host目录为空没有数据？**
- A：优先检查Plog报错，可能存在的情况如下：
  - 磁盘空间不足
  - 目录没有相关操作权限

- **Q：msprof动态采集，配置完成后实际当前路径没有采集到相关数据？**
- A：优先查看prof_dir的路径，如果是相对路径则是相对于脚本的路径，而不是相对于profiler_config.json的路径；

- **Q：安装 `mindstudio-profiler_26.0.0_aarch64.run --install` 时提示 `Not enough space left in /root` 怎么处理？**
- A：这是安装包在解压阶段默认使用了 `/root` 作为临时目录，但 `/root` 剩余空间不足导致安装失败，并不是 `.run` 包本身损坏。
  - 可先清理 `/root` 下无用文件，确保有足够剩余空间后重新执行安装。
  - 如果不方便清理 `/root`，可以通过设置 `TMPDIR` 将解压临时目录切换到其他空间更大的路径，例如：

  ```bash
  mkdir -p /path/to/tmpdir
  export TMPDIR=/path/to/tmpdir
  ./mindstudio-profiler_26.0.0_aarch64.run --install
  ```
