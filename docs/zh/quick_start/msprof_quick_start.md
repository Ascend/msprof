# msProf 快速入门

<br>

## 1. 概述

msProf 是面向昇腾 AI 处理器的性能数据采集与分析工具。本文以一次完整的性能分析为例，介绍环境准备、数据采集、结果查看和可视化分析的基本流程。

**体验地图（核心操作约需 10 分钟）**

| 步骤 | 环节                 | 核心工具               | 参考操作耗时 | 建议原理学习耗时 |
| :---: |:-------------------|:-------------------|:------:|:------:|
| **1** | **环境准备**           | CANN 容器环境          |  5 分钟  |  5 分钟  |
| **2** | **数据采集与 CSV 结果查看** | msProf 命令行工具       |  2 分钟  | 10 分钟  |
| **3** | **可视化性能分析（可选）**    | MindStudio Insight |  3 分钟  | 10 分钟  |

## 2. 操作步骤

### 2.1 环境准备（必做）

🛑 **本节为强制前置步骤！跳过本节可能导致后续多项操作失败。**

本教程**仅支持**在标准化 CANN 容器中执行，不支持直接在裸机、虚拟机或其他非标准容器环境中执行。

#### 2.1.1 前置条件

开始前，请确认服务器满足以下要求：

| 项目   | 要求                                                        | 验证方法                           |
|------|-----------------------------------------------------------|--------------------------------|
| **硬件算力** | Linux 服务器配备至少 1 张 NPU 卡（基于昇腾 310P/910B/A3/A5 芯片），驱动与固件已安装 | 执行 `npu-smi info`，确认 NPU 卡状态正常 |
| **容器运行** | 已安装并运行 Docker（建议版本 ≥ 18.0）                                | 执行 `docker ps`，无报错即表示服务正常启动    |
| **脚本执行** | 宿主机已安装 Python 3                                           | 在宿主机执行 `python3 -V`，有版本信息输出即表示已安装 |
| **网络通信** | 已安装 curl（任意版本）                                            | 执行 `curl -V`，有版本信息输出即表示已安装     |

> 👉 确认前置条件满足后，若环境具备公网访问能力，本章后续命令可全程直接 **Copy/Paste** 执行，无需手动输入或拼接，以避免因输入错误导致命令执行失败。

#### 2.1.2 宿主机：基于芯片型号自动识别并配置镜像环境变量

在宿主机执行以下命令。该命令将根据当前 NPU 芯片型号匹配对应的 CANN 镜像，并将完整镜像地址写入环境变量，供后续流程使用：

```bash
source /dev/stdin <<< "$(dev_id=$(lspci -n -D | grep -o '19e5:d[0-9a-f]\{3\}' | head -n1 | cut -d: -f2); case "$dev_id" in 'd500' ) echo "export MY_STUDY_VAR_CANN_IMAGE=swr.cn-south-1.myhuaweicloud.com/ascendhub/cann:9.0.0-310p-openeuler24.03-py3.11-devel; export MY_CHIP_NAME=310P";; 'd802' ) echo "export MY_STUDY_VAR_CANN_IMAGE=swr.cn-south-1.myhuaweicloud.com/ascendhub/cann:9.0.0-910b-openeuler24.03-py3.11-devel; export MY_CHIP_NAME=910B";; 'd803' ) echo "export MY_STUDY_VAR_CANN_IMAGE=swr.cn-south-1.myhuaweicloud.com/ascendhub/cann:9.0.0-a3-openeuler24.03-py3.11-devel; export MY_CHIP_NAME=A3";; 'd806' ) echo "export MY_STUDY_VAR_CANN_IMAGE=swr.cn-south-1.myhuaweicloud.com/ascendhub/cann:9.0.0-950-openeuler24.03-py3.11-devel; export MY_CHIP_NAME=950";; * ) echo "unset MY_STUDY_VAR_CANN_IMAGE MY_CHIP_NAME; echo >&2; echo -e '\033[31m[FAIL] Get device ID: $dev_id. Learning is not supported in the current environment.\033[0m' >&2";; esac)"
[ -n "$MY_STUDY_VAR_CANN_IMAGE" ] && echo -e "\e[32m[PASS] Successfully identified chip [$MY_CHIP_NAME] and auto-selected image:\n    $MY_STUDY_VAR_CANN_IMAGE\e[0m"
```

