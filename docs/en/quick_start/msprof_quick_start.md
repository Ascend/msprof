# msProf Quick Start

<br>

## 1. Overview

MindStudio Profiler (msProf) is a profile data collection and analysis tool for Ascend AI Processors. Using a complete performance analysis as an example, this tutorial introduces the basic workflow for environment setup, profile data collection, result viewing, and visual performance analysis.

**Quick Start Roadmap (Core operations take approximately 10 minutes)**

| Step | Stage | Core Tool | Estimated Operation Time | Recommended Learning Time |
| :---: | :--- | :--- | :---: | :---: |
| **1** | **Environment setup** | CANN container environment | 5 minutes | 5 minutes |
| **2** | **Profile data collection and CSV result viewing** | msProf command-line tool | 2 minutes | 10 minutes |
| **3** | **Visual performance analysis (optional)** | MindStudio Insight | 3 minutes | 10 minutes |

## 2. Procedure

### 2.1 Environment Setup (Mandatory)

🛑 **This section is mandatory. Skipping this section may cause multiple subsequent operations to fail.**

This tutorial **supports only** execution in a standardized CANN container. Direct execution on bare-metal servers, virtual machines, or other non-standard container environments is not supported.

#### 2.1.1 Prerequisites

Before starting, make sure that the server meets the following requirements.

| Item | Requirement | Verification Method |
| :--- | :--- | :--- |
| **Hardware** | A Linux server equipped with at least one NPU, with the driver and firmware installed | Run `npu-smi info` and verify that the NPU is operating normally |
| **Container runtime** | Docker installed and running (version ≥ 18.0 recommended) | Run `docker ps`; no error indicates that the service is running normally |
| **Script execution** | Python 3 installed on the host | Run `python3 -V` on the host; displayed version information indicates that it is installed |
| **Network communication** | `curl` installed (any version) | Run `curl -V`; displayed version information indicates that it is installed |

> 👉 After confirming that the prerequisites are met, if the environment has public network access, all subsequent commands in this chapter can be executed directly by **Copy/Paste**, without manual input or concatenation. This helps avoid command execution failures caused by input errors.

#### 2.1.2 Host: Automatically Identifying the NPU and Configuring Image Environment Variables

Run the following command on the host. This command performs three operations: (1) obtains the NPU PCI ID → (2) matches the image version → (3) sets the environment variables for use in subsequent steps.

```bash
source /dev/stdin <<< "$(dev_id=$(lspci -n -D | grep -o '19e5:d[0-9a-f]\{3\}' | head -n1 | cut -d: -f2); case "$dev_id" in 'd500' ) echo "export MY_STUDY_VAR_CANN_IMAGE=swr.cn-south-1.myhuaweicloud.com/ascendhub/cann:9.0.0-310p-openeuler24.03-py3.11-devel; export MY_CHIP_NAME=310P";; 'd802' ) echo "export MY_STUDY_VAR_CANN_IMAGE=swr.cn-south-1.myhuaweicloud.com/ascendhub/cann:9.0.0-910b-openeuler24.03-py3.11-devel; export MY_CHIP_NAME=910B";; 'd803' ) echo "export MY_STUDY_VAR_CANN_IMAGE=swr.cn-south-1.myhuaweicloud.com/ascendhub/cann:9.0.0-a3-openeuler24.03-py3.11-devel; export MY_CHIP_NAME=A3";; 'd806' ) echo "export MY_STUDY_VAR_CANN_IMAGE=swr.cn-south-1.myhuaweicloud.com/ascendhub/cann:9.0.0-950-openeuler24.03-py3.11-devel; export MY_CHIP_NAME=950";; * ) echo "unset MY_STUDY_VAR_CANN_IMAGE MY_CHIP_NAME; echo >&2; echo -e '\033[31m[FAIL] Get device ID: $dev_id. Learning is not supported in the current environment.\033[0m' >&2";; esac)"
[ -n "$MY_STUDY_VAR_CANN_IMAGE" ] && echo -e "\e[32m[PASS] Successfully identified chip [$MY_CHIP_NAME] and auto-selected image:\n    $MY_STUDY_VAR_CANN_IMAGE\e[0m"
```

