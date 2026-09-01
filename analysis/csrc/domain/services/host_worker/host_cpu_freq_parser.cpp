#include "analysis/csrc/domain/services/host_worker/host_cpu_freq_parser.h"

#include <algorithm>
#include <sstream>
#include <tuple>
#include <vector>

#include "analysis/csrc/infrastructure/db/include/database.h"
#include "analysis/csrc/infrastructure/db/include/db_runner.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/utils/common_constant.h"
#include "analysis/csrc/infrastructure/utils/file.h"
#include "analysis/csrc/infrastructure/utils/utils.h"

namespace Analysis
{
namespace Domain
{
namespace
{
const std::string HOST_CPU_FREQ_PREFIX = "host_cpu_freq.data.slice_";

bool CompareSliceId(const std::string& lhs, const std::string& rhs)
{
    auto lhsBase = Utils::File::BaseName(lhs);
    auto rhsBase = Utils::File::BaseName(rhs);
    auto lhsPos = lhsBase.find_last_of('_');
    auto rhsPos = rhsBase.find_last_of('_');
    if (lhsPos == std::string::npos || rhsPos == std::string::npos)
    {
        return lhs < rhs;
    }
    uint32_t lhsId = 0;
    uint32_t rhsId = 0;
    if (Utils::StrToU32(lhsId, lhsBase.substr(lhsPos + 1)) != ANALYSIS_OK ||
        Utils::StrToU32(rhsId, rhsBase.substr(rhsPos + 1)) != ANALYSIS_OK)
    {
        return lhs < rhs;
    }
    return lhsId < rhsId;
}
}  // namespace

uint32_t HostCpuFreqParser::Run() const
{
    auto dataPath = Utils::File::PathJoin({hostPath_, "data"});
    auto sqlitePath = Utils::File::PathJoin({hostPath_, Analysis::Common::SQLITE});
    if (!Utils::File::CreateDir(sqlitePath))
    {
        ERROR("Create host sqlite dir failed, path is %.", sqlitePath);
        return ANALYSIS_ERROR;
    }

    auto files = Utils::File::GetOriginData(dataPath, {HOST_CPU_FREQ_PREFIX}, {"done", "complete"});
    if (files.empty())
    {
        INFO("No host cpu freq files found under %.", dataPath);
        return ANALYSIS_OK;
    }
    std::sort(files.begin(), files.end(), CompareSliceId);

    Infra::HostCpuFreq dbInfo;
    auto dbPath = Utils::File::PathJoin({sqlitePath, dbInfo.GetDBName()});
    Infra::DBRunner dbRunner(dbPath);
    const std::string tableName = "CpuFreq";
    if (!dbRunner.CreateTable(tableName, dbInfo.GetTableCols(tableName)))
    {
        ERROR("Create host cpu freq table failed.");
        return ANALYSIS_ERROR;
    }

    std::vector<std::tuple<uint64_t, std::string, double>> rows;
    uint64_t currentTimestamp = 0;
    bool hasTimestamp = false;
    for (const auto& file : files)
    {
        Utils::FileReader reader(file);
        if (!reader.IsOpen())
        {
            ERROR("Open host cpu freq file failed, file is %.", file);
            return ANALYSIS_ERROR;
        }
        std::vector<std::string> lines;
        if (reader.ReadText(lines) != ANALYSIS_OK)
        {
            ERROR("Read host cpu freq file failed, file is %.", file);
            return ANALYSIS_ERROR;
        }
        for (const auto& line : lines)
        {
            std::stringstream ss(line);
            std::string token;
            if (!(ss >> token))
            {
                continue;
            }
            if (token == "time")
            {
                std::string timestampStr;
                if (ss >> timestampStr && Utils::StrToU64(currentTimestamp, timestampStr) == ANALYSIS_OK)
                {
                    hasTimestamp = true;
                }
                continue;
            }
            if (!hasTimestamp)
            {
                continue;
            }
            double freq = 0;
            if (!(ss >> freq))
            {
                continue;
            }
            rows.emplace_back(currentTimestamp, token, freq);
        }
    }

    if (rows.empty())
    {
        WARN("Host cpu freq parsed data is empty.");
        return ANALYSIS_OK;
    }
    if (!dbRunner.InsertData(tableName, rows))
    {
        ERROR("Insert host cpu freq data failed.");
        return ANALYSIS_ERROR;
    }
    INFO("Host cpu freq parser saved % rows.", rows.size());
    return ANALYSIS_OK;
}
}  // namespace Domain
}  // namespace Analysis
