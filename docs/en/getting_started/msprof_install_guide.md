# MindStudio Profiler Installation Guide

## 1. Installation Description

This tool has been integrated into CANN. If CANN has been installed and this tool does not need to be updated, you can directly use it without following the instructions in this document.

If CANN has not been installed in your environment, install the Ascend NPU driver and CANN software (including the Toolkit and ops) by referring to [CANN Quick Installation](https://www.hiascend.com/cann/download), and configure environment variables.

If you need to upgrade this tool separately or use the latest version, you can install it in any of the following ways: [Online Installation](#21-online-installation), [Offline Installation](#22-offline-installation), and [Installation from Source](#23-installation-from-source).

## 2. Installation Methods

### 2.1 Online Installation

If your device has Internet access, you can run a single command to automatically download and install the tool. Visit the [Ascend community](https://www.hiascend.com/developer/software/mindstudio/download), select the target CANN version, and choose "Online Installation". The system will guide you through the subsequent operations.

### 2.2 Offline Installation

For devices that are not connected to the Internet, such as those on an enterprise intranet, download the complete offline installation package on a device that has Internet access and then transfer the package to the target device for installation. Visit the [Ascend community](https://www.hiascend.com/developer/software/mindstudio/download), select the target CANN version, and choose "offline installation". The system will guide you through the subsequent operations.

### 2.3 Installation from Source

#### 2.3.1 Build Environment Setup

1. Install dependencies.

   msProf requires SQLite3 to build from source. Run the following commands to install SQLite3 or ensure it is already installed in your environment.

   * Install SQLite3 on Ubuntu.

     ```shell
      sudo apt update
      sudo apt install sqlite3 libsqlite3-dev
     ```

   * Install SQLite3 on openEuler or CentOS.

     ```shell
     sudo yum install sqlite sqlite-devel
     ```

2. Clone this repository.

   ```shell
   git clone https://gitcode.com/Ascend/msprof.git
   ```

3. Download third-party dependencies.

   ```shell
   cd msprof
   # Download third-party dependencies
   bash scripts/download_thirdparty.sh
   ```

#### 2.3.2 Building and Packaging

The `build/build.sh` build script allows you to specify the build type by using the `--mode` option:

* `all`: builds the full-featured runfile (including collection and parsing features).
* `analysis`: builds the parsing runfile (including only the parsing feature).

For details about more parameters, see [Command-line Options for Building a Runfile](#51-command-line-options-for-building-a-runfile).

After the build is complete, the runfile is generated in the `output` directory within the current path. The file name format is `mindstudio-profiler_{version}_{arch}.run`. `version` represents the version number and `arch` represents the system architecture (automatically adapted based on the current system).

##### 2.3.2.1 Method 1: Building the full-featured runfile of msProf (recommended)

```shell
# Build the full-featured runfile, including msProf collection and parsing features
bash build/build.sh --mode=all --version={version}
```

##### 2.3.2.2 Method 2: Building the msProf parsing runfile

```shell
# Build the parsing package independently
bash build/build.sh --mode=analysis --version={version}
```

#### 2.3.3 Installing the Runfile

1. The runfile is generated in the `output` directory. Run the following command to grant execute permissions for the runfile.

   ```shell
   cd output
   chmod +x mindstudio-profiler_{version}_{arch}.run
   ```

2. Run the installation command.

   ```shell
   ./mindstudio-profiler_{version}_{arch}.run --install
   ```

   The installation command supports options such as `--install-path`. For details, see [Command-line Options for Installing a Runfile](#52-command-line-options-for-installing-a-runfile).

   When the installation command is executed, the `--check` option is automatically executed to check the consistency and integrity of the software package. If the following information is displayed, the software package is verified successfully.

   ```ColdFusion
   Verifying archive integrity...  100%   SHA256 checksums are OK. All good.
   ```

   If the following information is displayed, the software is successfully installed.

   ```ColdFusion
   mindstudio-profiler package install success, the path is '{install_path}'.
   ```

## 3. Uninstallation

To uninstall the tool, perform the following steps:

1. Download the script.

   ```bash
   curl -O https://inst.obs.cn-north-4.myhuaweicloud.com/26.0.0/ms_install.py
   ```

   > [!NOTE]
   >
   > - Internet access is required to download the script. If your target environment is offline or does not allow internet access, download the script on an internet-connected device first, then copy it to the target device.
   > - If the command does not respond, or you encounter connection failures, SSL certificate errors, or other issues, refer to the [FAQ](https://www.hiascend.com/developer/blog/details/02176213671719317003).

2. Uninstall the tool.

   ```bash
   python ms_install.py uninstall {tools_name}
   ```

   Replace `{tools_name}` with the name of the tool to be uninstalled. You can run the `python ms_install.py help` command to query the tool name, which is displayed under the `Available Tools` field in the command output.

   If the uninstallation is successful, the following information is displayed:

   ```ColdFusion
   Successfully uninstalled 1 tool ({tools_name})
   ```

## 4. Upgrade

Upgrades follow the "uninstall first, then install" process. Simply run the installation command. The tool will automatically remove the previous version and guide you through the upgrade process.

You can use the `msprof --version` command to view the version information of the current environment, and then select the version to upgrade to. When upgrading the version, you need to pay attention to the version compatibility relationship. Please refer to the [Release Notes](https://gitcode.com/Ascend/release-management/blob/master/MindStudio/26.0.0/release_notes_en.md).

## 5. Appendixes

### 5.1 Command-line Options for Building a Runfile

The following table describes the command-line options for building an msProf runfile.

| Option               | Mandatory (Yes/No)| Description                                                                                                                                           |
|-------------------|-------|-----------------------------------------------------------------------------------------------------------------------------------------------|
| --build_type      | No   | Specifies the runfile build type. Valid values:<br>&#8226; `Release`: builds a software package for deployment in the production environment.<br>&#8226; `Debug`: builds a software package for development and debugging. This value supports only building runfiles with the **parsing** feature.<br>Default value: `Release`     |
| --mode            | No   | Specifies the features to be included in the runfile. Valid values:<br>&#8226; `all`: builds a software package that includes both the msProf collection and parsing features.<br>&#8226; `analysis`: builds a software package that includes only the msProf parsing feature.<br>Default value: `analysis`|
| --version | No   | Specifies the version number of the runfile.<br>Default value: `none`                                                                                                                    |

### 5.2 Command-line Options for Installing a Runfile

The following table describes the command-line options for installing an msProf runfile.

| Option    | Mandatory (Yes/No)| Description                                                                                                                                            |
| --------| -------  |------------------------------------------------------------------------------------------------------------------------------------------------|
| --install | No| Installs the software package. You can use the `--install-path` option to specify the software installation path. If the `--install-path` option is not specified, the software is installed in the default path.                                                                          |
| --uninstall | No| Uninstalls the software package. You can use the `--install-path` option to specify the software installation path. If the `--install-path` option is not specified, the software is uninstalled from the default path.                                                                   |
| --install-path | No| Specifies the installation path. The path must point to the `cann` directory. If this option is not specified, the software is installed in the default path. Default installation paths are as follows:<br>&#8226; root user: `/usr/local/Ascend/cann`<br>&#8226; Non-root user: `${HOME}/Ascend/cann`, where `${HOME}` indicates the home directory of the current user.|
| --install-for-all | No| Allows other users to have the permission of the installation user group during installation. If this option is specified during installation, other users can use msProf to run services. However, this option has security risks. Exercise caution when using this option.                                                                           |
| --help | No| Displays help information.                                                                                                                                       |
