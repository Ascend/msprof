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

#ifndef ANALYSIS_VIEWER_DATABASE_DATABASE_H
#define ANALYSIS_VIEWER_DATABASE_DATABASE_H

#include <string>
#include <unordered_map>
#include <vector>

#include "analysis/csrc/infrastructure/db/include/connection.h"

namespace Analysis
{
namespace Infra
{

using TableColumns = std::vector<TableColumn>;
const std::string SQL_TEXT_TYPE = "TEXT";
const std::string SQL_INTEGER_TYPE = "INTEGER";
const std::string SQL_NUMERIC_TYPE = "NUMERIC";
const std::string SQL_REAL_TYPE = "REAL";

// DB中Table映射基类
class Database
{
   public:
    Database() = default;
    // 获取该DB实际落盘的文件名
    std::string GetDBName() const;
    // 获取该DB指定表中字段名
    TableColumns GetTableCols(const std::string &tableName);

   protected:
    std::string dbName_;
    std::unordered_map<std::string, TableColumns> tableColNames_;
};

class ApiEventDB : public Database
{
   public:
    ApiEventDB();
};

class RuntimeDB : public Database
{
   public:
    RuntimeDB();
};

class GEInfoDB : public Database
{
   public:
    GEInfoDB();
};

class StreamExpandSpecDB : public Database
{
   public:
    StreamExpandSpecDB();
};

class StreamInfoDB : public Database
{
   public:
    StreamInfoDB();
};

class HashDB : public Database
{
   public:
    HashDB();
};

class GeModelInfoDB : public Database
{
   public:
    GeModelInfoDB();
};

class HCCLDB : public Database
{
   public:
    HCCLDB();
};

class RtsTrackDB : public Database
{
   public:
    RtsTrackDB();
};

class AscendTaskDB : public Database
{
   public:
    AscendTaskDB();
};

class TraceDB : public Database
{
   public:
    TraceDB();
};

class HCCLSingleDeviceDB : public Database
{
   public:
    HCCLSingleDeviceDB();
};

class NpuMemDB : public Database
{
   public:
    NpuMemDB();
};

class NpuModuleMemDB : public Database
{
   public:
    NpuModuleMemDB();
};

class TaskMemoryDB : public Database
{
   public:
    TaskMemoryDB();
};

class NicDB : public Database
{
   public:
    NicDB();
};

class NicReceiveSendDB : public Database
{
   public:
    NicReceiveSendDB();
};

class RoceDB : public Database
{
   public:
    RoceDB();
};

class RoceReceiveSendDB : public Database
{
   public:
    RoceReceiveSendDB();
};

class HBMDB : public Database
{
   public:
    HBMDB();
};

class DDRDB : public Database
{
   public:
    DDRDB();
};

class LLCDB : public Database
{
   public:
    LLCDB();
};

class AicoreDB : public Database
{
   public:
    AicoreDB();
};

class AiVectorCoreDB : public Database
{
   public:
    AiVectorCoreDB();
};

class AccPmuDB : public Database
{
   public:
    AccPmuDB();
};

class SocProfilerDB : public Database
{
   public:
    SocProfilerDB();
};

class PCIeDB : public Database
{
   public:
    PCIeDB();
};

class HCCSDB : public Database
{
   public:
    HCCSDB();
};

class NetDevStatsDB : public Database
{
   public:
    NetDevStatsDB();
};

class FreqDB : public Database
{
   public:
    FreqDB();
};

class MsprofTxDB : public Database
{
   public:
    MsprofTxDB();
};

class StepTraceDB : public Database
{
   public:
    StepTraceDB();
};

class KfcInfo : public Database
{
   public:
    KfcInfo();
};

class Mc2CommInfoDB : public Database
{
   public:
    Mc2CommInfoDB();
};

class HostCpuUsage : public Database
{
   public:
    HostCpuUsage();
};

class HostMemUsage : public Database
{
   public:
    HostMemUsage();
};

class HostDiskUsage : public Database
{
   public:
    HostDiskUsage();
};

class HostNetworkUsage : public Database
{
   public:
    HostNetworkUsage();
};

class HostRuntimeApi : public Database
{
   public:
    HostRuntimeApi();
};

class ChipTransDB : public Database
{
   public:
    ChipTransDB();
};

class GeLogicStreamDB : public Database
{
   public:
    GeLogicStreamDB();
};

class SioDB : public Database
{
   public:
    SioDB();
};

class QosDB : public Database
{
   public:
    QosDB();
};

class OpCounterDB : public Database
{
   public:
    OpCounterDB();
};

class MetricSummaryDB : public Database
{
   public:
    MetricSummaryDB();
};

class DPUDB : public Database
{
   public:
    DPUDB();
};

class LowPowerDB : public Database
{
   public:
    LowPowerDB();
};

class UbDB : public Database
{
   public:
    UbDB();
};

class BiuPerfDB : public Database
{
   public:
    BiuPerfDB();
};

}  // namespace Infra
}  // namespace Analysis
#endif  // ANALYSIS_VIEWER_DATABASE_DATABASE_H
