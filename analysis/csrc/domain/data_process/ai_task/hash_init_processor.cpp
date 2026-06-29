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

#include "analysis/csrc/domain/data_process/ai_task/hash_init_processor.h"

namespace Analysis
{
namespace Domain
{
using namespace Analysis::Application;
using GeHashFormat = std::vector<std::tuple<std::string, std::string>>;
using LogicStream = std::vector<std::tuple<uint32_t, uint32_t>>;
HashInitProcessor::HashInitProcessor(const std::string &profPath) : DataProcessor(profPath) {}

bool HashInitProcessor::ProcessLogicStream(DataInventory &dataInventory)
{
    DBInfo logicInfo("ge_logic_stream_info.db", "GeLogicStreamInfo");
    LogicStream logicStream;
    std::string dbPath = Utils::File::PathJoin({profPath_, HOST, SQLITE, logicInfo.dbName});
    if (!logicInfo.ConstructDBRunner(dbPath))
    {
        return false;
    }
    StreamMap streamMap;
    auto flag = CheckPathAndTable(dbPath, logicInfo);
    if (flag != CHECK_SUCCESS)
    {
        return flag != CHECK_FAILED;
    }
    std::string sql = "SELECT physic_stream, logic_stream FROM " + logicInfo.tableName;
    if (!logicInfo.dbRunner->QueryData(sql, logicStream))
    {
        ERROR("Query logic stream data failed, db path is %", dbPath);
        return false;
    }
    for (auto &item : logicStream)
    {
        if (streamMap.find(std::get<0>(item)) == streamMap.end())
        {
            streamMap[std::get<0>(item)] = std::get<1>(item);
        }
        else
        {
            WARN("logic_stream is reporting duplicate data, please check the accuracy of the information.");
        }
    }
    std::shared_ptr<StreamMap> res;
    MAKE_SHARED_RETURN_VALUE(res, StreamMap, false, std::move(streamMap));
    dataInventory.Inject(res);
    return true;
}

bool HashInitProcessor::ProcessHashMap(DataInventory &dataInventory)
{
    INFO("Init Hash data, dir is %", profPath_);
    DBInfo hashDB("ge_hash.db", "GeHashInfo");
    GeHashFormat hashData;
    std::string dbPath = Utils::File::PathJoin({profPath_, HOST, SQLITE, hashDB.dbName});
    if (!hashDB.ConstructDBRunner(dbPath))
    {
        return false;
    }
    GeHashMap hashMap;
    std::shared_ptr<GeHashMap> res;
    // 并不是所有场景都有ge hash数据
    auto flag = CheckPathAndTable(dbPath, hashDB);
    if (flag != CHECK_SUCCESS)
    {
        MAKE_SHARED_RETURN_VALUE(res, GeHashMap, false, std::move(hashMap));
        dataInventory.Inject(res);
        return flag != CHECK_FAILED;
    }
    std::string sql = "SELECT hash_key, hash_value FROM " + hashDB.tableName;
    if (!hashDB.dbRunner->QueryData(sql, hashData))
    {
        ERROR("Query hash data failed, db path is %.", dbPath);
        return false;
    }
    for (auto &item : hashData)
    {
        hashMap[std::get<0>(item)] = std::get<1>(item);
    }
    MAKE_SHARED_RETURN_VALUE(res, GeHashMap, false, std::move(hashMap));
    dataInventory.Inject(res);
    return true;
}

bool HashInitProcessor::Process(DataInventory &dataInventory)
{
    bool res = true;
    res = ProcessLogicStream(dataInventory) && res;
    return ProcessHashMap(dataInventory) && res;
}
}  // namespace Domain
}  // namespace Analysis
