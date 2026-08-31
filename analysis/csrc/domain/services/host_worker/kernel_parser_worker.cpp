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

#include "kernel_parser_worker.h"

#include <utility>

#include "analysis/csrc/domain/services/environment/context.h"
#include "analysis/csrc/domain/services/modeling/npu_op_mem_calculator.h"
#include "analysis/csrc/domain/services/parser/host/cann/addition_info_parser.h"
#include "analysis/csrc/domain/services/parser/host/cann/hash_data.h"
#include "analysis/csrc/domain/services/parser/host/cann/rt_add_info_center.h"
#include "analysis/csrc/domain/services/parser/host/cann/type_data.h"
#include "analysis/csrc/domain/services/persistence/host/hash_db_dumper.h"
#include "analysis/csrc/domain/services/persistence/host/npu_op_mem_db_dumper.h"
#include "analysis/csrc/domain/services/persistence/host/type_info_db_dumper.h"
#include "host_trace_worker.h"

namespace Analysis
{
namespace Domain
{
using HostTraceWorker = Analysis::Domain::HostTraceWorker;
using ThreadPool = Analysis::Utils::ThreadPool;
using namespace Analysis::Domain::Environment;
using namespace Analysis::Domain::Host::Cann;
// 解析流程控制类，传入host data所在路径，启动采集流程并返回结果
KernelParserWorker::KernelParserWorker(std::string hostFilePath) : hostFilePath_(std::move(hostFilePath)), result_(true)
{
}

int KernelParserWorker::Run()
{
    std::set<std::string> profPaths{Utils::File::PathJoin({hostFilePath_, ".."})};
    if (!Context::GetInstance().Load(profPaths))
    {
        ERROR("Context load failed.");
        return ANALYSIS_ERROR;
    }
    INFO("Start run KernelParserWorker");
    // 先创建目录
    std::string sqlBaseDir = Utils::File::PathJoin({hostFilePath_, "sqlite"});
    if (!Utils::File::CreateDir(sqlBaseDir))
    {
        ERROR("Create path failed");
        return ANALYSIS_ERROR;
    }
    // 启动线程
    const uint16_t taskNumber = 3;
    ThreadPool pool(taskNumber);
    pool.Start();
    pool.AddTask(
        [this]()
        {
            INFO("Start parse hash data");
            DumpHashData();
        });
    pool.AddTask(
        [this]()
        {
            INFO("Start parse type info data");
            DumpTypeInfoData();
        });
    pool.WaitAllTasks();
    pool.Stop();
    // ProcessNpuOpMemData(); 先注释掉C++入口，代码不生效，依旧使用python业务代码，后续统一调整上库
    LaunchTraceParser();
    if (!result_)
    {
        ERROR("Parse failed or dump failed");
        return ANALYSIS_ERROR;
    }
    return ANALYSIS_OK;
}

void KernelParserWorker::DumpHashData()
{
    auto dataPath = Utils::File::PathJoin({hostFilePath_, "data"});
    INFO("Hash data load from path: %", dataPath);
    HashData::GetInstance().Load(dataPath);
    INFO("Start load runtime op info data");
    RTAddInfoCenter::GetInstance().Load(Utils::File::PathJoin({hostFilePath_, "sqlite"}));
    auto hashDataContent = HashData::GetInstance().GetAll();
    INFO("success get hash data");
    if (hashDataContent.empty())
    {
        WARN("Empty hash data");
        return;
    }
    std::shared_ptr<HashDBDumper> hashDbDumper;
    MAKE_SHARED_RETURN_VOID(hashDbDumper, HashDBDumper, hostFilePath_);
    INFO("success get hashDbDumper");
    if (!hashDbDumper->DumpData(hashDataContent))
    {
        ERROR("Hash data parse failed");
        result_ = false;
    }
}

void KernelParserWorker::DumpTypeInfoData()
{
    auto dataPath = Utils::File::PathJoin({hostFilePath_, "data"});
    INFO("Typeinfo data load from path: %", dataPath);
    TypeData::GetInstance().Load(dataPath);
    auto typeInfoContent = TypeData::GetInstance().GetAll();
    if (typeInfoContent.empty())
    {
        WARN("Empty type info data");
        return;
    }
    TypeInfoDBDumper typeDbDumper(hostFilePath_);
    auto dumpresult = typeDbDumper.DumpData(typeInfoContent);
    if (!dumpresult)
    {
        ERROR("Type info parse failed");
        result_ = false;
    }
}

void KernelParserWorker::ProcessNpuOpMemData()
{
    const auto dataPath = Utils::File::PathJoin({hostFilePath_, "data"});
    Host::Cann::TaskMemoryParser parser(dataPath);
    auto rawData = parser.ParseData<ParserAdditionalInfo>();
    if (parser.GetStatus() == ParserStatus::NOT_EXIST)
    {
        INFO("Npu op memory data does not exist");
        return;
    }
    if (parser.GetStatus() == ParserStatus::ERROR)
    {
        ERROR("Npu op memory data parse failed");
        result_ = false;
        return;
    }
    if (rawData.empty())
    {
        WARN("Empty npu op memory data");
        return;
    }

    Host::NpuOpMemCalculationResult calculationResult;
    Host::NpuOpMemCalculator calculator;
    if (!calculator.Calculate(rawData, HashData::GetInstance().GetAll(), calculationResult))
    {
        ERROR("Npu op memory data calculate failed");
        result_ = false;
        return;
    }

    NpuOpMemRawDBDumper rawDumper(hostFilePath_);
    if (!rawDumper.DumpData(rawData))
    {
        ERROR("Npu op memory raw data dump failed");
        result_ = false;
        return;
    }
    if (calculationResult.memoryRecords.empty() || calculationResult.operatorMemory.empty())
    {
        return;
    }

    NpuOpMemRecordDBDumper recordDumper(hostFilePath_);
    NpuOpMemLifecycleDBDumper lifecycleDumper(hostFilePath_);
    if (!recordDumper.DumpData(calculationResult.memoryRecords) ||
        !lifecycleDumper.DumpData(calculationResult.operatorMemory))
    {
        ERROR("Npu op memory calculated data dump failed");
        result_ = false;
    }
}

void KernelParserWorker::LaunchTraceParser()
{
    HostTraceWorker hostTraceWorker{hostFilePath_};
    if (!hostTraceWorker.Run())
    {
        ERROR("Host trace parse failed");
        result_ = false;
    }
}
}  // namespace Domain
}  // namespace Analysis