> [!NOTE]说明
>
> **命令原理**  
> 通过 `lspci` 获取 NPU 的 PCI ID，自动匹配 CANN 官方镜像，并将镜像地址赋给环境变量 `MY_STUDY_VAR_CANN_IMAGE`，供后续使用。  
> 所有镜像均来自华为云 AscendHub 上发布的 CANN 官方镜像。如需了解镜像详情，请参阅 [CANN 官方镜像仓库](https://www.hiascend.com/developer/ascendhub/detail/17da20d1c2b6493cb38765adeba85884)。

若命令执行后输出 `[PASS]`，则表示执行成功；若输出 `[FAIL]`，可能原因如下：

1. 硬件不在本教程支持范围内：本学习环境仅支持昇腾 310P、910B、A3 及 A5 系列芯片，请切换至兼容的硬件环境后重试；
2. 底层环境异常：未安装 `lspci`，或当前用户无法通过 `lspci -n -D` 查询 NPU PCI ID，请联系环境管理员确认底层环境。

#### 2.1.3 宿主机：拉取镜像

在宿主机执行：

```bash
docker pull ${MY_STUDY_VAR_CANN_IMAGE}
```

若因处于企业内网导致拉取失败，请参考 [3.1 节](#31-docker-镜像在隔离内网的获取方法) 的解决方案。

#### 2.1.4 宿主机：下载容器启动脚本

在宿主机执行：

```bash
cd ~ && curl -fLO --retry 3 https://inst.obs.cn-north-4.myhuaweicloud.com/env/ctr_in.py && chmod +x ctr_in.py
```

若因网络限制无法下载，请参考 [3.2 节](#32-传输容器启动脚本) 的解决方案。

#### 2.1.5 宿主机：启动容器

在宿主机执行以下命令，并根据终端提示确认容器创建信息：

```bash
~/ctr_in.py ${MY_STUDY_VAR_CANN_IMAGE}
```

**预期结果**：终端进入类似以下 root Shell 提示符，表示容器已成功启动：

```text
[root@xxxxxx ~]#
```

若提示错误或出现容器选择界面，请返回 [第 2.1.2 节](#212-宿主机基于芯片型号自动识别并配置镜像环境变量)，确认命令输出 `[PASS]`，再重新启动容器。


#### 2.1.6 容器内：安装 Python 依赖

在容器内执行以下命令：

```bash
pip3 install networkx==3.6.1 pillow==12.2.0
pip3 install https://inst.obs.cn-north-4.myhuaweicloud.com/env/mirror/$(arch)/download.pytorch.org/whl/cpu/torch-2.7.1%2Bcpu-cp311-cp311-manylinux_2_28_$(arch).whl
pip3 install torchvision==0.22.1 --index-url https://download.pytorch.org/whl/cpu
pip3 install https://gitcode.com/Ascend/pytorch/releases/download/v26.0.0-pytorch2.7.1/torch_npu-2.7.1.post4-cp311-cp311-manylinux_2_28_$(arch).whl
```

若因处于企业内网导致安装失败，请参考 [3.3 节](#33-离线安装-python-依赖) 的解决方案。

#### 2.1.7 容器内：检查环境安装正确性

安装完成后执行环境检查命令：

```bash
python3 -c 'import torch, torch_npu, torchvision; assert torch.npu.is_available(), "NPU is unavailable"; print("PyTorch:", torch.__version__)' && echo -e "\e[32m[PASS] NPU environment check passed.\e[0m"
```

若显示 `[PASS]`，表示 NPU 环境正常配置完成，依赖安装正常，可以继续进行下一步操作。

### 2.2 采集、解析并导出性能数据

#### 2.2.1 准备模型训练代码

在容器内执行以下命令，将示例训练代码写入 `~/train.py`。该示例使用随机数据训练 2 个 epoch，仅用于生成性能数据。

```bash
cat > ~/train.py << 'EOF'
import sys, re, subprocess, torch, torch.nn as nn, torch.optim as optim, torchvision.models as models # 为精简篇幅，导入语句合并为单行（非规范写法）

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

> [!CAUTION]注意
>
> 上述脚本会自动查找空闲 NPU 并在该设备上执行。如需指定 NPU，请将最后一行代码中的 `find_idle_npu()` 替换为实际的 NPU 设备编号，例如 `0`。

#### 2.2.2 启动训练和采集

容器内执行以下命令，使用 msProf 启动训练脚本并采集性能数据：

```bash
msprof --application="python3 ${HOME}/train.py" --output=${HOME}/prof_output
```

显示类似以下信息，且末尾出现 `Profiling finished` 和数据保存路径，表示采集与自动解析成功。损失值、时间、设备编号和目录名称等信息会因运行环境而异。

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

#### 2.2.3 查看采集结果

命令执行完成后，运行以下命令查看本次最新生成的结果目录信息：

```bash
PROF_DIR=$(ls -dt "${HOME}"/prof_output/PROF_* | head -n 1)
echo "${PROF_DIR}"
tree -L 1 "${PROF_DIR}"
tree -L 1 "${PROF_DIR}/mindstudio_profiler_output"
```

`PROF_XXX` 目录同时包含原始数据和解析导出结果；实际文件取决于采集内容和导出类型，常见结构如下：

```text
PROF_XXX
├── host   # Host 侧性能原始数据，快速入门阶段通常无需关注
│   └── data
├── device_{id}   # Device 侧性能原始数据，快速入门阶段通常无需关注
│   └── data
├── msprof_{timestamp}.db  # 数据库格式的性能数据
└── mindstudio_profiler_output   # Host 侧和各 Device 侧性能数据汇总
    ├── msprof_{timestamp}.json  # Chrome Trace 格式的 Timeline 数据
    ├── op_statistic_{timestamp}.csv  # 按算子类型聚合的统计数据
    ├── op_summary_{timestamp}.csv  # AI Core 和 AI CPU 算子明细数据
    └── ...
```

##### 2.2.3.1 op_statistic_*.csv 功能介绍

按算子类型聚合统计性能数据，包含各类算子的总耗时、调用次数等关键指标（字段随产品版本和采集参数会变化，如下示例仅供学习参考）。

分析方法举例：可按 `Total Time(us)` 降序排列，优先关注耗时占比高的算子类型，进而评估其优化潜力。

![op_statistic CSV 文件示例](../figures/zh-cn_image_0000002534398593.png)
<div style="text-align: center;">
<strong>图1</strong> <code>op_statistic_*.csv</code> 文件示例
</div>

##### 2.2.3.2 op_summary_*.csv 功能介绍

记录每个算子任务的执行明细（字段随产品版本和采集参数会变化，如下示例仅供学习参考）。

分析方法举例：可按 `Task Duration(us)` 降序排列以定位耗时较长的任务，再结合 `Task Type` 查看 AI Core 和 AI CPU 上的任务分布。

![op_summary CSV 文件示例](../figures/zh-cn_image_0000002502718556.png)
<div style="text-align: center;">
<strong>图2</strong> <code>op_summary_*.csv</code> 文件示例
</div>

> [!NOTE]说明
>
> 本教程仅介绍基础分析方法。如需了解上述输出文件及各字段的完整定义，请参见 《[性能数据文件参考](../user_guide/profile_data_file_references.md)》。

### 2.3 可视化性能分析（可选）

#### 2.3.1 环境准备

本节操作需在安装了 MindStudio Insight 的本地环境中进行，推荐使用 Windows 或 macOS 图形界面运行。

1.  **安装工具**：若尚未安装，请前往昇腾社区 [MindStudio 下载](https://www.hiascend.com/developer/software/mindstudio/download) 页面获取并安装。
2.  **熟悉操作**：如需了解基本使用方法，请参见《[MindStudio Insight 系统调优快速入门](https://gitcode.com/Ascend/msinsight/blob/master/docs/zh/quick_start/system_tuning_quick_start.md)》。

#### 2.3.2 加载 msProf 采集数据

1. 打开 MindStudio Insight。
2.  在容器内执行以下命令，获取并复制输出的变量赋值语句：
    ```bash
    echo "CONTAINER_ID=${HOSTNAME:-$(cat /etc/hostname)}; PROF_DIR=${PROF_DIR}"
    ```
3. 切换至宿主机终端，粘贴并执行上一步复制的变量赋值语句，然后使用 `docker cp` 将 Profiling 数据从容器拷贝至本地：

    ```bash
    docker cp "${CONTAINER_ID}:${PROF_DIR}" ./
    ```

4. 在 MindStudio Insight 中导入拷贝出的 `PROF_XXX` 目录。
5. 进入 Timeline（时间线）或 Operator（算子）视图查看分析结果。

#### 2.3.3 可视化查看示例

MindStudio Insight 的 Timeline 界面主要包含以下三个功能区域：

-   **区域一（CANN 层）**：展示 ACL、Runtime 等软件栈关键接口的执行耗时。
-   **区域二（NPU 设备层）**：展示 Ascend Hardware 各 Stream 的执行时序、迭代轨迹及处理器系统级数据。
-   **区域三（详情面板）**：展示选中对象的详细属性。单击 Timeline 中的任意色块，即可在此处查看对应算子或接口的完整信息。

![MindStudio Insight Timeline 界面](../figures/zh-cn_image_0000002502558722.png)
<div style="text-align: center;">
<strong>图3</strong> msProf 采集结果文件的可视化呈现
</div>

借助上述可视化视图，可高效完成以下性能分析任务：

-   **瓶颈定位**：快速识别耗时异常的 API、算子及任务流。
-   **下发关系分析**：通过 HostToDevice 连线，直观分析 Host 侧任务与 Device 侧任务的调度依赖与时序对齐情况。

更多高级操作与分析方法，请参见《[MindStudio Insight 系统调优指南](https://gitcode.com/Ascend/msinsight/blob/master/docs/zh/user_guide/system_tuning.md)》。

## 3. 内网环境无公网访问权限的应对方案

### 3.1 Docker 镜像在隔离内网的获取方法

**方案一：配置 Docker 代理直接拉取**

适用于大多数 Linux 发行版且 Docker 版本 ≥ 18.0 的环境（不保证所有场景兼容）。若遇异常，请结合实际情况调整。

编辑 Docker 服务代理配置文件 `/etc/systemd/system/docker.service.d/http-proxy.conf`，内容示例如下（请根据实际环境替换用户名、密码、代理地址及端口）：

```text
[Service]
Environment="HTTP_PROXY=http://username:password@proxy.example.com:8080"
Environment="HTTPS_PROXY=http://username:password@proxy.example.com:8080"
Environment="NO_PROXY=localhost,127.0.0.1,.example.com"
```

保存后重载并重启 Docker 服务：

```bash
sudo systemctl daemon-reload
sudo systemctl restart docker
```

随后即可正常执行 `docker pull`。

**方案二：离线导入 CANN 镜像**

如果代理方案不可行，请先在内网 NPU 服务器上执行 [第 2.1.2 节](#212-宿主机基于芯片型号自动识别并配置镜像环境变量)，记录 `MY_STUDY_VAR_CANN_IMAGE` 的完整值。然后在具备公网访问能力且 CPU 架构相同的中转机上执行以下命令，将第一行的值替换为刚才记录的镜像地址：

```bash
CANN_IMAGE='完整镜像地址'
docker pull "${CANN_IMAGE}"
docker save -o cann.tar "${CANN_IMAGE}"
```

将 `cann.tar` 通过 U 盘等方式传输至内网服务器后，在内网服务器执行以下命令加载：

```bash
docker load -i cann.tar
docker images | grep cann
```

加载完成后，继续完成 [第 3.2 节](#32-传输容器启动脚本)，再返回 [第 2.1.5 节](#215-宿主机启动容器)，然后启动容器。如果已切换宿主机 shell，请重新执行第 2.1.2 节中的命令以恢复镜像环境变量。

### 3.2 传输容器启动脚本

在可访问当前网页的浏览器中输入如下链接，下载 `ctr_in.py` 脚本文件，并将其手动拷贝至内网服务器的 `~/` 目录：

```text
https://inst.obs.cn-north-4.myhuaweicloud.com/env/ctr_in.py
```

拷贝完成后，在内网服务器的宿主机上执行：

```bash
cd ~
chmod +x ctr_in.py
ls -l ctr_in.py
```

确认 `ctr_in.py` 存在且具有执行权限后，返回 [第 2.1.5 节](#215-宿主机启动容器)，然后启动容器。

### 3.3 离线安装 Python 依赖

优先使用内网 pip 源安装依赖。若没有可用的内网软件源，请在具备公网访问能力、与内网 NPU 服务器 CPU 架构相同且使用 Python 3.11 的中转环境中，按以下方式下载所需安装包：

```bash
mkdir -p offline_wheels
python3 -m pip download xxx --dest offline_wheels
```

将 `offline_wheels` 目录传输到内网服务器并复制到容器的用户主目录，然后在容器内执行：

```bash
pip3 install --no-index --find-links="${HOME}/offline_wheels" xxx
```

安装完成后，返回 [第 2.1.7 节](#217-容器内环境检查)，然后执行验证命令，无需再次执行联网安装命令。