> [!NOTE]
>
> **How the Command Works**
> The command obtains the NPU PCI ID using `lspci`, automatically matches the official CANN image, and assigns the image address to the `MY_STUDY_VAR_CANN_IMAGE` environment variable for subsequent use.
> All images are official CANN images published on Huawei Cloud AscendHub. For details about the images, see the [CANN Official Image Repository](https://www.hiascend.com/developer/ascendhub/detail/17da20d1c2b6493cb38765adeba85884).

If the command outputs `[PASS]`, the operation is successful. If it outputs `[FAIL]`, the possible causes are as follows:

1. The hardware is outside the supported scope of this tutorial: this learning environment supports only Ascend 310P, A2, A3, and 950 series products. Switch to a compatible hardware environment and try again.
2. The underlying environment is abnormal: `lspci` is not installed, or the current user cannot query the NPU PCI ID using `lspci -n -D`. Contact the environment administrator to verify the underlying environment.

#### 2.1.3 Host: Pulling the Image

Run the following command on the host:

```bash
docker pull ${MY_STUDY_VAR_CANN_IMAGE}
```

If the image cannot be pulled because you are on an enterprise intranet, see [Section 3.1](#31-obtaining-the-docker-image-in-an-isolated-intranet-environment) for a solution.

#### 2.1.4 Host: Downloading the Container Startup Script

Run the following command on the host:

```bash
cd ~ && curl -fLO --retry 3 https://inst.obs.cn-north-4.myhuaweicloud.com/env/ctr_in.py && chmod +x ctr_in.py
```

If the script cannot be downloaded due to network restrictions, see [Section 3.2](#32-transferring-the-container-startup-script) for a solution.

#### 2.1.5 Host: Starting the Container

Run the following command on the host and confirm the container creation information according to the terminal prompts:

```bash
~/ctr_in.py ${MY_STUDY_VAR_CANN_IMAGE}
```

**Expected result**: If the terminal displays a root shell prompt similar to the following, the container has started successfully:

```text
[root@xxxxxx ~]#
```

If an error is reported or a container selection interface appears, return to [Section 2.1.2](#212-host-automatically-identifying-the-npu-and-configuring-image-environment-variables), verify that the command output is `[PASS]`, and restart the container.

#### 2.1.6 Container: Installing Python Dependencies

Run the following commands in the container:

```bash
pip3 install networkx==3.6.1 pillow==12.2.0
pip3 install https://inst.obs.cn-north-4.myhuaweicloud.com/env/mirror/$(arch)/download.pytorch.org/whl/cpu/torch-2.7.1%2Bcpu-cp311-cp311-manylinux_2_28_$(arch).whl
pip3 install torchvision==0.22.1 --index-url https://download.pytorch.org/whl/cpu
pip3 install https://gitcode.com/Ascend/pytorch/releases/download/v26.0.0-pytorch2.7.1/torch_npu-2.7.1.post4-cp311-cp311-manylinux_2_28_$(arch).whl
```

If installation fails because you are on an enterprise intranet, see [Section 3.3](#33-offline-installation-of-python-dependencies) for a solution.

#### 2.1.7 Container: Verifying the Installation

After installation is complete, run the following environment verification command:

```bash
python3 -c 'import torch, torch_npu, torchvision; assert torch.npu.is_available(), "NPU is unavailable"; print("PyTorch:", torch.__version__)' && echo -e "\e[32m[PASS] NPU environment check passed.\e[0m"
```

If `[PASS]` is displayed, the NPU environment has been configured correctly and the dependencies have been installed successfully. You can proceed to the next step.

### 2.2 Collecting and Parsing Profile Data and Exporting Results

#### 2.2.1 Preparing the Model Training Code

Run the following command in the container to write the sample training code to `~/train.py`. The sample trains a model with random data for two epochs and is used only to generate profile data.

```bash
cat > ~/train.py << 'EOF'
import sys, re, subprocess, torch, torch.nn as nn, torch.optim as optim, torchvision.models as models # For brevity, the import statements are combined into a single line. This is not standard practice.

class ResNet50:
    def __init__(self, num_classes=1000, device=0):
        self.device = f"npu:{device}"
        torch.npu.set_device(self.device)
        print(f"[INFO] Using device: {self.device}")

        self.model = models.resnet50(weights=None, num_classes=num_classes).to(self.device)

    def train(self, data_loader, epochs=1, lr=1e-4, freeze_backbone=False):
        for p in self.model.parameters(): p.requires_grad = not freeze_backbone
        for p in self.model.fc.parameters(): p.requires_grad = True
        optimizer = optim.Adam([p for p in self.model.parameters() if p.requires_grad], lr=lr)
        criterion = nn.CrossEntropyLoss().to(self.device)

        self.model.train()
        for epoch in range(epochs):
            total_loss = 0.0
            for inputs, labels in data_loader:
                inputs, labels = inputs.to(self.device), labels.to(self.device)
                optimizer.zero_grad()
                loss = criterion(self.model(inputs), labels)
                loss.backward()
                optimizer.step()
                total_loss += loss.item()
            print(f"[Epoch {epoch + 1}/{epochs}] Average Loss: {total_loss / len(data_loader):.4f}")

def find_idle_npu():
    res = subprocess.run(["npu-smi", "info"], capture_output=True, text=True)
    if res.returncode != 0: sys.exit(f"ERROR: npu-smi info failed: {res.stderr.strip()}")
    idle = [int(x) for x in re.findall(r'No running processes found in NPU\s+(\d+)', res.stdout)]
    print(f"[INFO] Available (idle) NPU IDs: {idle}")
    if not idle: sys.exit("[WARNING] No idle NPU, please free resources or retry later.")
    return int(idle[0])

if __name__ == "__main__":
    dataset = torch.utils.data.TensorDataset(torch.randn(80, 3, 224, 224), torch.randint(0, 10, (80,)))
    ResNet50(num_classes=10, device=find_idle_npu()).train(torch.utils.data.DataLoader(dataset, batch_size=8, shuffle=True), epochs=2, lr=1e-3, freeze_backbone=True)
EOF
```

> [!CAUTION]
>
> The preceding script automatically finds an idle NPU and runs on it. To specify an NPU, replace `find_idle_npu()` in the last line with the actual NPU device number, such as `0`.

#### 2.2.2 Starting Training and Collecting Profile Data

Run the following command in the container to use msProf to execute the training script and collect profile data:

```bash
msprof --application="python3 ${HOME}/train.py" --output=${HOME}/prof_output
```

If information similar to the following is displayed, with `Profiling finished` and the data save path shown at the end, profile data collection and automatic parsing have completed successfully. The loss values, time, device IDs, and directory names may vary depending on the runtime environment.

```text
[INFO] Start profiling....
[INFO] Using device: npu:0
[Epoch 1/2] Average Loss: 2.4961
[Epoch 2/2] Average Loss: 2.2166
[INFO] Start export data in PROF_000001_20260323031749197_00815596RKPKAHRB..
...
[INFO] Export all data in PROF_000001_20260323031749197_00815596RKPKAHRB. done.
[INFO] Start query data in PROF_000001_20260323031749197_00815596RKPKAHRB..
Job Info        Device ID       Dir Name        Collection Time                 Model ID   Iteration Number Top Time Iteration      Rank ID
NA                              host            2026-03-23 03:17:50.944273      N/A        N/A              N/A                     -1
NA              0               device_0        2026-03-23 03:17:50.954390      N/A        N/A              N/A                     -1
[INFO] Query all data in PROF_000001_20260323031749197_00815596RKPKAHRB. done.
[INFO] Profiling finished.
[INFO] Process profiling data complete. Data is saved in /home/prof_output/PROF_000001_20260323031749197_00815596RKPKAHRB.
```

If the script reports that there is no idle NPU, terminate other NPU tasks or retry after they have completed. If the operation does not complete after more than five minutes, the NPU may be abnormal or may have been preempted. Run the command again or specify another idle NPU.

#### 2.2.3 Viewing the Collected Data

After the command completes, run the following commands to view information about the latest generated result directory:

```bash
PROF_DIR=$(ls -dt "${HOME}"/prof_output/PROF_* | head -n 1)
echo "${PROF_DIR}"
tree -L 1 "${PROF_DIR}"
tree -L 1 "${PROF_DIR}/mindstudio_profiler_output"
```

The `PROF_XXX` directory contains both raw data and parsed and exported results. The actual files depend on the collection content and export type. A common structure is as follows:

```text
PROF_XXX
├── host   # Host-side raw profile data; usually not relevant in the quick start
│   └── data
├── device_{id}   # Device-side raw profile data; usually not relevant in the quick start
│   └── data
├── msprof_{timestamp}.db  # Profile data in database format
└── mindstudio_profiler_output   # Aggregated profile data from the host and each device
    ├── msprof_{timestamp}.json  # Timeline data in Chrome Trace format
    ├── op_statistic_{timestamp}.csv  # Statistics aggregated by operator type
    ├── op_summary_{timestamp}.csv  # Detailed data for AICore and AICPU operators
    └── ...
```

##### 2.2.3.1 `op_statistic_*.csv`

Aggregates profile data by operator type, including key metrics such as the total execution time and number of calls for each operator type. The fields may vary depending on the product version and collection parameters. The following example is for reference only.

You can sort the data by `Total Time(us)` in descending order and focus first on operator types with a high proportion of the total execution time to evaluate their optimization potential.

![op_statistic CSV file example](../figures/en-us_image_0000002534398593.png)
<div style="text-align: center;">
<strong>Figure 1</strong> op_statistic_*.csv file example
</div>

##### 2.2.3.2 `op_summary_*.csv`

Records detailed execution information for each operator task. The fields may vary depending on the product version and collection parameters. The following example is for reference only.

You can sort the data by `Task Duration(us)` in descending order to locate long-running tasks, and then check `Task Type` to view the distribution of tasks on AICore and AICPU.

![op\_summary CSV file](../figures/en-us_image_0000002502718556.png)
<div style="text-align: center;">
<strong>Figure 2</strong> op_summary_*.csv file example
</div>

> [!NOTE]
>
> This tutorial introduces only basic analysis methods. For complete definitions of the output files and their fields, see [Profile Data File Reference](../user_guide/profile_data_file_references.md).

### 2.3 (Optional) Visual Performance Analysis

#### 2.3.1 Environment Setup

Perform operations in this section in a local environment with MindStudio Insight installed. Windows or macOS with a graphical user interface is recommended.

1. **Installing the tool**: If the tool is not installed, download and install it from the Ascend Community [MindStudio Download](https://www.hiascend.com/developer/software/mindstudio/download) page.
2. **Getting familiar with the tool**: For basic usage, see [MindStudio Insight System Tuning Quick Start](https://gitcode.com/Ascend/msinsight/blob/26.1.0/docs/en/quick_start/system_tuning_quick_start.md).

#### 2.3.2 Loading msProf Collected Data

1. Open MindStudio Insight.
2. Run the following command in the container to obtain the variable assignment statement from the output and copy it:

    ```bash
    echo "CONTAINER_ID=${HOSTNAME:-$(cat /etc/hostname)}; PROF_DIR=${PROF_DIR}"
    ```

3. Switch to the host terminal, paste and run the variable assignment statement copied in the previous step, and then use `docker cp` to copy the profile data from the container to the local environment:

    ```bash
    docker cp "${CONTAINER_ID}:${PROF_DIR}" ./
    ```

4. Import the copied `PROF_XXX` directory into MindStudio Insight.
5. Open the **Timeline** or **Operator** view to view the analysis results.

#### 2.3.3 Visual Analysis Example

The **Timeline** view in MindStudio Insight mainly consists of the following three functional areas:

* **Area 1 (CANN track)**: Displays the execution time of key interfaces in the software stack, such as **ACL** and **Runtime**.
* **Area 2 (NPU device track)**: Displays the execution sequence of each stream on **Ascend Hardware**, iteration traces, and processor system-level data.
* **Area 3 (Details panel)**: Displays detailed properties of the selected object. Click any colored block in the timeline to view complete information about the corresponding operator or API.

![MindStudio Insight Timeline](../figures/en-us_image_0000002502558722.png)
<div style="text-align: center;">
<strong>Figure 3</strong> Visual representation of msProf collected data
</div>

The preceding visual views can be used to efficiently perform the following performance analysis tasks:

* **Locating bottlenecks**: Quickly identify APIs, operators, and task flows with abnormal execution time.
* **Analyzing dispatch relationships**: Use `HostToDevice` connections to visually analyze scheduling dependencies and timing alignment between host-side and device-side tasks.

For more advanced operations and analysis methods, see [MindStudio Insight System Tuning Guide](https://gitcode.com/Ascend/msinsight/blob/26.1.0/docs/en/user_guide/system_tuning.md).

## 3. Solutions for Intranet Environments Without Public Network Access

### 3.1 Obtaining the Docker Image in an Isolated Intranet Environment

**Solution 1: Configuring a Docker Proxy for Direct Pulling**

This solution applies to most Linux distributions and Docker versions ≥ 18.0, but compatibility is not guaranteed in all scenarios. If an exception occurs, adjust the configuration according to the actual environment.

Edit the Docker service proxy configuration file `/etc/systemd/system/docker.service.d/http-proxy.conf`. An example of the file content is as follows. Replace the username, password, proxy address, and port based on your actual environment.

```text
[Service]
Environment="HTTP_PROXY=http://username:password@proxy.example.com:8080"
Environment="HTTPS_PROXY=http://username:password@proxy.example.com:8080"
Environment="NO_PROXY=localhost,127.0.0.1,.example.com"
```

Save the file, and then reload and restart the Docker service:

```bash
sudo systemctl daemon-reload
sudo systemctl restart docker
```

You can then run `docker pull` normally.

**Solution 2: Importing the CANN Image Offline**

If the proxy solution is not feasible, first run [Section 2.1.2](#212-host-automatically-identifying-the-npu-and-configuring-image-environment-variables) on the NPU server in the intranet and record the complete value of `MY_STUDY_VAR_CANN_IMAGE`. Then, run the following commands on an intermediate machine with public network access and the same CPU architecture, replacing the value in the first line with the recorded image address:

```bash
CANN_IMAGE='complete image address'
docker pull "${CANN_IMAGE}"
docker save -o cann.tar "${CANN_IMAGE}"
```

Transfer `cann.tar` to the intranet server using a USB flash drive or another method, and then run the following commands on the intranet server to load the image:

```bash
docker load -i cann.tar
docker images | grep cann
```

After the image is loaded, continue by following [Section 3.2](#32-transferring-the-container-startup-script) to transfer the container startup script. Then, return to [Section 2.1.5](#215-host-starting-the-container) and start the container. If you have switched to a different host shell, rerun the command in [Section 2.1.2](#212-host-automatically-identifying-the-npu-and-configuring-image-environment-variables) to restore the image environment variables.

### 3.2 Transferring the Container Startup Script

In a browser that can access the current webpage, enter the following URL to download the `ctr_in.py` script, and manually copy it to the `~/` directory on the intranet server:

```text
https://inst.obs.cn-north-4.myhuaweicloud.com/env/ctr_in.py
```

After copying the file, run the following commands on the host of the intranet server:

```bash
cd ~
chmod +x ctr_in.py
ls -l ctr_in.py
```

After confirming that `ctr_in.py` exists and has execute permission, return to [Section 2.1.5](#215-host-starting-the-container) and start the container.

### 3.3 Offline Installation of Python Dependencies

Prioritize using an intranet `pip` repository to install the dependencies. If no intranet software repository is available, download the required installation packages in an intermediate environment that has public network access, the same CPU architecture as the NPU server in the intranet, and the same Python version.

```bash
mkdir -p offline_wheels
python3 -m pip download xxx --dest offline_wheels
```

Transfer the `offline_wheels` directory to the intranet server and copy it to the user's home directory in the container. Then run the following command in the container:

```bash
pip3 install --no-index --find-links="${HOME}/offline_wheels" xxx
```

After installation is complete, return to [Section 2.1.7](#217-container-verifying-the-installation) and run the verification command. There is no need to rerun the network-based installation commands.

## 4. Frequently Asked Questions (FAQ)

### 4.1 How Do I Re-enter the Container After Exiting?

Run either of the following commands on the host:

**Method 1 (Recommended)**: Run `~/ctr_in.py` to interactively select the target container. If there is only one container, the script automatically enters it.

**Method 2 (Native command)**: Run `docker exec -it <container_name> bash` (replace `<container_name>` with the actual container name).

### 4.2 What Should I Do If a `permission denied` Error Occurs When Running Docker Commands?

The current user may not have been added to the Docker user group. You can use `root` privileges to run the following command on the host:

```bash
sudo usermod -aG docker <current_username>
```

After running the command, log in to the current user session again, or run `newgrp docker` to apply the group membership change immediately. Running routine operations as `root` is not recommended.
