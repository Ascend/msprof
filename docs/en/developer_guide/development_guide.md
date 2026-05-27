# Developer Guide

This document is intended for MindStudio Profiler developers and maintenance personnel. It describes the source code directory structure, build methods, data collection and parsing workflows, verification methods for feature changes, and requirements for synchronized documentation updates. This guide is based on the current MindStudio Profiler (msProf) repository and existing documentation. It applies to scenarios such as adding command-line parameters, extending parsing capabilities, adding deliverables, or maintaining runfile installation methods.

## 1. msProf Development Overview

msProf provides capabilities for collecting, parsing, and exporting AI task execution profile data and Ascend AI processor system data. Development work is generally categorized into the following areas.

| Development Object| Typical Content|
| --- | --- |
| Data collection| `msprof` command-line parameters, collection process, and raw data persistence|
| Data parsing| `msprof --export`, `msprof.py (import/query/export)`, and communication data parsing|
| Export deliverables| `msprof_*.json`, `op_summary_*.csv`, and `msprof_*.db`|
| Packaging and release| Building the `mindstudio-profiler_<version>_<arch>.run` package by using `build/build.sh`|
| Documentation| Installation guide, quick start, parsing description, data file reference, and extended features|

## 2. Code Directory

The following table describes the primary directories in the msProf project repository.

| Directory| Description|
| --- | --- |
| `analysis` | Main directory for data parsing|
| `analysis/analyzer` | Communication data parsing directory|
| `analysis/framework` | Main parsing process directory|
| `analysis/interface` | Parsing interfaces directory|
| `analysis/msinterface` | Command-line option parsing directory|
| `analysis/msmodel` | .db data processing directory|
| `analysis/msparser` | Binary data parsing process management directory|
| `analysis/msprof` | Entry point for msProf|
| `analysis/viewer` | Directory storing export deliverables such as timeline, summary, and .db files|
| `build` | Build directory containing `build.sh`|
| `scripts` | Scripts related to third-party dependency download and runfile installation/upgrade|
| `test` | C++ and Python test code|
| `docs/en` | English documentation|

Before starting development, identify the specific layer that your changes will affect:

1. For changes in command-line parameters, check the `analysis/msinterface` directory and external documentation first.
2. For changes in parsing logic, check the `analysis/framework`, `analysis/msparser`, and `analysis/msmodel` directories first.
3. For changes in export files, check the `analysis/viewer` directory and data file reference documents first.
4. For changes in installation and release, check the `build` and `scripts` directories and installation guide first.

## 3. Development Environment Settings

### 3.1 Foundational Software

| Software| Version Requirement| Purpose|
| --- | --- | --- |
| Git | No specific requirement| Code pulling and committing|
| Python | 3.7.5 or later| Parsing script execution|
| SQLite3 | Building dependency| Parsing-related capabilities|
| Bash | Recommended for Linux environments| Build and script execution|

### 3.2 Prerequisites

1. A compatible version of the CANN environment has been installed.
2. The `cann` installation directory is available.
3. The SQLite3 dependencies required for building from source have been installed.

Example:

```bash
sudo apt update
sudo apt install sqlite3 libsqlite3-dev
```

or:

```bash
sudo yum install sqlite sqlite-devel
```

## 4. Obtaining Code and Building

### 4.1 Obtaining Code

```bash
git clone https://gitcode.com/Ascend/msprof.git
cd msprof
```

### 4.2 Downloading Third-party Dependencies

```bash
bash scripts/download_thirdparty.sh
```

### 4.3 Building the Runfile

`build/build.sh` allows you to specify the capability to be included in the runfile by using `--mode`.

| Mode| Description|
| --- | --- |
| `all` | Builds a full-featured runfile with collection and parsing capabilities.|
| `collector` | Builds a runfile with only collection capabilities.|
| `analysis` | Builds a runfile with only parsing capabilities.|

Example:

```bash
bash build/build.sh --mode=all --version=<version>
```

or:

```bash
bash build/build.sh --mode=analysis --version=<version>
```

Upon a successful build, the `mindstudio-profiler_<version>_<arch>.run` file will be generated in the `output` directory.

### 4.4 Installing the Runfile

```bash
cd output
chmod +x mindstudio-profiler_<version>_<arch>.run
./mindstudio-profiler_<version>_<arch>.run --install
```

Upon successful installation, run the following commands to verify the installation:

```bash
which msprof
msprof --help
```

## 5. Feature Development Process

### 5.1 Adding or Modifying Command-Line Parameters

If a change involves command-line parameters for `msprof` or `msprof.py`, perform at least the following tasks:

1. Implement parameter parsing and default value processing.
2. Validate parameter constraints, mutual exclusivity, and error messaging.
3. Verify help information, command examples, and output descriptions.
4. Update parsing specifications or extended feature documentation accordingly.

The following table describes the common documents to be updated.

| Change Type| Document to Update|
| --- | --- |
| `msprof --export` parameters| `en/user_guide/msprof_parsing_instruct.md`|
| `msprof.py import/query/export` parameters| `en/user_guide/extended_functions.md`|
| Installation, uninstallation, and verification parameters| `en/getting_started/msprof_install_guide.md`|
| Quick Start paths or basic examples| `en/getting_started/quick_start.md`|

