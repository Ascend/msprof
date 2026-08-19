# mstx样例

## 简介

本目录包含有mstx各种接口的使用用例，各个文件夹对应不同用例，供用户理解使用mstx接口。目录以及用例具体说明如下：

| 样例                                   | 说明                                                | 支持产品型号                                                  |
| -------------------------------------- | -------------------------------------------------- | ------------------------------------------------------------ |
| [mstx_with_domain](./mstx_with_domain) | 展示mstx接口在默认domain与自定义domain中打点的使用方式 | 昇腾A3系列产品<br/>昇腾A2系列产品/Atlas 800I A2 推理产品/A200I A2 Box 异构组件<br/>昇腾310B系列产品<br/>昇腾310P系列产品<br/>昇腾910系列产品 |

## 使用前准备

本用例依赖于ascend-toolkit，使用前请确认已安装。

## 操作指导

1. 请在使用前执行`source ${install_path}/set_env.sh`以保证用例正常执行，`${install_path}`为CANN的安装路径，以root安装为例，默认路径为/usr/local/Ascend/ascend-toolkit。

2. 切换目录至用例所在位置，例如/usr/local/Ascend/ascend-toolkit/latest/tools/mstx/samples。

3. 执行用例目录下的sample_run.sh脚本。如下三种场景：

   - 正常执行用例脚本sample_run.sh的情况下，使用msprof配置--msproftx=on采集所有的打点数据（包括默认domain和用户自定义domain范围内的数据），命令如下：

     ```bash
     msprof --msproftx=on bash sample_run.sh
     ```

   - 可以通过增加配置--mstx-domain-include开关来控制想要采集的domain的打点数据，如只想采集"default"域的打点数据，可以配置命令如下：

     ```bash
     msprof --msproftx=on --mstx-domain-include="default" bash sample_run.sh
     ```

   - 可以通过增加配置--mstx-domain-exclude开关来控制不想要采集的domain的打点数据，如想采集除"default"域之外的打点数据，可以配置命令如下：

     ```bash
     msprof --msproftx=on --mstx-domain-exclude="default" bash sample_run.sh
     ```

   --mstx-domain-include与--mstx-domain-exclude参数互斥，不可同时配置。如需指定多个domain，使用逗号隔开。
