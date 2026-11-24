/*
 * -------------------------------------------------------------------------
 * This file is part of the MindStudio project.
 * Copyright (c) 2026 Huawei Technologies Co.,Ltd.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *          http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------
 */

#include <cmath>
#include <fstream>
#include <string>
#include <vector>

#include "analysis/csrc/domain/services/parser/host_pmu/include/host_pmu_counter_parser.h"
#include "analysis/csrc/infrastructure/utils/file.h"
#include "gtest/gtest.h"

namespace Analysis
{
namespace Domain
{
using namespace Utils;

namespace
{
const std::string TEST_ROOT = "./host_pmu_counter_parser_utest";
const std::string DATA_FILE = "host_pmu_123.data";
const std::string HEADER = "#msprof_pmu_v1";
const std::string COLUMNS = "timestamp,cycles/sysfs,instructions/generic";
const std::string ROW = "1500,100,200";
// Info line as the current collector writes it: every field present.
const std::string FULL_INFO = "#pid=123 cpu=0 intervalNs=100000000 anchorMonoNs=1000 anchorEpochNs=2000";
// Info line as an older collector wrote it: no clock anchor at all.
const std::string LEGACY_INFO = "#pid=123 cpu=0 intervalNs=100000000";

std::string BuildFile(const std::string &header, const std::string &info, const std::string &columns,
                      const std::string &rows)
{
    return header + "\n" + info + "\n" + columns + "\n" + rows + "\n";
}
}  // namespace

class HostPmuCounterParserUtest : public testing::Test
{
   protected:
    void SetUp() override
    {
        if (File::Exist(TEST_ROOT))
        {
            ASSERT_TRUE(File::RemoveDir(TEST_ROOT, 0));
        }
        ASSERT_TRUE(File::CreateDir(TEST_ROOT));
    }

    void TearDown() override
    {
        ASSERT_TRUE(File::RemoveDir(TEST_ROOT, 0));
    }

    std::string WriteFile(const std::string &name, const std::string &content)
    {
        const std::string path = File::PathJoin({TEST_ROOT, name});
        std::ofstream stream(path, std::ios::out | std::ios::trunc | std::ios::binary);
        stream << content;
        stream.close();
        return path;
    }

    std::string WriteData(const std::string &info, const std::string &columns, const std::string &rows)
    {
        return WriteFile(DATA_FILE, BuildFile(HEADER, info, columns, rows));
    }

