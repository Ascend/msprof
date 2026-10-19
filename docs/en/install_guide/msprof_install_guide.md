# msProf Installation Guide

## 1. Installation Overview

This tool has been integrated into CANN. If CANN has been installed and this tool does not need to be updated, you can directly use it without following the instructions in this document.

If CANN has not been installed in your environment, install the Ascend NPU driver and CANN software (including the Toolkit and ops) by referring to [CANN Quick Installation](https://www.hiascend.com/cann/download), and configure environment variables.

If you need to upgrade this tool separately or use the latest version, you can install it in any of the following ways: [Online Installation](#21-online-installation), [Offline Installation](#22-offline-installation), and [Source Installation](#23-source-installation).

## 2. Installation Methods

### 2.1 Online Installation

If your device has Internet access, you can run a single command to automatically download and install the tool. Visit the Visit the Ascend community's [MindStudio download page](https://www.hiascend.com/developer/software/mindstudio/download), select the target CANN version, and choose "Online Installation". The system will guide you through the subsequent operations.

### 2.2 Offline Installation

For devices that are not connected to the Internet, such as those on an enterprise intranet, download the complete offline installation package on a device that has Internet access and then transfer the package to the target device for installation. Visit the Visit the Ascend community's [MindStudio download page](https://www.hiascend.com/developer/software/mindstudio/download), select the target CANN version, and choose "offline installation". The system will guide you through the subsequent operations.

### 2.3 Source Installation

If you need to use the features of the latest code, or modify the source code to enhance functionality, you can download the code from this repository, compile and package the tool yourself, and complete the installation.

#### 2.3.1 Environment Setup

Source builds must use the standard MindStudio build environment.

- For daily development or when using released images, see *[MindStudio Tool Development Environment Installation Guide](https://gitcode.com/Ascend/msot/blob/26.2.0/docs/en/common/dev_env_setup.md)*.
- To reproduce the environment from a base operating system, perform source build verification, or unit test verification, you must refer to *[MindStudio Unified Build Image Creation Guide](https://gitcode.com/Ascend/msot/blob/26.2.0/docs/en/common/docker_image_build_guide.md)* and build the environment image locally from the openEuler base image.

The source build and unit test commands described later in this document are all executed in the container of the specified image or the container of the environment image built on site. The CANN software package version, GCC version, and Python version are subject to the unified image creation guide, and this repository does not maintain them separately.

After the image is built, you must use the `ctr_in.py` command provided in Chapter 7 of the unified image creation guide to start and enter the container in an interactive terminal. Do not create a container using a standard `docker run` command, and do not use `docker exec <container_name> bash -c '<command>'` as a substitute for the interactive environment; otherwise, the Python, GCC, and CANN environment initialization may be skipped.

After entering the interactive container shell opened by `ctr_in.py`, run the following command to clone this repository:

```bash
cd ~
git clone https://gitcode.com/Ascend/msprof.git -b 26.2.0
```

#### 2.3.2 Building

In the same interactive container shell started using `ctr_in.py`, run the following commands from the repository root directory to download dependencies and build the tool:

```bash
cd ~/msprof
python3 build.py
```

After a successful build, the installation package is generated in the `artifacts/` directory.

#### 2.3.3 Running Unit Tests (Optional)

This step is not required for installation. If you need to verify the basic functionality of the code, you can run unit tests:

```bash
cd ~/msprof
python3 build.py test
```

A command return code of 0 with no failed test cases indicates that the unit tests have passed.

#### 2.3.4 Installing the Runfile

The generated whl package is packaged into the runfile, so you can install the runfile directly:

1. Run the following command to make the runfile executable:

   ```shell
   cd ~/msprof/artifacts
   chmod +x mindstudio-profiler_<version>_{arch}.run
   ```

2. Run the installation command.

   ```shell
   ./mindstudio-profiler_<version>_{arch}.run --install
   ```

   The installation command supports parameters such as `--install-path`. For details, see [Command-line Options for Installing a Runfile](#61-command-line-options-for-installing-a-runfile).

   If the following information is displayed, the software is successfully installed.

   ```text
   mindstudio-profiler package install success.
   ```

## 3. Verifying the Installation

After the installation is complete, run the following command to check whether the installation is successful:

```bash
msprof --help
```

If no error is reported and the help information is displayed, the installation is successful.

If running `msprof --help` results in a command-not-found error, check whether the current terminal is using the Python environment where `msprof` is installed.

## 4. Uninstallation

To uninstall the tool, perform the following steps:

1. Download the script.

   ```bash
   curl -O https://inst.obs.cn-north-4.myhuaweicloud.com/26.2.0/ms_install.py
   ```

   > [!NOTE]
   >
   > - Internet access is required to download the script. If your target environment is offline or does not allow internet access, download the script on an internet-connected device first, then copy it to the target device.
   > - If the command does not respond, or you encounter connection failures, SSL certificate errors, or other issues, refer to the [FAQs](https://www.hiascend.com/developer/blog/details/02176213671719317003).

2. Uninstall the tool.

   ```bash
   python ms_install.py uninstall {tools_name}
   ```

   Replace `{tools_name}` with the name of the tool to be uninstalled. You can run the `python ms_install.py help` command to query the tool name, which is displayed under the `Available Tools` field in the command output.

   If the uninstallation is successful, the following information is displayed:

   ```text
   Successfully uninstalled 1 tool ({tools_name})
   ```

## 5. Upgrade

Upgrades follow the "uninstall first, then install" process. Simply run the installation command. The tool will automatically remove the previous version and guide you through the upgrade process.

You can run the `msprof` `--version` command to view the version information of the current environment, and then select the version to upgrade. When upgrading, pay attention to the version compatibility. For details, see *[Release Notes](https://gitcode.com/Ascend/release-management/blob/master/MindStudio/26.2.0/release_notes.md)*.

## 6. Appendixes

### 6.1 Command-line Options for Installing a Runfile

The following table describes the command-line options for installing a runfile of msProf.

| Option             | Mandatory (Yes/No)| Description                                                        |
| ----------------- | --------- | ------------------------------------------------------------ |
| --install         | No     | Installs the software package. You can use the `--install-path` option to specify the software installation path. If the `--install-path` option is not specified, the software is installed in the default path.|
| --uninstall       | No     | Uninstalls the software package. You can use the `--install-path` option to specify the software installation path. If the `--install-path` option is not specified, the software is uninstalled from the default path.|
| --install-path    | No     | Specifies the installation path. The path must point to the `cann` directory. If this option is not specified, the software is installed in the default path. Default installation paths are as follows:<br>&#8226; root user: `/usr/local/Ascend/cann`<br>&#8226; Non-root user: `${HOME}/Ascend/cann`, where `${HOME}` indicates the home directory of the current user.|
| --install-for-all | No     | Allows other users to have the permission of the installation user group during installation. If this option is specified during installation, other users can use msProf to run services. However, this option has security risks. Exercise caution when using this option.|
| --help            | No     | Displays help information.                                              |
