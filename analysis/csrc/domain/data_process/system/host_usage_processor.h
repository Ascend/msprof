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
#ifndef ANALYSIS_DOMAIN_HOST_USAGE_PROCESSOR_H
#define ANALYSIS_DOMAIN_HOST_USAGE_PROCESSOR_H

#include "analysis/csrc/domain/data_process/data_processor.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/host_usage_data.h"

namespace Analysis
{
namespace Domain
{
using namespace Analysis::Utils;
// start_time, cpu_no, usage
using OriCpuUsage = std::vector<std::tuple<uint64_t, std::string, double>>;

// timestamp, cpu_no, freq
using OriCpuFreq = std::vector<std::tuple<uint64_t, std::string, double>>;

// start_time, usage
using OriMemUsage = std::vector<std::tuple<uint64_t, double>>;

// start_time, usage, disk_read, disk_write
using OriDiskUsage = std::vector<std::tuple<uint64_t, double, double, double>>;

// start_time, usage, speed
using OriNetworkUsage = std::vector<std::tuple<uint64_t, double, double>>;

// runtime_api_name, runtime_pid, runtime_tid, runtime_trans_start, runtime_trans_end
using OriSysCall = std::vector<std::tuple<std::string, uint64_t, uint64_t, uint64_t, uint64_t>>;

class HostUsageProcessor : public DataProcessor
{
   public:
    HostUsageProcessor() = default;
    explicit HostUsageProcessor(const std::string &profPath_, const std::string &processorName, DBInfo &&dbInfo);
    virtual ~HostUsageProcessor() = default;

   protected:
    DBInfo usageDb_;
    std::string processorName_;

   private:
    bool Process(DataInventory &dataInventory) override;
    virtual bool ProcessData(DataInventory &dataInventory, const ProfTimeRecord &record, const std::string &dbPath) = 0;
};

class HostCpuUsageProcessor : public HostUsageProcessor
{
   public:
    HostCpuUsageProcessor() = default;
    explicit HostCpuUsageProcessor(const std::string &profPath_);

   private:
    bool ProcessData(DataInventory &dataInventory, const ProfTimeRecord &record, const std::string &dbPath) override;
};

class HostCpuFreqProcessor : public HostUsageProcessor
{
   public:
    HostCpuFreqProcessor() = default;
    explicit HostCpuFreqProcessor(const std::string &profPath_);

   private:
    bool ProcessData(DataInventory &dataInventory, const ProfTimeRecord &record, const std::string &dbPath) override;
};

class HostMemUsageProcessor : public HostUsageProcessor
{
   public:
    HostMemUsageProcessor() = default;
    explicit HostMemUsageProcessor(const std::string &profPath_);

   private:
    bool ProcessData(DataInventory &dataInventory, const ProfTimeRecord &record, const std::string &dbPath) override;
};

class HostDiskUsageProcessor : public HostUsageProcessor
{
   public:
    HostDiskUsageProcessor() = default;
    explicit HostDiskUsageProcessor(const std::string &profPath_);

   private:
    bool ProcessData(DataInventory &dataInventory, const ProfTimeRecord &record, const std::string &dbPath) override;
};

class HostNetworkUsageProcessor : public HostUsageProcessor
{
   public:
    HostNetworkUsageProcessor() = default;
    explicit HostNetworkUsageProcessor(const std::string &profPath_);

   private:
    bool ProcessData(DataInventory &dataInventory, const ProfTimeRecord &record, const std::string &dbPath) override;
};

class OSRuntimeApiProcessor : public HostUsageProcessor
{
   public:
    OSRuntimeApiProcessor() = default;
    explicit OSRuntimeApiProcessor(const std::string &profPath_);

   private:
    bool ProcessData(DataInventory &dataInventory, const ProfTimeRecord &record, const std::string &dbPath) override;
};
}  // namespace Domain
}  // namespace Analysis

#endif  // ANALYSIS_DOMAIN_HOST_USAGE_PROCESSOR_H
