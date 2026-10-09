/*
 * -------------------------------------------------------------------------
 * This file is part of the MindStudio project.
 * Copyright (c) 2025 Huawei Technologies Co.,Ltd.
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

#include <gtest/gtest.h>
#include <vector>
#include "analysis/csrc/domain/entities/json_trace/include/meta_data_event.h"

using namespace testing;
using namespace Analysis::Domain;
namespace {
std::string NAME = "process_name";
std::string THREAD_NAME = "thread_name";
std::string LABEL = "CPU";
std::string LABEL_NAME = "process_labels";
std::string INDEX_NAME = "process_sort_index";
std::string ARG_NAME = "CANN";
std::string THREAD_ARG_NAME = "thread 10";
std::string DUR_TS = "1719920055208389.438";
std::string DUR_NAME = "Node@launch";
std::string COUNTER_TS = "1719920054197069.526";
std::string COUNTER_NAME = "AI Core Freq";
std::string BEGIN_TS = "1719920063469963.587";
std::string END_TS = "1719920063492974.476";
std::string CAT = "HostToDevice";
std::string ID = "60129542143";
std::string FLOW_NAME = "HostToDevice60129542143";
std::string BEGIN_PH = "S";
std::string BEND_PH = "F";
std::string END_BP = "E";
}
class TraceEventUTest : public Test {
protected:
    virtual void SetUp()
    {}

    virtual void TearDown()
    {}
};

TEST_F(TraceEventUTest, ShouldDumpSuccessWhenRunDumpJson)
{
    std::vector<TraceEvent*> events;
    MetaDataNameEvent nameEvent(1000, 0, NAME, ARG_NAME); // pid 1000, tid 0
    MetaDataLabelEvent labelEvent(1000, 0, LABEL_NAME, LABEL); // pid 1000, tid 0
    MetaDataIndexEvent indexEvent(1000, 0, INDEX_NAME, 8); // pid 1000, tid 0 sort_index 8
    MetaDataNameEvent threadName(1000, 10, THREAD_NAME, THREAD_ARG_NAME); // pid 1000, tid 10
    events.push_back(&nameEvent);
    events.push_back(&labelEvent);
    events.push_back(&indexEvent);
    events.push_back(&threadName);
    DurationEvent durationEvent(1000, 10, 100.5, DUR_TS, DUR_NAME); // pid 1000, tid 10, dur 100.5
    events.push_back(&durationEvent);
    CounterEvent counterEvent(735530278, 0, COUNTER_TS, COUNTER_NAME); // pid 735530278, tid 0
    events.push_back(&counterEvent);
    FlowEvent beginEvent(735530054, 724245, BEGIN_TS, CAT, ID, FLOW_NAME, BEGIN_PH); // pid 735530054, tid 724245
    FlowEvent endEvent(735530854, 0, END_TS, CAT, ID, FLOW_NAME, BEND_PH, END_BP); // pid 735530854, tid 0
    events.push_back(&beginEvent);
    events.push_back(&endEvent);
    JsonWriter ostream;
    ostream.StartArray();
    for (const auto &node : events) {
        node->DumpJson(ostream);
    }
    ostream.EndArray();
    std::string expectJsonStr = "[{\"name\":\"process_name\",\"pid\":1000,\"tid\":0,\"ph\":\"M\",\"args\":{\"name\":"
                                "\"CANN\"}},{\"name\":\"process_labels\",\"pid\":1000,\"tid\":0,\"ph\":\"M\",\"args\""
                                ":{\"labels\":\"CPU\"}},{\"name\":\"process_sort_index\",\"pid\":1000,\"tid\":0,\"ph\":"
                                "\"M\",\"args\":{\"sort_index\":8}},{\"name\":\"thread_name\",\"pid\":1000,\"tid\":10,"
                                "\"ph\":\"M\",\"args\":{\"name\":\"thread 10\"}},{\"name\":\"Node@launch\",\"pid\":"
                                "1000,\"tid\":10,\"ts\":\"1719920055208389.438\",\"dur\":100.5,\"ph\":\"X\",\"args\":"
                                "{}},{\"name\":\"AI Core Freq\",\"pid\":735530278,\"tid\":0,\"ts\":"
                                "\"1719920054197069.526\",\"ph\":\"C\",\"args\":{}},"
                                "{\"name\":\"HostToDevice60129542143\",\"pid\":"
                                "735530054,\"tid\":724245,\"ph\":\"S\",\"cat\":\"HostToDevice\",\"id\":\"60129542143\","
                                "\"ts\":\"1719920063469963.587\"},{\"name\":\"HostToDevice60129542143\",\"pid\":"
                                "735530854,\"tid\":0,\"ph\":\"F\",\"cat\":\"HostToDevice\",\"id\":\"60129542143\","
                                "\"ts\":\"1719920063492974.476\",\"bp\":\"E\"}]";
    EXPECT_EQ(expectJsonStr, ostream.GetString());
}