### 5.2 Adding Parsing Capabilities

If a change involves new parsing logic (such as adding statistical items, supporting new collection scenarios, or enhancing communication analysis), perform the following self-checks in order:

1. Check whether the raw data can be successfully imported without errors.
2. Check whether `msprof --export=on` generates files as expected.
3. Check whether the `python3 msprof.py import/query/export` commands maintain consistent behavior.
4. Check whether the exported timeline, summary, and .db files contain the expected content.
5. Check whether error messages in failure scenarios provide enough information for troubleshooting.

### 5.3 Adding or Modifying Deliverables or Fields

If a change introduces or modifies deliverables (such as adding `xx_*.csv` files, changing the hierarchy in `msprof_*.json`, or adding fields to database tables), perform at least the following tasks:

1. Define the deliverable name, generation triggers, and output path.
2. Check whether the change affects the `mindstudio_profiler_output` directory structure.
3. Specify the meaning, units, value ranges, and applicable scenarios for all new or modified fields.
4. Update the data file reference documents accordingly.

The following table describes the key documents to be updated.

| Change Object| Document to Update|
| --- | --- |
| Timeline, summary, or text deliverables| `en/user_guide/profile_data_file_references.md`|
| Database tables or fields| `en/user_guide/profile_data_file_references_db.md`|
| User-visible new capabilities| `en/user_guide/extended_functions.md`|

## 6. Development & Verification

### 6.1 Collection Verification

After feature development is complete, perform a minimum verification of the collection process:

```bash
msprof --application="python train.py" --output=/home/prof_output
```

If the feature is functioning correctly, a `PROF_XXX` directory will be generated in the output path with the following structure:

```text
PROF_XXX
├── host
│   └── data
├── device_{id}
│   └── data
├── msprof_{timestamp}.db
└── mindstudio_profiler_output
    ├── msprof_{timestamp}.json
    ├── op_summary_{timestamp}.csv
    └── ...
```

### 6.2 Parsing Verification

For changes to the parsing workflow, verify at least the following commands:

```bash
msprof --export=on --output=/home/profiler_data/PROF_XXX
```

```bash
python3 msprof.py import -dir /home/profiler_data/PROF_XXX
```

```bash
python3 msprof.py query -dir /home/profiler_data/PROF_XXX
```

```bash
python3 msprof.py export timeline -dir /home/profiler_data/PROF_XXX
```

If necessary, run the following commands to perform additional verification:

```bash
python3 msprof.py export summary -dir /home/profiler_data/PROF_XXX
python3 msprof.py export db -dir /home/profiler_data/PROF_XXX
```

### 6.3 Result Verification

Focus on the following items:

1. Check whether expected files are generated in the `mindstudio_profiler_output` directory.
2. Check whether `msprof_*.json` can be properly loaded by visualization tools.
3. Check whether all fields are complete in `op_summary_*.csv` and `op_statistic_*.csv`.
4. Check whether `msprof_*.db` is successfully generated.
5. If the feature involves `--reports`, `--iteration-id`, `--model-id`, or `--clear`, check whether the test cases cover these parameters.

## 7. Requirements for Documentation Updates

Development changes to msProf directly impact user documentation. Updating code without corresponding documentation updates is strictly prohibited. Use the following table to identify documents that require updates.

| Change Content| Document to Update|
| --- | --- |
| Installation methods, dependencies, or packaging parameters| `en/getting_started/msprof_install_guide.md`|
| Basic usage workflows or examples| `en/getting_started/quick_start.md`|
| Parsing workflows or command-line parameters| `en/user_guide/msprof_parsing_instruct.md`|
| Script capabilities or advanced usage| `en/user_guide/extended_functions.md`|
| Exported files, fields, or layers| `en/user_guide/profile_data_file_references.md`|
| DB data structure descriptions| `en/user_guide/profile_data_file_references_db.md`|
| Product or capability overview| `en/overview.md`|

If the change involves new screenshots or diagrams:

1. Save all image files in the `en/figures` directory.
2. File names must correspond to the semantic meaning of the feature.
3. Update figure titles, file paths, and descriptive text within the document accordingly.

## 8. Pre-submission Checklist

Before submission, perform at least the following checks.

| Check Item| Description|
| --- | --- |
| Build check| Check whether the runfile can be successfully compiled.|
| Installation check| Check whether the runfile can be successfully installed and whether `msprof --help` functions as expected.|
| Collection check| Check whether the `PROF_XXX` directory can be generated.|
| Export check| Check whether the JSON, CSV, or DB deliverables can be generated as expected.|
| Document check| Check whether the parameters, examples, and output descriptions have been updated accordingly.|
| Compatibility check| Check whether the default scenarios are regression-free and whether existing parameter behavior remains unchanged.|

If the change involves user-visible behavior, clearly specify the following information in the commit message:

1. The affected workflow (collection, parsing, export, or installation)
2. Whether any parameters, deliverables, or data fields are added or modified
3. The specific commands and scenarios used for verification
4. The scope of documentation that has been updated
