# msProf工具安装指南

## 安装说明

msProf工具的安装方式包括：

- 使用CANN包安装：msProf工具完整功能已集成在CANN包中，请参考《[CANN 快速安装](https://www.hiascend.com/cann/download)》安装昇腾NPU驱动和CANN软件（包含Toolkit和ops包）并配置环境变量。
- 使用run包安装：msProf工具完整功能集成在CANN包中且msProf依赖CANN包，因此使用msProf工具需要**先完成CANN包的安装**，若需要升级安装本工具代码仓中的最新功能，可以[下载run包安装](#下载run包安装)或[源码编译安装](#源码编译安装)，在已安装CANN包的环境下覆盖安装msProf包。

## 下载run包安装

> [!NOTE] 
>
> 下载的msProf run包需要在已安装CANN的环境中进行覆盖安装才能使用。

1. 请参考[msProf Release](https://gitcode.com/Ascend/msprof/releases)下载msProf的run包和对应数字签名文件（.sha256）。

   下载本软件即表示您同意《[华为企业业务最终用户许可协议（EULA）](https://e.huawei.com/cn/about/eula)》的条款和条件。

2. 验证run包的完整性。

   1. 在run包所在目录执行如下命令获取run包的sha256校验码。

      ```bash
      sha256sum {name}.run
      ```

      打印如下示例信息。

      ```ColdFusion
      {sha256} {name}.run
      ```

   2. 用记事本打开数字签名文件查看sha256校验码。

   3. 比对两个文件的sha256校验码是否一致。

      若两个校验码一致，则表示下载了正确的软件包；若不一致，请不要使用该软件包，需要支持与服务请在论坛求助或提交技术工单。

3. 为run包添加可执行权限。

   ```bash
   chmod +x mindstudio-profiler_{version}_{arch}.run
   ```

4. 安装run包。

   ```shell
   ./mindstudio-profiler_{version}_{arch}.run --install
   ```

   如需指定安装路径，可附加 `--install-path={cann_path}` 参数。安装路径须指向`cann`目录，具体请参见[安装run包参数说明](#安装run包参数说明)。

   安装完成后，若显示如下信息，则说明软件安装成功。

   ```ColdFusion
   mindstudio-profiler package install success.
   ```

## 源码编译安装

如需使用最新代码的功能，可下载本仓库代码，自行编译、打包并完成安装。

> [!NOTE] 
> 
> 编译出的msProf run包需要在已安装CANN的环境中进行覆盖安装才能使用。

### 编译环境准备

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

### 执行编译打包

`build/build.sh`编译脚本支持通过--mode参数指定编译类型：

- all：编译全量run包（包含采集与解析功能）
- analysis：编译解析run包（仅包含解析功能）

更多参数说明请参见[编译run包参数说明](#编译run包参数说明)。

编译完成后，会在当前路径`output`目录下生成run包，名称格式为`mindstudio-profiler_{version}_{arch}.run`。其中，`version`为版本号，`arch`为系统架构（根据实际运行系统自动适配）。

#### 方式一：编译msProf全量run包（推荐）

```shell
# 编译全量run包，包含msProf的采集和解析功能
bash build/build.sh --mode=all --version={version}
```

#### 方式二：编译msProf解析run包

```shell
# 单独编译解析包
bash build/build.sh --mode=analysis --version={version}
```

### 安装run包

1. run包将生成在`output`目录下，执行以下命令为其添加可执行权限：

   ```shell
   cd output
   chmod +x mindstudio-profiler_{version}_{arch}.run
   ```

2. 执行安装命令。

   ```shell
   ./mindstudio-profiler_{version}_{arch}.run --install
   ```

   安装命令支持`--install-path`等参数，具体请参见[安装run包参数说明](https://gitcode.com/Ascend/msprof/blob/37781908bfb29a6686a6a37130d141bff08203bf/docs/zh/msprof_install_guide.md#安装run包参数说明)。

   执行安装命令时，会自动执行`--check`参数，校验软件包的一致性和完整性，出现如下回显信息，表示软件包校验成功。

   ```ColdFusion
   Verifying archive integrity...  100%   SHA256 checksums are OK. All good.
   ```

   安装完成后，若显示如下信息，则说明软件安装成功。

   ```ColdFusion
   mindstudio-profiler package install success.
   ```

## 升级

msProf工具升级可参照[下载run包安装](#下载run包安装)或[源码编译安装](#源码编译安装)中的步骤直接安装msProf最新的run包即可，新的run包会自动覆盖原有的run包。

## 卸载

卸载msProf工具有如下两种方式：

方式一：通过--uninstall参数单独卸载

```bash
./mindstudio-profiler_{version}_{arch}.run --uninstall --install-path={msprof_install_path}
```

> [!NOTE]
>
> 需要在run包所在路径执行该命令，其中`msprof_install_path`为该run包安装路径。

方式二：卸载CANN包

msProf默认会安装在CANN包的安装路径下，直接卸载CANN包的时候会将msProf一起卸载。

## 附录

### 编译run包参数说明

msProf工具run包的编译命令可配置如下参数。

| 参数         | 可选/必选 | 说明                                                         |
| ------------ | --------- | ------------------------------------------------------------ |
| --build_type | 可选      | 编译run包类型，可取值：<br>&#8226; Release：编译出用于生产环境部署的软件包。<br>&#8226; Debug：编译出用于开发调试的软件包（只支持编译**解析**部分的Debug软件包）。<br>默认值为Release。 |
| --mode       | 可选      | 编译run包方式。可取值：<br>&#8226; all：编译出包含msProf采集和解析功能的软件包。<br>&#8226; analysis：编译出仅包含msProf解析功能的软件包。<br>默认值为analysis。 |
| --version    | 可选      | 配置run包的版本号。<br>默认值为none。                        |

### 安装run包参数说明

msProf工具run包的安装命令可配置如下参数。

| 参数              | 可选/必选 | 说明                                                         |
| ----------------- | --------- | ------------------------------------------------------------ |
| --install         | 可选      | 安装软件包。可配置--install-path参数指定软件的安装路径；不配置--install-path参数时，则直接安装到默认路径下。 |
| --uninstall       | 可选      | 卸载软件包。可配置--install-path参数指定软件安装时的路径；不配置--install-path参数时，则直接卸载默认路径下的msProf。 |
| --install-path    | 可选      | 安装路径。路径须指定到cann目录，如果用户未指定安装路径，则软件会安装到默认路径下，默认安装路径如下：<br>&#8226; root用户：`/usr/local/Ascend/cann`<br>&#8226; 非root用户：`${HOME}/Ascend/cann`，${HOME}为当前用户的家目录。 |
| --install-for-all | 可选      | 安装时，允许其他用户具有安装用户组的权限。当安装携带该参数时，支持其他用户使用msProf运行业务，但该参数存在安全风险，请谨慎使用。 |
| --help            | 可选      | 查看帮助信息。                                               |
