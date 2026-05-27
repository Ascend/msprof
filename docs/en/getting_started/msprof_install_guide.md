# MindStudio Profiler Installation Guide

MindStudio Profiler (msProf) can be installed using the following methods:

- CANN package installation: The full features of msProf are integrated into the CANN package. You can install it directly by following the [CANN Quick Installation Guide](https://www.hiascend.com/cann/download).
- Runfile installation: The full features of msProf are integrated into the CANN package and depend on CANN. Therefore, you must install the CANN package before using msProf. To upgrade to the latest features from the code repository, you can [download](#downloading-and-installing-the-runfile) or [build](#building-and-installing-the-runfile) a runfile to perform an overwrite installation of the msProf package within an existing CANN environment.

## Downloading and Installing the Runfile

To install the msProf runfile, you must first obtain the standalone msProf release package. You can visit the [msProf Release](https://gitcode.com/Ascend/msprof/releases) page, select the target version, and download the `mindstudio-profiler_<version>_<arch>.run` installation package that matches the current system architecture. If the SHA256 checksum file is provided on the release page for the corresponding version, download it as well to perform an integrity verification before installation.

> [!note]NOTE
>
> The downloaded runfile of msProf must be installed as an overwrite in an environment where CANN is already installed.

1. After downloading the runfile of msProf, run the following command to grant the execute permission on the run package:

   ```shell
   chmod +x mindstudio-profiler_<version>_<arch>.run
   ```

2. Verify the integrity of the installation package.

   ```shell
   # If a SHA256 checksum file is provided on the release page, perform the verification directly.
   sha256sum -c <sha256_file>
   
   # If a SHA256 digest is provided, run the following command and manually verify the output against the value on the release page.
   sha256sum mindstudio-profiler_<version>_<arch>.run
   
   # You can also use `--check` to perform an integrity verification.
   ./mindstudio-profiler_<version>_<arch>.run --check
   ```

   If the following information is displayed, the software package integrity verification is successful.

   ```ColdFusion
   Verifying archive integrity...  100%   SHA256 checksums are OK. All good.
   ```

3. After the verification is successful, run the following command to complete the installation:

   ```shell
   ./mindstudio-profiler_<version>_<arch>.run --install
   ```

4. To specify a custom installation directory, use the `--install-path=<cann_path>` option. The path must point to a valid `CANN` directory. For more details, see [Command-line Options for Installing the Runfile](#command-line-options-for-installing-the-runfile).

## Building and Installing the Runfile

To use the latest features, you can download the source code from this repository to build, package, and install the tool.

> [!note]NOTE
> 
> The built runfile of msProf must be installed as an overwrite in an environment where CANN is already installed.

### Build Environment Setup

1. Install dependencies.

   msProf requires SQLite3 to build from source. Run the following commands to install SQLite3 or ensure it is already installed in your environment.

   - Install SQLite3 on Ubuntu.

     ```shell
      sudo apt update
      sudo apt install sqlite3 libsqlite3-dev
     ```

   - Install SQLite3 on openEuler or CentOS.

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

### Building and Packaging

The `build/build.sh` build script allows you to specify the build type by using the `--mode` option:

- `all`: builds the full-featured runfile (including collection and parsing features).
- `analysis`: builds the parsing runfile (including only the parsing feature).

For details about more parameters, see [Command-line Options for Building a Runfile](#command-line-options-for-building-a-runfile).

After the build is complete, the runfile is generated in the `output` directory within the current path. The file name format is `mindstudio-profiler_<version>_<arch>.run`. `version` represents the version number and `arch` represents the system architecture (automatically adapted based on the current system).

#### Method 1: Building the full-featured runfile of msProf (recommended)

```shell
# Build the full-featured runfile, including msProf collection and parsing features
bash build/build.sh --mode=all --version=<version>
```

#### Method 2: Building the msProf parsing runfile

```shell
# Build the parsing package independently
bash build/build.sh --mode=analysis --version=<version>
```

### Installing the Runfile

1. The runfile is generated in the `output` directory. Run the following command to grant the execute permission to the runfile:

   ```shell
   cd output
   chmod +x mindstudio-profiler_<version>_<arch>.run
   ```

2. Run the installation command.

   ```shell
   ./mindstudio-profiler_<version>_<arch>.run --install
   ```

   The installation command supports options such as `--install-path`. For details, see [Command-line Options for Installing the Runfile](#command-line-options-for-installing-the-runfile).

   When the installation command is executed, the `--check` option is automatically executed to check the consistency and integrity of the software package. If the following information is displayed, the software package is verified successfully.

   ```ColdFusion
   Verifying archive integrity...  100%   SHA256 checksums are OK. All good.
   ```

   If the following information is displayed, the software is successfully installed.

   ```ColdFusion
   mindstudio-profiler package install success.
   ```

## Appendixes

### Command-line Options for Building a Runfile

The following table describes the command-line options for building a runfile of msProf.

| Option | Mandatory (Yes/No) | Description |
| ------------ | --------- | ------------------------------------------------------------ |
| --build_type | No     | Specifies the runfile build type. Valid values:<br>&#8226; `Release`: builds a package for deployment in the production environment.<br>&#8226; `Debug`: builds a package for development and debugging. This value supports only building runfiles with the parsing feature.<br>Default value: `Release`|
| --mode       | No     | Specifies the features to be included in the runfile. Valid values:<br>&#8226; `all`: builds a software package that includes both the msProf collection and parsing features.<br>&#8226; `analysis`: builds a software package that includes only the msProf parsing feature.<br>Default value: `analysis`|
| --version    | No     | Specifies the version number of the runfile.<br>Default value: `none`                      |

### Command-line Options for Installing the Runfile

The following table describes the command-line options for installing a runfile of msProf.

| Option             | Mandatory (Yes/No)| Description                                                        |
| ----------------- | --------- | ------------------------------------------------------------ |
| --install         | No     | Installs the software package. You can use the `--install-path` option to specify the software installation path. If the `--install-path` option is not specified, the software is installed in the default path.|
| --uninstall       | No     | Uninstalls the software package. You can use the `--install-path` option to specify the software installation path. If the `--install-path` option is not specified, the software is uninstalled from the default path.|
| --install-path    | No     | Specifies the installation path. The path must point to the `cann` directory. If this option is not specified, the software is installed in the default path. Default installation paths are as follows:<br>&#8226; root user: `/usr/local/Ascend/cann`<br>&#8226; Non-root user: `${HOME}/Ascend/cann`, where `${HOME}` indicates the home directory of the current user.|
| --install-for-all | No     | Allows other users to have the permission of the installation user group during installation. If this option is specified during installation, other users can use msProf to run services. However, this option has security risks. Exercise caution when using this option.|
| --help            | No     | Displays help information.                                              |
