# msProf工具安装指南

## 1. 安装说明

本工具已集成于CANN中，若已安装CANN且无需更新此工具，可直接使用，无需按本文档安装。

若您的环境尚未安装CANN，请参见《[CANN 快速安装](https://www.hiascend.com/cann/download)》安装昇腾NPU驱动和CANN软件（包含Toolkit和ops包），并配置环境变量。

如需单独升级本工具或使用最新版本，您可通过以下三种方式进行安装：[在线安装](#21-在线安装)、[离线安装](#22-离线安装)、[源码安装](#23-源码安装)。

## 2. 安装方式

### 2.1 在线安装

若您的设备具备互联网访问能力，可通过一条命令自动完成工具的下载与安装。请参见昇腾社区MindStudio[下载](https://www.hiascend.com/developer/software/mindstudio/download)页面，选择对应的CANN版本，并在安装方式中选择“在线安装”，系统将引导您完成后续操作。

### 2.2 离线安装

对处于企业内网等无外网环境的设备，请先在可联网的机器上下载完整的离线安装包，再将其传输至目标设备进行安装。请参见昇腾社区MindStudio[下载](https://www.hiascend.com/developer/software/mindstudio/download)页面，选择对应的CANN版本，并在安装方式中选择“离线安装”，获取对应的安装包及操作指引。

### 2.3 源码安装

如需使用最新代码的功能，可下载本仓库代码，自行编译、打包并完成安装。

> [!NOTE]
>
> 编译出的msProf run包需要在已安装CANN的环境中进行覆盖安装才能使用。

#### 2.3.1 编译环境准备

1. 安装依赖。

   msProf工具源码编译依赖SQLite3，请执行以下命令完成安装，或确保当前环境已满足该依赖。

   - Ubuntu系统上安装SQLite3：

     ```shell
      sudo apt update
      sudo apt install sqlite3 libsqlite3-dev
     ```

   - openEuler/CentOS系统上安装SQLite3：

     ```shell
     sudo yum install sqlite sqlite-devel
     ```

2. 克隆本仓库。

   ```shell
   git clone https://gitcode.com/Ascend/msprof.git
   ```

3. 下载第三方依赖。

   ```shell
   cd msprof
   # 下载三方依赖包
   bash scripts/download_thirdparty.sh
   ```

#### 2.3.2 执行编译打包

`build/build.sh`编译脚本支持通过--mode参数指定编译类型：

- all：编译全量run包（包含采集与解析功能）
- analysis：编译解析run包（仅包含解析功能）

更多参数说明请参见[编译run包参数说明](#61-编译run包参数说明)。

编译完成后，会在当前路径`output`目录下生成run包，名称格式为`mindstudio-profiler_{version}_{arch}.run`。其中，`version`为版本号，`arch`为系统架构（根据实际运行系统自动适配）。

##### 2.3.2.1 方式一：编译msProf全量run包（推荐）

```shell
# 编译全量run包，包含msProf的采集和解析功能
bash build/build.sh --mode=all --version=26.1.0
```

##### 2.3.2.2 方式二：编译msProf解析run包

```shell
# 单独编译解析包
bash build/build.sh --mode=analysis --version=26.1.0
```

#### 2.3.3 安装run包

1. run包将生成在`output`目录下，执行以下命令为其添加可执行权限：

   ```shell
   cd output
   chmod +x mindstudio-profiler_26.1.0_{arch}.run
   ```

2. 执行安装命令。

   ```shell
   ./mindstudio-profiler_26.1.0_{arch}.run --install
   ```

   安装命令支持`--install-path`等参数，具体请参见[安装run包参数说明](#62-安装run包参数说明)。

   执行安装命令时，会自动执行`--check`参数，校验软件包的一致性和完整性，出现如下回显信息，表示软件包校验成功。

   ```ColdFusion
   Verifying archive integrity...  100%   SHA256 checksums are OK. All good.
   ```

   安装完成后，若显示如下信息，则说明软件安装成功。

   ```ColdFusion
   mindstudio-profiler package install success.
   ```

## 3. 验证安装

安装完成后，执行以下命令验证工具是否安装成功：

```bash
msprof --help
```

若输出不报错，且能显示帮助信息，则表明安装成功。

若 `msprof --help` 提示命令不存在，请确认当前终端使用的是安装 `msprof` 的 Python 环境。

## 4. 卸载

可通过如下步骤卸载：

1. 下载脚本。

   ```bash
   curl -O https://inst.obs.cn-north-4.myhuaweicloud.com/26.1.0/ms_install.py
   ```

   > [!NOTE]
   >
   > - 需要联网环境才能下载，若环境不允许联网或离线，请先在可联网的环境下载该脚本后拷贝到目标设备。
   > - 若执行命令无响应或出现连接失败、SSL证书错误等问题，请参见[FAQ](https://www.hiascend.com/developer/blog/details/02176213671719317003)。

2. 执行卸载。

   ```bash
   python ms_install.py uninstall {tools_name}
   ```

   其中{tools_name}配置为需卸载的工具名称，可通过`python ms_install.py help`命令查询，在打印信息中的Available Tools字段下显示工具名称。

   卸载成功打印如下信息：

   ```ColdFusion
   Successfully uninstalled 1 tool ({tools_name})
   ```

## 5. 升级

升级即“先卸后装”。直接执行安装命令，工具将自动卸载旧版本，并引导您完成覆盖安装。

可通过`msprof --version`命令查看当前环境的版本信息，再选择需要升级的版本。升级版本时需要关注版本配套关系，请参见《[版本说明](https://gitcode.com/Ascend/release-management/blob/master/MindStudio/26.1.0/release_notes.md)》。

## 6. 附录

### 6.1 编译run包参数说明

msProf工具run包的编译命令可配置如下参数。

| 参数         | 可选/必选 | 说明                                                                                                                       |
| ------------ | --------- |--------------------------------------------------------------------------------------------------------------------------|
| --build_type | 可选      | 编译run包类型，可取值：<br>&#8226; Release：编译出用于生产环境部署的软件包。<br>&#8226; Debug：编译出用于开发调试的软件包（只支持编译**解析**部分的Debug软件包）。<br>默认值为Release。 |
| --mode       | 可选      | 编译run包方式。可取值：<br>&#8226; all：编译出包含msProf采集和解析功能的软件包。<br>&#8226; analysis：编译出仅包含msProf解析功能的软件包。<br>默认值为analysis。          |
| --version    | 可选      | 配置run包的版本号，用户自定义。<br>默认值为none。                                                                                           |

### 6.2 安装run包参数说明

msProf工具run包的安装命令可配置如下参数。

| 参数              | 可选/必选 | 说明                                                         |
| ----------------- | --------- | ------------------------------------------------------------ |
| --install         | 可选      | 安装软件包。可配置--install-path参数指定软件的安装路径；不配置--install-path参数时，则直接安装到默认路径下。 |
| --uninstall       | 可选      | 卸载软件包。可配置--install-path参数指定软件安装时的路径；不配置--install-path参数时，则直接卸载默认路径下的msProf。 |
| --install-path    | 可选      | 安装路径。路径须指定到cann目录，如果用户未指定安装路径，则软件会安装到默认路径下，默认安装路径如下：<br>&#8226; root用户：`/usr/local/Ascend/cann`<br>&#8226; 非root用户：`${HOME}/Ascend/cann`，${HOME}为当前用户的家目录。 |
| --install-for-all | 可选      | 安装时，允许其他用户具有安装用户组的权限。当安装携带该参数时，支持其他用户使用msProf运行业务，但该参数存在安全风险，请谨慎使用。 |
| --help            | 可选      | 查看帮助信息。                                               |