    static bool Parse(const std::string &path, PmuCounterBundle &bundle, std::string &errorMsg)
    {
        return HostPmuCounterParser::ParseFile(path, bundle, errorMsg);
    }
};

// ---------------------------------------------------------------------------
// header
// ---------------------------------------------------------------------------

TEST_F(HostPmuCounterParserUtest, RejectsVersionWithPrefixButDifferentSuffix)
{
    // "#msprof_pmu_v10" shares the "#msprof_pmu_v1" prefix but describes a different column layout,
    // so the header check has to be an exact match.
    const std::string path = WriteFile(DATA_FILE, BuildFile("#msprof_pmu_v10", FULL_INFO, COLUMNS, ROW));
    PmuCounterBundle bundle;
    std::string errorMsg;
    EXPECT_FALSE(Parse(path, bundle, errorMsg));
    EXPECT_NE(std::string::npos, errorMsg.find("unsupported header"));
    EXPECT_TRUE(bundle.counters.empty());
}

TEST_F(HostPmuCounterParserUtest, RejectsFileWithoutHeader)
{
    // The first line is the info line, so there is no header at all.
    const std::string path = WriteFile(DATA_FILE, BuildFile(FULL_INFO, COLUMNS, ROW, ""));
    PmuCounterBundle bundle;
    std::string errorMsg;
    EXPECT_FALSE(Parse(path, bundle, errorMsg));
    EXPECT_NE(std::string::npos, errorMsg.find("unsupported header"));
}

TEST_F(HostPmuCounterParserUtest, RejectsEmptyFile)
{
    const std::string path = WriteFile(DATA_FILE, "");
    PmuCounterBundle bundle;
    std::string errorMsg;
    EXPECT_FALSE(Parse(path, bundle, errorMsg));
    EXPECT_NE(std::string::npos, errorMsg.find("empty file"));
}

// ---------------------------------------------------------------------------
// info line
// ---------------------------------------------------------------------------

TEST_F(HostPmuCounterParserUtest, RejectsMissingInfoLine)
{
    const std::string path = WriteFile(DATA_FILE, HEADER + "\n");
    PmuCounterBundle bundle;
    std::string errorMsg;
    EXPECT_FALSE(Parse(path, bundle, errorMsg));
    EXPECT_NE(std::string::npos, errorMsg.find("missing info line"));
}

TEST_F(HostPmuCounterParserUtest, RejectsInfoLineWithoutPid)
{
    const std::string path = WriteData("cpu=0 intervalNs=100000000", COLUMNS, ROW);
    PmuCounterBundle bundle;
    std::string errorMsg;
    EXPECT_FALSE(Parse(path, bundle, errorMsg));
    EXPECT_NE(std::string::npos, errorMsg.find("malformed info line"));
}

TEST_F(HostPmuCounterParserUtest, RejectsInfoLineWithoutCpu)
{
    const std::string path = WriteData("#pid=123 intervalNs=100000000", COLUMNS, ROW);
    PmuCounterBundle bundle;
    std::string errorMsg;
    EXPECT_FALSE(Parse(path, bundle, errorMsg));
    EXPECT_NE(std::string::npos, errorMsg.find("malformed info line"));
}

TEST_F(HostPmuCounterParserUtest, RejectsInfoLineWithoutInterval)
{
    const std::string path = WriteData("#pid=123 cpu=0", COLUMNS, ROW);
    PmuCounterBundle bundle;
    std::string errorMsg;
    EXPECT_FALSE(Parse(path, bundle, errorMsg));
    EXPECT_NE(std::string::npos, errorMsg.find("malformed info line"));
}

TEST_F(HostPmuCounterParserUtest, AcceptsPidMinusOneAsAllProcesses)
{
    // -1 is the "all processes" sentinel, not a parse failure.
    const std::string path = WriteData("#pid=-1 cpu=0 intervalNs=100000000", COLUMNS, ROW);
    PmuCounterBundle bundle;
    std::string errorMsg;
    EXPECT_TRUE(Parse(path, bundle, errorMsg)) << errorMsg;
    EXPECT_EQ(-1, bundle.pid);
    EXPECT_EQ(0, bundle.cpu);
}

TEST_F(HostPmuCounterParserUtest, AcceptsCpuMinusOneAsAnyCpu)
{
    const std::string path = WriteData("#pid=123 cpu=-1 intervalNs=100000000", COLUMNS, ROW);
    PmuCounterBundle bundle;
    std::string errorMsg;
    EXPECT_TRUE(Parse(path, bundle, errorMsg)) << errorMsg;
    EXPECT_EQ(-1, bundle.cpu);
}

TEST_F(HostPmuCounterParserUtest, RejectsMalformedPidInsteadOfTreatingItAsAllProcesses)
{
    const std::string path = WriteData("#pid=abc cpu=0 intervalNs=100000000", COLUMNS, ROW);
    PmuCounterBundle bundle;
    std::string errorMsg;
    EXPECT_FALSE(Parse(path, bundle, errorMsg));
    EXPECT_NE(std::string::npos, errorMsg.find("malformed pid/cpu"));
}

TEST_F(HostPmuCounterParserUtest, RejectsMalformedInterval)
{
    const std::string path = WriteData("#pid=123 cpu=0 intervalNs=abc", COLUMNS, ROW);
    PmuCounterBundle bundle;
    std::string errorMsg;
    EXPECT_FALSE(Parse(path, bundle, errorMsg));
    EXPECT_NE(std::string::npos, errorMsg.find("malformed interval"));
}

TEST_F(HostPmuCounterParserUtest, RejectsMonoAnchorWithoutEpochAnchor)
{
    const std::string path = WriteData("#pid=123 cpu=0 intervalNs=100000000 anchorMonoNs=1000", COLUMNS, ROW);
    PmuCounterBundle bundle;
    std::string errorMsg;
    EXPECT_FALSE(Parse(path, bundle, errorMsg));
    EXPECT_NE(std::string::npos, errorMsg.find("missing epoch anchor"));
}

// ---------------------------------------------------------------------------
// column header
// ---------------------------------------------------------------------------

TEST_F(HostPmuCounterParserUtest, RejectsFileWithoutColumnHeader)
{
    const std::string path = WriteFile(DATA_FILE, HEADER + "\n" + FULL_INFO + "\n");
    PmuCounterBundle bundle;
    std::string errorMsg;
    EXPECT_FALSE(Parse(path, bundle, errorMsg));
    EXPECT_NE(std::string::npos, errorMsg.find("missing column header"));
}

TEST_F(HostPmuCounterParserUtest, RejectsColumnHeaderWithoutCounters)
{
    const std::string path = WriteData(FULL_INFO, "timestamp", "1500");
    PmuCounterBundle bundle;
    std::string errorMsg;
    EXPECT_FALSE(Parse(path, bundle, errorMsg));
    EXPECT_NE(std::string::npos, errorMsg.find("column header has no counters"));
}

TEST_F(HostPmuCounterParserUtest, RejectsRowWithFewerColumnsThanHeader)
{
    // A short row means this reader and the writer disagree, so the whole file is rejected.
    const std::string path = WriteData(FULL_INFO, COLUMNS, "1500,100");
    PmuCounterBundle bundle;
    std::string errorMsg;
    EXPECT_FALSE(Parse(path, bundle, errorMsg));
    EXPECT_NE(std::string::npos, errorMsg.find("expected 3 columns, got 2"));
}

TEST_F(HostPmuCounterParserUtest, RejectsUnparsableTimestamp)
{
    const std::string path = WriteData(FULL_INFO, COLUMNS, "abc,100,200");
    PmuCounterBundle bundle;
    std::string errorMsg;
    EXPECT_FALSE(Parse(path, bundle, errorMsg));
    EXPECT_NE(std::string::npos, errorMsg.find("bad timestamp"));
}

TEST_F(HostPmuCounterParserUtest, RejectsUnparsableValue)
{
    const std::string path = WriteData(FULL_INFO, COLUMNS, "1500,abc,200");
    PmuCounterBundle bundle;
    std::string errorMsg;
    EXPECT_FALSE(Parse(path, bundle, errorMsg));
    EXPECT_NE(std::string::npos, errorMsg.find("bad value"));
}

// ---------------------------------------------------------------------------
// counters, values and the timing sidecar
// ---------------------------------------------------------------------------

TEST_F(HostPmuCounterParserUtest, MapsColumnSourceAndUnit)
{
    const std::string path =
        WriteData(FULL_INFO, "timestamp,cycles/sysfs,task-clock/software,page-faults/software,bare", "1500,1,2,3,4");
    PmuCounterBundle bundle;
    std::string errorMsg;
    ASSERT_TRUE(Parse(path, bundle, errorMsg)) << errorMsg;
    ASSERT_EQ(4UL, bundle.counters.size());
    EXPECT_EQ("cycles", bundle.counters[0].name);
    EXPECT_EQ(PmuCounterSource::SYSFS, bundle.counters[0].source);
    EXPECT_EQ(UNIT_COUNT, bundle.counters[0].unit);
    // A time-based kernel counter carries nanoseconds, an event-counting one a plain count.
    EXPECT_EQ(PmuCounterSource::SOFTWARE, bundle.counters[1].source);
    EXPECT_EQ(UNIT_NS, bundle.counters[1].unit);
    EXPECT_EQ(UNIT_COUNT, bundle.counters[2].unit);
    // A bare name comes from an older collector: the source is unknown, not an error.
    EXPECT_EQ(PmuCounterSource::UNKNOWN, bundle.counters[3].source);
    EXPECT_EQ(UNIT_UNKNOWN, bundle.counters[3].unit);
}

TEST_F(HostPmuCounterParserUtest, KeepsNanForUnavailableCounter)
{
    // "nan" means the counter could not be read in this sample and must not become 0.
    const std::string path = WriteData(FULL_INFO, COLUMNS, "1500,100,nan");
    PmuCounterBundle bundle;
    std::string errorMsg;
    ASSERT_TRUE(Parse(path, bundle, errorMsg)) << errorMsg;
    ASSERT_EQ(1UL, bundle.samples.size());
    ASSERT_EQ(2UL, bundle.samples[0].values.size());
    EXPECT_DOUBLE_EQ(100.0, bundle.samples[0].values[0]);
    EXPECT_TRUE(std::isnan(bundle.samples[0].values[1]));
}

TEST_F(HostPmuCounterParserUtest, ParsesLegacyFileWithoutAnchorOrTimingSidecar)
{
    // An older collector writes neither the clock anchor nor a .timing sidecar; the rows still parse
    // and the samples keep only their reported values.
    const std::string path = WriteData(LEGACY_INFO, "timestamp,cycles/sysfs", "1500,100\n2500,110");
    PmuCounterBundle bundle;
    std::string errorMsg;
    ASSERT_TRUE(Parse(path, bundle, errorMsg)) << errorMsg;
    EXPECT_EQ(0UL, bundle.anchorMonoNs);
    EXPECT_EQ(0UL, bundle.anchorEpochNs);
    EXPECT_EQ(100000000UL, bundle.intervalNs);
    ASSERT_EQ(2UL, bundle.samples.size());
    EXPECT_EQ(1500UL, bundle.samples[0].timestampNs);
    EXPECT_DOUBLE_EQ(110.0, bundle.samples[1].values[0]);
    EXPECT_TRUE(bundle.samples[0].rawValues.empty());
    EXPECT_TRUE(bundle.samples[0].timeEnabled.empty());
    EXPECT_TRUE(bundle.samples[0].timeRunning.empty());
}

TEST_F(HostPmuCounterParserUtest, MergesTimingSidecarIntoMatchingSample)
{
    const std::string path = WriteData(FULL_INFO, "timestamp,cycles/sysfs", "1500,100\n2500,110");
    ASSERT_TRUE(File::WriteTextAtomic(path + ".timing", "1500,0,999,888,777\n2500,0,1111,2222,3333\n"));
    PmuCounterBundle bundle;
    std::string errorMsg;
    ASSERT_TRUE(Parse(path, bundle, errorMsg)) << errorMsg;
    ASSERT_EQ(2UL, bundle.samples.size());
    ASSERT_EQ(1UL, bundle.samples[0].rawValues.size());
    EXPECT_EQ(999UL, bundle.samples[0].rawValues[0]);
    EXPECT_EQ(888UL, bundle.samples[0].timeEnabled[0]);
    EXPECT_EQ(777UL, bundle.samples[0].timeRunning[0]);
    EXPECT_EQ(1111UL, bundle.samples[1].rawValues[0]);
    EXPECT_EQ(2222UL, bundle.samples[1].timeEnabled[0]);
    EXPECT_EQ(3333UL, bundle.samples[1].timeRunning[0]);
}

TEST_F(HostPmuCounterParserUtest, IgnoresTimingLineThatDoesNotMatchAnySample)
{
    const std::string path = WriteData(FULL_INFO, "timestamp,cycles/sysfs", "1500,100");
    ASSERT_TRUE(File::WriteTextAtomic(path + ".timing", "9999,0,999,888,777\n"));
    PmuCounterBundle bundle;
    std::string errorMsg;
    ASSERT_TRUE(Parse(path, bundle, errorMsg)) << errorMsg;
    ASSERT_EQ(1UL, bundle.samples.size());
    EXPECT_TRUE(bundle.samples[0].rawValues.empty());
}

// ---------------------------------------------------------------------------
// LocateFiles
// ---------------------------------------------------------------------------

TEST_F(HostPmuCounterParserUtest, LocateFilesReturnsEmptyForRunWithoutPmuCollection)
{
    // A run that did not enable PMU collection writes no such directory at all; that is not an error.
    EXPECT_TRUE(HostPmuCounterParser::LocateFiles(File::PathJoin({TEST_ROOT, "missing"})).empty());
}

TEST_F(HostPmuCounterParserUtest, LocateFilesExcludesStatusAndTimingSidecars)
{
    const std::string hostDir = File::PathJoin({TEST_ROOT, "host"});
    const std::string dataDir = File::PathJoin({hostDir, "data", "pmu"});
    ASSERT_TRUE(File::CreateDir(hostDir));
    ASSERT_TRUE(File::CreateDir(File::PathJoin({hostDir, "data"})));
    ASSERT_TRUE(File::CreateDir(dataDir));
    const std::string content = BuildFile(HEADER, FULL_INFO, "timestamp,cycles/sysfs", "1500,100");
    ASSERT_TRUE(File::WriteTextAtomic(File::PathJoin({dataDir, "host_pmu_1.data"}), content));
    ASSERT_TRUE(File::WriteTextAtomic(File::PathJoin({dataDir, "host_pmu_2.data"}), content));
    ASSERT_TRUE(File::WriteTextAtomic(File::PathJoin({dataDir, "host_pmu_status.txt"}), "ok\n"));
    ASSERT_TRUE(File::WriteTextAtomic(File::PathJoin({dataDir, "host_pmu_1.data.timing"}), "1500,0,1,2,3\n"));
    const std::vector<std::string> files = HostPmuCounterParser::LocateFiles(TEST_ROOT);
    ASSERT_EQ(2UL, files.size());
    EXPECT_NE(std::string::npos, files[0].find("host_pmu_1.data"));
    EXPECT_NE(std::string::npos, files[1].find("host_pmu_2.data"));
}
}  // namespace Domain
}  // namespace Analysis
