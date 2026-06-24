/* -------------------------------------------------------------------------
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This file is part of the MindStudio project.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *    http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------*/
#ifndef ANALYSIS_APPLICATION_EXPORT_JSON_ENUM_H
#define ANALYSIS_APPLICATION_EXPORT_JSON_ENUM_H

namespace Analysis
{
namespace Application
{
enum class JsonProcess
{
    ASCEND,
    ACC_PMU,
    CANN,
    DDR,
    STARS_CHIP_TRANS,
    HBM,
    COMMUNICATION,
    CCU,
    HCCS,
    OS_RUNTIME_API,
    NETWORK_USAGE,
    DISK_USAGE,
    MEMORY_USAGE,
    CPU_USAGE,
    MSPROFTX,
    NPU_MEM,
    OVERLAP_ANALYSE,
    PCIE,
    SIO,
    STARS_SOC,
    STEP_TRACE,
    FREQ,
    LLC,
    NIC,
    ROCE,
    QOS,
    DEVICE_TX,
    LOW_POWER,
    BIU_PERF,
    UB,
    BLOCK_DETAIL,
    DPU,
    FUSION_TASK,
};

const std::unordered_map<std::string, JsonProcess> strToJsonProcess = {
    {"ascend", JsonProcess::ASCEND},
    {"acc_pmu", JsonProcess::ACC_PMU},
    {"cann", JsonProcess::CANN},
    {"ddr", JsonProcess::DDR},
    {"stars_chip_trans", JsonProcess::STARS_CHIP_TRANS},
    {"hbm", JsonProcess::HBM},
    {"communication", JsonProcess::COMMUNICATION},
    {"ccu", JsonProcess::CCU},
    {"hccs", JsonProcess::HCCS},
    {"os_runtime_api", JsonProcess::OS_RUNTIME_API},
    {"network_usage", JsonProcess::NETWORK_USAGE},
    {"disk_usage", JsonProcess::DISK_USAGE},
    {"memory_usage", JsonProcess::MEMORY_USAGE},
    {"cpu_usage", JsonProcess::CPU_USAGE},
    {"msproftx", JsonProcess::MSPROFTX},
    {"npu_mem", JsonProcess::NPU_MEM},
    {"overlap_analyse", JsonProcess::OVERLAP_ANALYSE},
    {"pcie", JsonProcess::PCIE},
    {"sio", JsonProcess::SIO},
    {"stars_soc", JsonProcess::STARS_SOC},
    {"step_trace", JsonProcess::STEP_TRACE},
    {"freq", JsonProcess::FREQ},
    {"llc", JsonProcess::LLC},
    {"nic", JsonProcess::NIC},
    {"roce", JsonProcess::ROCE},
    {"qos", JsonProcess::QOS},
    {"device_tx", JsonProcess::DEVICE_TX},
    {"biu_perf", JsonProcess::BIU_PERF},
    {"ub", JsonProcess::UB},
    {"block_detail", JsonProcess::BLOCK_DETAIL},
    {"dpu", JsonProcess::DPU},
    {"fusion_task", JsonProcess::FUSION_TASK},
};

const std::vector<JsonProcess> allProcesses{JsonProcess::ASCEND,
                                            JsonProcess::ACC_PMU,
                                            JsonProcess::CANN,
                                            JsonProcess::DDR,
                                            JsonProcess::STARS_CHIP_TRANS,
                                            JsonProcess::HBM,
                                            JsonProcess::COMMUNICATION,
                                            JsonProcess::HCCS,
                                            JsonProcess::OS_RUNTIME_API,
                                            JsonProcess::NETWORK_USAGE,
                                            JsonProcess::DISK_USAGE,
                                            JsonProcess::MEMORY_USAGE,
                                            JsonProcess::CPU_USAGE,
                                            JsonProcess::MSPROFTX,
                                            JsonProcess::NPU_MEM,
                                            JsonProcess::OVERLAP_ANALYSE,
                                            JsonProcess::PCIE,
                                            JsonProcess::SIO,
                                            JsonProcess::STARS_SOC,
                                            JsonProcess::STEP_TRACE,
                                            JsonProcess::FREQ,
                                            JsonProcess::LLC,
                                            JsonProcess::NIC,
                                            JsonProcess::ROCE,
                                            JsonProcess::QOS,
                                            JsonProcess::DEVICE_TX,
                                            JsonProcess::LOW_POWER,
                                            JsonProcess::BIU_PERF,
                                            JsonProcess::UB,
                                            JsonProcess::BLOCK_DETAIL,
                                            JsonProcess::CCU,
                                            JsonProcess::DPU,
                                            JsonProcess::FUSION_TASK};
}  // namespace Application
}  // namespace Analysis

#endif  // ANALYSIS_APPLICATION_EXPORT_JSON_ENUM_H
