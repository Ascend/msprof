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

#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "analysis/csrc/application/timeline/overlap_analysis_assembler.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/api_data.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"
#include "analysis/csrc/domain/services/environment/context.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/domain/entities/time/time_duration.h"

using namespace Analysis::Application;
using namespace Analysis::Utils;
using namespace Analysis::Domain;
using namespace Analysis::Viewer::Database;
using namespace Analysis::Domain::Environment;

namespace {
const int DEPTH = 0;
const std::string BASE_PATH = "./ascend_test";
const std::string PROF_PATH = File::PathJoin({BASE_PATH, "PROF_0"});
const std::string RESULT_PATH = File::PathJoin({PROF_PATH, OUTPUT_PATH});
const std::string SCENARIOS1_JSON =
    "{\"name\":\"process_name\",\"pid\":10330081,\"tid\":0,\"ph\":\"M\",\"args\":{\"name\":\"Overlap Analysis\"}},"
    "{\"name\":\"process_labels\",\"pid\":10330081,\"tid\":0,\"ph\":\"M\",\"args\":{\"labels\":\"NPU 1\"}},"
    "{\"name\":\"process_sort_index\",\"pid\":10330081,\"tid\":0,\"ph\":\"M\",\"args\":{\"sort_index\":31}},"
    "{\"name\":\"process_name\",\"pid\":10330080,\"tid\":0,\"ph\":\"M\",\"args\":{\"name\":\"Overlap Analysis\"}},"
    "{\"name\":\"process_labels\",\"pid\":10330080,\"tid\":0,\"ph\":\"M\",\"args\":{\"labels\":\"NPU 0\"}},"
    "{\"name\":\"process_sort_index\",\"pid\":10330080,\"tid\":0,\"ph\":\"M\",\"args\":{\"sort_index\":31}},"
    "{\"name\":\"thread_name\",\"pid\":10330080,\"tid\":0,\"ph\":\"M\",\"args\":{\"name\":\"Communication\"}},"
    "{\"name\":\"thread_sort_index\",\"pid\":10330080,\"tid\":0,\"ph\":\"M\",\"args\":{\"sort_index\":0}},"
    "{\"name\":\"thread_name\",\"pid\":10330080,\"tid\":1,\"ph\":\"M\","
    "\"args\":{\"name\":\"Communication(Not Overlapped)\"}},"
    "{\"name\":\"thread_sort_index\",\"pid\":10330080,\"tid\":1,\"ph\":\"M\",\"args\":{\"sort_index\":1}},"
    "{\"name\":\"thread_name\",\"pid\":10330080,\"tid\":2,\"ph\":\"M\",\"args\":{\"name\":\"Computing\"}},"
    "{\"name\":\"thread_sort_index\",\"pid\":10330080,\"tid\":2,\"ph\":\"M\",\"args\":{\"sort_index\":2}},"
    "{\"name\":\"thread_name\",\"pid\":10330080,\"tid\":3,\"ph\":\"M\",\"args\":{\"name\":\"Free\"}},"
    "{\"name\":\"thread_sort_index\",\"pid\":10330080,\"tid\":3,\"ph\":\"M\",\"args\":{\"sort_index\":3}},"
    "{\"name\":\"Computing\",\"pid\":10330080,\"tid\":2,\"ts\":\"0.001\","
    "\"dur\":0.002,\"ph\":\"X\",\"args\":{}},"
    "{\"name\":\"Computing\",\"pid\":10330080,\"tid\":2,\"ts\":\"0.006\","
    "\"dur\":0.003,\"ph\":\"X\",\"args\":{}},"
    "{\"name\":\"Computing\",\"pid\":10330080,\"tid\":2,\"ts\":\"0.015\","
    "\"dur\":0.009,\"ph\":\"X\",\"args\":{}},"
    "{\"name\":\"Computing\",\"pid\":10330080,\"tid\":2,\"ts\":\"0.026\","
    "\"dur\":0.007,\"ph\":\"X\",\"args\":{}},"
    "{\"name\":\"Computing\",\"pid\":10330080,\"tid\":2,\"ts\":\"0.034\","
    "\"dur\":0.002,\"ph\":\"X\",\"args\":{}},"
    "{\"name\":\"Communication\",\"pid\":10330080,\"tid\":0,\"ts\":\"0.004\","
    "\"dur\":0.009,\"ph\":\"X\",\"args\":{}},"
    "{\"name\":\"Communication\",\"pid\":10330080,\"tid\":0,\"ts\":\"0.014\","
    "\"dur\":0.003,\"ph\":\"X\",\"args\":{}},"
    "{\"name\":\"Communication\",\"pid\":10330080,\"tid\":0,\"ts\":\"0.020\","
    "\"dur\":0.004,\"ph\":\"X\",\"args\":{}},"
    "{\"name\":\"Communication\",\"pid\":10330080,\"tid\":0,\"ts\":\"0.025\","
    "\"dur\":0.016,\"ph\":\"X\",\"args\":{}},"
    "{\"name\":\"Communication(Not Overlapped)\",\"pid\":10330080,\"tid\":1,\"ts\":\"0.004\","
    "\"dur\":0.002,\"ph\":\"X\",\"args\":{}},"
    "{\"name\":\"Communication(Not Overlapped)\",\"pid\":10330080,\"tid\":1,\"ts\":\"0.009\","
    "\"dur\":0.004,\"ph\":\"X\",\"args\":{}},"
    "{\"name\":\"Communication(Not Overlapped)\",\"pid\":10330080,\"tid\":1,\"ts\":\"0.014\","
    "\"dur\":0.001,\"ph\":\"X\",\"args\":{}},"
    "{\"name\":\"Communication(Not Overlapped)\",\"pid\":10330080,\"tid\":1,\"ts\":\"0.025\","
    "\"dur\":0.001,\"ph\":\"X\",\"args\":{}},"
    "{\"name\":\"Communication(Not Overlapped)\",\"pid\":10330080,\"tid\":1,\"ts\":\"0.033\","
    "\"dur\":0.001,\"ph\":\"X\",\"args\":{}},"
    "{\"name\":\"Communication(Not Overlapped)\",\"pid\":10330080,\"tid\":1,\"ts\":\"0.036\","
    "\"dur\":0.005,\"ph\":\"X\",\"args\":{}},"
    "{\"name\":\"Free\",\"pid\":10330080,\"tid\":3,\"ts\":\"0.003\","
    "\"dur\":0.001,\"ph\":\"X\",\"args\":{}},"
    "{\"name\":\"Free\",\"pid\":10330080,\"tid\":3,\"ts\":\"0.013\","
    "\"dur\":0.001,\"ph\":\"X\",\"args\":{}},"
    "{\"name\":\"Free\",\"pid\":10330080,\"tid\":3,\"ts\":\"0.024\","
    "\"dur\":0.001,\"ph\":\"X\",\"args\":{}},"
    "{\"name\":\"process_name\",\"pid\":10330081,\"tid\":0,\"ph\":\"M\","
    "\"args\":{\"name\":\"Overlap Analysis\"}},"
    "{\"name\":\"process_labels\",\"pid\":10330081,\"tid\":0,\"ph\":\"M\","
    "\"args\":{\"labels\":\"NPU 1\"}},"
    "{\"name\":\"process_sort_index\",\"pid\":10330081,\"tid\":0,\"ph\":\"M\",\"args\":{\"sort_index\":31}},"
    "{\"name\":\"process_name\",\"pid\":10330080,\"tid\":0,\"ph\":\"M\",\"args\":{\"name\":\"Overlap Analysis\"}},"
    "{\"name\":\"process_labels\",\"pid\":10330080,\"tid\":0,\"ph\":\"M\",\"args\":{\"labels\":\"NPU 0\"}},"
    "{\"name\":\"process_sort_index\",\"pid\":10330080,\"tid\":0,\"ph\":\"M\",\"args\":{\"sort_index\":31}},"
    "{\"name\":\"thread_name\",\"pid\":10330081,\"tid\":0,\"ph\":\"M\",\"args\":{\"name\":\"Communication\"}},"
    "{\"name\":\"thread_sort_index\",\"pid\":10330081,\"tid\":0,\"ph\":\"M\",\"args\":{\"sort_index\":0}},"
    "{\"name\":\"thread_name\",\"pid\":10330081,\"tid\":1,\"ph\":\"M\","
    "\"args\":{\"name\":\"Communication(Not Overlapped)\"}},"
    "{\"name\":\"thread_sort_index\",\"pid\":10330081,\"tid\":1,\"ph\":\"M\",\"args\":{\"sort_index\":1}},"
    "{\"name\":\"thread_name\",\"pid\":10330081,\"tid\":2,\"ph\":\"M\",\"args\":{\"name\":\"Computing\"}},"
    "{\"name\":\"thread_sort_index\",\"pid\":10330081,\"tid\":2,\"ph\":\"M\",\"args\":{\"sort_index\":2}},"
    "{\"name\":\"thread_name\",\"pid\":10330081,\"tid\":3,\"ph\":\"M\",\"args\":{\"name\":\"Free\"}},"
    "{\"name\":\"thread_sort_index\",\"pid\":10330081,\"tid\":3,\"ph\":\"M\",\"args\":{\"sort_index\":3}},"
    "{\"name\":\"Computing\",\"pid\":10330081,\"tid\":2,\"ts\":\"0.001\","
    "\"dur\":0.002,\"ph\":\"X\",\"args\":{}},"
    "{\"name\":\"Computing\",\"pid\":10330081,\"tid\":2,\"ts\":\"0.006\","
    "\"dur\":0.003,\"ph\":\"X\",\"args\":{}},"
    "{\"name\":\"Computing\",\"pid\":10330081,\"tid\":2,\"ts\":\"0.015\","
    "\"dur\":0.009,\"ph\":\"X\",\"args\":{}},"
    "{\"name\":\"Computing\",\"pid\":10330081,\"tid\":2,\"ts\":\"0.026\","
    "\"dur\":0.007,\"ph\":\"X\",\"args\":{}},"
    "{\"name\":\"Computing\",\"pid\":10330081,\"tid\":2,\"ts\":\"0.034\","
    "\"dur\":0.002,\"ph\":\"X\",\"args\":{}},"
    "{\"name\":\"Communication\",\"pid\":10330081,\"tid\":0,\"ts\":\"0.004\","
    "\"dur\":0.009,\"ph\":\"X\",\"args\":{}},"
    "{\"name\":\"Communication\",\"pid\":10330081,\"tid\":0,\"ts\":\"0.014\","
    "\"dur\":0.003,\"ph\":\"X\",\"args\":{}},"
    "{\"name\":\"Communication\",\"pid\":10330081,\"tid\":0,\"ts\":\"0.020\","
    "\"dur\":0.004,\"ph\":\"X\",\"args\":{}},"
    "{\"name\":\"Communication\",\"pid\":10330081,\"tid\":0,\"ts\":\"0.025\","
    "\"dur\":0.016,\"ph\":\"X\",\"args\":{}},"
    "{\"name\":\"Communication(Not Overlapped)\",\"pid\":10330081,\"tid\":1,\"ts\":\"0.004\","
    "\"dur\":0.002,\"ph\":\"X\",\"args\":{}},"
    "{\"name\":\"Communication(Not Overlapped)\",\"pid\":10330081,\"tid\":1,\"ts\":\"0.009\","
    "\"dur\":0.004,\"ph\":\"X\",\"args\":{}},"
    "{\"name\":\"Communication(Not Overlapped)\",\"pid\":10330081,\"tid\":1,\"ts\":\"0.014\","
    "\"dur\":0.001,\"ph\":\"X\",\"args\":{}},"
    "{\"name\":\"Communication(Not Overlapped)\",\"pid\":10330081,\"tid\":1,\"ts\":\"0.025\","
    "\"dur\":0.001,\"ph\":\"X\",\"args\":{}},"
    "{\"name\":\"Communication(Not Overlapped)\",\"pid\":10330081,\"tid\":1,\"ts\":\"0.033\","
    "\"dur\":0.001,\"ph\":\"X\",\"args\":{}},"
    "{\"name\":\"Communication(Not Overlapped)\",\"pid\":10330081,\"tid\":1,\"ts\":\"0.036\","
    "\"dur\":0.005,\"ph\":\"X\",\"args\":{}},"
    "{\"name\":\"Free\",\"pid\":10330081,\"tid\":3,\"ts\":\"0.003\",\"dur\":0.001,\"ph\":\"X\",\"args\":{}},"
    "{\"name\":\"Free\",\"pid\":10330081,\"tid\":3,\"ts\":\"0.013\",\"dur\":0.001,\"ph\":\"X\",\"args\":{}},"
    "{\"name\":\"Free\",\"pid\":10330081,\"tid\":3,\"ts\":\"0.024\",\"dur\":0.001,\"ph\":\"X\",\"args\":{}},";
const std::string SCENARIOS2_JSON =
    "{\"name\":\"process_name\",\"pid\":10330080,\"tid\":0,\"ph\":\"M\",\"args\":{\"name\":\"Overlap Analysis\"}},"
    "{\"name\":\"process_labels\",\"pid\":10330080,\"tid\":0,\"ph\":\"M\",\"args\":{\"labels\":\"NPU 0\"}},"
    "{\"name\":\"process_sort_index\",\"pid\":10330080,\"tid\":0,\"ph\":\"M\",\"args\":{\"sort_index\":31}},"
    "{\"name\":\"thread_name\",\"pid\":10330080,\"tid\":0,\"ph\":\"M\",\"args\":{\"name\":\"Communication\"}},"
    "{\"name\":\"thread_sort_index\",\"pid\":10330080,\"tid\":0,\"ph\":\"M\",\"args\":{\"sort_index\":0}},"
    "{\"name\":\"thread_name\",\"pid\":10330080,\"tid\":1,\"ph\":\"M\","
    "\"args\":{\"name\":\"Communication(Not Overlapped)\"}},"
    "{\"name\":\"thread_sort_index\",\"pid\":10330080,\"tid\":1,\"ph\":\"M\",\"args\":{\"sort_index\":1}},"
    "{\"name\":\"thread_name\",\"pid\":10330080,\"tid\":2,\"ph\":\"M\",\"args\":{\"name\":\"Computing\"}},"
    "{\"name\":\"thread_sort_index\",\"pid\":10330080,\"tid\":2,\"ph\":\"M\",\"args\":{\"sort_index\":2}},"
    "{\"name\":\"thread_name\",\"pid\":10330080,\"tid\":3,\"ph\":\"M\",\"args\":{\"name\":\"Free\"}},"
    "{\"name\":\"thread_sort_index\",\"pid\":10330080,\"tid\":3,\"ph\":\"M\",\"args\":{\"sort_index\":3}},"
    "{\"name\":\"Computing\",\"pid\":10330080,\"tid\":2,\"ts\":\"0.001\",\"dur\":0.002,\"ph\":\"X\",\"args\":{}},"
    "{\"name\":\"Computing\",\"pid\":10330080,\"tid\":2,\"ts\":\"0.006\",\"dur\":0.003,\"ph\":\"X\",\"args\":{}},"
    "{\"name\":\"Computing\",\"pid\":10330080,\"tid\":2,\"ts\":\"0.015\",\"dur\":0.009,\"ph\":\"X\",\"args\":{}},"
    "{\"name\":\"Computing\",\"pid\":10330080,\"tid\":2,\"ts\":\"0.026\",\"dur\":0.007,\"ph\":\"X\",\"args\":{}},"
    "{\"name\":\"Computing\",\"pid\":10330080,\"tid\":2,\"ts\":\"0.034\",\"dur\":0.002,\"ph\":\"X\",\"args\":{}},"
    "{\"name\":\"Communication\",\"pid\":10330080,\"tid\":0,\"ts\":\"0.004\","
    "\"dur\":0.004,\"ph\":\"X\",\"args\":{}},"
    "{\"name\":\"Communication\",\"pid\":10330080,\"tid\":0,\"ts\":\"0.009\","
    "\"dur\":0.004,\"ph\":\"X\",\"args\":{}},"
    "{\"name\":\"Communication\",\"pid\":10330080,\"tid\":0,\"ts\":\"0.014\","
    "\"dur\":0.003,\"ph\":\"X\",\"args\":{}},"
    "{\"name\":\"Communication\",\"pid\":10330080,\"tid\":0,\"ts\":\"0.020\","
    "\"dur\":0.004,\"ph\":\"X\",\"args\":{}},"
    "{\"name\":\"Communication\",\"pid\":10330080,\"tid\":0,\"ts\":\"0.025\","
    "\"dur\":0.016,\"ph\":\"X\",\"args\":{}},"
    "{\"name\":\"Communication(Not Overlapped)\",\"pid\":10330080,\"tid\":1,\"ts\":\"0.004\","
    "\"dur\":0.002,\"ph\":\"X\",\"args\":{}},"
    "{\"name\":\"Communication(Not Overlapped)\",\"pid\":10330080,\"tid\":1,\"ts\":\"0.009\","
    "\"dur\":0.004,\"ph\":\"X\",\"args\":{}},"
    "{\"name\":\"Communication(Not Overlapped)\",\"pid\":10330080,\"tid\":1,\"ts\":\"0.014\","
    "\"dur\":0.001,\"ph\":\"X\",\"args\":{}},"
    "{\"name\":\"Communication(Not Overlapped)\",\"pid\":10330080,\"tid\":1,\"ts\":\"0.025\","
    "\"dur\":0.001,\"ph\":\"X\",\"args\":{}},"
    "{\"name\":\"Communication(Not Overlapped)\",\"pid\":10330080,\"tid\":1,\"ts\":\"0.033\","
    "\"dur\":0.001,\"ph\":\"X\",\"args\":{}},"
    "{\"name\":\"Communication(Not Overlapped)\",\"pid\":10330080,\"tid\":1,\"ts\":\"0.036\","
    "\"dur\":0.005,\"ph\":\"X\",\"args\":{}},"
    "{\"name\":\"Free\",\"pid\":10330080,\"tid\":3,\"ts\":\"0.003\",\"dur\":0.001,\"ph\":\"X\",\"args\":{}},"
    "{\"name\":\"Free\",\"pid\":10330080,\"tid\":3,\"ts\":\"0.013\",\"dur\":0.001,\"ph\":\"X\",\"args\":{}},"
    "{\"name\":\"Free\",\"pid\":10330080,\"tid\":3,\"ts\":\"0.024\",\"dur\":0.001,\"ph\":\"X\",\"args\":{}},";
const std::string SCENARIOS3_JSON =
    "{\"name\":\"process_name\",\"pid\":10330080,\"tid\":0,\"ph\":\"M\",\"args\":{\"name\":\"Overlap Analysis\"}},"
    "{\"name\":\"process_labels\",\"pid\":10330080,\"tid\":0,\"ph\":\"M\",\"args\":{\"labels\":\"NPU 0\"}},"
    "{\"name\":\"process_sort_index\",\"pid\":10330080,\"tid\":0,\"ph\":\"M\",\"args\":{\"sort_index\":31}},"
    "{\"name\":\"thread_name\",\"pid\":10330080,\"tid\":0,\"ph\":\"M\",\"args\":{\"name\":\"Communication\"}},"
    "{\"name\":\"thread_sort_index\",\"pid\":10330080,\"tid\":0,\"ph\":\"M\",\"args\":{\"sort_index\":0}},"
    "{\"name\":\"thread_name\",\"pid\":10330080,\"tid\":1,\"ph\":\"M\","
    "\"args\":{\"name\":\"Communication(Not Overlapped)\"}},"
    "{\"name\":\"thread_sort_index\",\"pid\":10330080,\"tid\":1,\"ph\":\"M\",\"args\":{\"sort_index\":1}},"
    "{\"name\":\"thread_name\",\"pid\":10330080,\"tid\":2,\"ph\":\"M\",\"args\":{\"name\":\"Computing\"}},"
    "{\"name\":\"thread_sort_index\",\"pid\":10330080,\"tid\":2,\"ph\":\"M\",\"args\":{\"sort_index\":2}},"
    "{\"name\":\"thread_name\",\"pid\":10330080,\"tid\":3,\"ph\":\"M\",\"args\":{\"name\":\"Free\"}},"
    "{\"name\":\"thread_sort_index\",\"pid\":10330080,\"tid\":3,\"ph\":\"M\",\"args\":{\"sort_index\":3}},"
    "{\"name\":\"Computing\",\"pid\":10330080,\"tid\":2,\"ts\":\"0.001\",\"dur\":0.002,\"ph\":\"X\",\"args\":{}},"
    "{\"name\":\"Computing\",\"pid\":10330080,\"tid\":2,\"ts\":\"0.006\",\"dur\":0.003,\"ph\":\"X\",\"args\":{}},"
    "{\"name\":\"Computing\",\"pid\":10330080,\"tid\":2,\"ts\":\"0.015\",\"dur\":0.009,\"ph\":\"X\",\"args\":{}},"
    "{\"name\":\"Computing\",\"pid\":10330080,\"tid\":2,\"ts\":\"0.026\",\"dur\":0.007,\"ph\":\"X\",\"args\":{}},"
    "{\"name\":\"Computing\",\"pid\":10330080,\"tid\":2,\"ts\":\"0.034\",\"dur\":0.002,\"ph\":\"X\",\"args\":{}},"
    "{\"name\":\"Free\",\"pid\":10330080,\"tid\":3,\"ts\":\"0.003\",\"dur\":0.003,\"ph\":\"X\",\"args\":{}},"
    "{\"name\":\"Free\",\"pid\":10330080,\"tid\":3,\"ts\":\"0.009\",\"dur\":0.006,\"ph\":\"X\",\"args\":{}},"
    "{\"name\":\"Free\",\"pid\":10330080,\"tid\":3,\"ts\":\"0.024\",\"dur\":0.002,\"ph\":\"X\",\"args\":{}},"
    "{\"name\":\"Free\",\"pid\":10330080,\"tid\":3,\"ts\":\"0.033\",\"dur\":0.001,\"ph\":\"X\",\"args\":{}},";
const std::string SCENARIOS4_JSON =
    "{\"name\":\"process_name\",\"pid\":10330080,\"tid\":0,\"ph\":\"M\",\"args\":{\"name\":\"Overlap Analysis\"}},"
    "{\"name\":\"process_labels\",\"pid\":10330080,\"tid\":0,\"ph\":\"M\",\"args\":{\"labels\":\"NPU 0\"}},"
    "{\"name\":\"process_sort_index\",\"pid\":10330080,\"tid\":0,\"ph\":\"M\",\"args\":{\"sort_index\":31}},"
    "{\"name\":\"thread_name\",\"pid\":10330080,\"tid\":0,\"ph\":\"M\",\"args\":{\"name\":\"Communication\"}},"
    "{\"name\":\"thread_sort_index\",\"pid\":10330080,\"tid\":0,\"ph\":\"M\",\"args\":{\"sort_index\":0}},"
    "{\"name\":\"thread_name\",\"pid\":10330080,\"tid\":1,\"ph\":\"M\","
    "\"args\":{\"name\":\"Communication(Not Overlapped)\"}},"
    "{\"name\":\"thread_sort_index\",\"pid\":10330080,\"tid\":1,\"ph\":\"M\",\"args\":{\"sort_index\":1}},"
    "{\"name\":\"thread_name\",\"pid\":10330080,\"tid\":2,\"ph\":\"M\",\"args\":{\"name\":\"Computing\"}},"
    "{\"name\":\"thread_sort_index\",\"pid\":10330080,\"tid\":2,\"ph\":\"M\",\"args\":{\"sort_index\":2}},"
    "{\"name\":\"thread_name\",\"pid\":10330080,\"tid\":3,\"ph\":\"M\",\"args\":{\"name\":\"Free\"}},"
    "{\"name\":\"thread_sort_index\",\"pid\":10330080,\"tid\":3,\"ph\":\"M\",\"args\":{\"sort_index\":3}},"
    "{\"name\":\"Communication\",\"pid\":10330080,\"tid\":0,\"ts\":\"0.004\","
    "\"dur\":0.004,\"ph\":\"X\",\"args\":{}},"
    "{\"name\":\"Communication\",\"pid\":10330080,\"tid\":0,\"ts\":\"0.009\","
    "\"dur\":0.004,\"ph\":\"X\",\"args\":{}},"
    "{\"name\":\"Communication\",\"pid\":10330080,\"tid\":0,\"ts\":\"0.014\","
    "\"dur\":0.003,\"ph\":\"X\",\"args\":{}},"
    "{\"name\":\"Communication\",\"pid\":10330080,\"tid\":0,\"ts\":\"0.020\","
    "\"dur\":0.004,\"ph\":\"X\",\"args\":{}},"
    "{\"name\":\"Communication\",\"pid\":10330080,\"tid\":0,\"ts\":\"0.025\","
    "\"dur\":0.016,\"ph\":\"X\",\"args\":{}},"
    "{\"name\":\"Communication(Not Overlapped)\",\"pid\":10330080,\"tid\":1,\"ts\":\"0.004\","
    "\"dur\":0.004,\"ph\":\"X\",\"args\":{}},"
    "{\"name\":\"Communication(Not Overlapped)\",\"pid\":10330080,\"tid\":1,\"ts\":\"0.009\","
    "\"dur\":0.004,\"ph\":\"X\",\"args\":{}},"
    "{\"name\":\"Communication(Not Overlapped)\",\"pid\":10330080,\"tid\":1,\"ts\":\"0.014\","
    "\"dur\":0.003,\"ph\":\"X\",\"args\":{}},"
    "{\"name\":\"Communication(Not Overlapped)\",\"pid\":10330080,\"tid\":1,\"ts\":\"0.020\","
    "\"dur\":0.004,\"ph\":\"X\",\"args\":{}},"
    "{\"name\":\"Communication(Not Overlapped)\",\"pid\":10330080,\"tid\":1,\"ts\":\"0.025\","
    "\"dur\":0.016,\"ph\":\"X\",\"args\":{}},"
    "{\"name\":\"Free\",\"pid\":10330080,\"tid\":3,\"ts\":\"0.008\",\"dur\":0.001,\"ph\":\"X\",\"args\":{}},"
    "{\"name\":\"Free\",\"pid\":10330080,\"tid\":3,\"ts\":\"0.013\",\"dur\":0.001,\"ph\":\"X\",\"args\":{}},"
    "{\"name\":\"Free\",\"pid\":10330080,\"tid\":3,\"ts\":\"0.017\",\"dur\":0.003,\"ph\":\"X\",\"args\":{}},"
    "{\"name\":\"Free\",\"pid\":10330080,\"tid\":3,\"ts\":\"0.024\",\"dur\":0.001,\"ph\":\"X\",\"args\":{}},";
const std::string SCENARIOS5_JSON =
    "{\"name\":\"process_name\",\"pid\":10330080,\"tid\":0,\"ph\":\"M\",\"args\":{\"name\":\"Overlap Analysis\"}},"
    "{\"name\":\"process_labels\",\"pid\":10330080,\"tid\":0,\"ph\":\"M\",\"args\":{\"labels\":\"NPU 0\"}},"
    "{\"name\":\"process_sort_index\",\"pid\":10330080,\"tid\":0,\"ph\":\"M\",\"args\":{\"sort_index\":31}},"
    "{\"name\":\"thread_name\",\"pid\":10330080,\"tid\":0,\"ph\":\"M\",\"args\":{\"name\":\"Communication\"}},"
    "{\"name\":\"thread_sort_index\",\"pid\":10330080,\"tid\":0,\"ph\":\"M\",\"args\":{\"sort_index\":0}},"
    "{\"name\":\"thread_name\",\"pid\":10330080,\"tid\":1,\"ph\":\"M\",\"args\":"
    "{\"name\":\"Communication(Not Overlapped)\"}},"
    "{\"name\":\"thread_sort_index\",\"pid\":10330080,\"tid\":1,\"ph\":\"M\",\"args\":{\"sort_index\":1}},"
    "{\"name\":\"thread_name\",\"pid\":10330080,\"tid\":2,\"ph\":\"M\",\"args\":{\"name\":\"Computing\"}},"
    "{\"name\":\"thread_sort_index\",\"pid\":10330080,\"tid\":2,\"ph\":\"M\",\"args\":{\"sort_index\":2}},"
    "{\"name\":\"thread_name\",\"pid\":10330080,\"tid\":3,\"ph\":\"M\",\"args\":{\"name\":\"Free\"}},"
    "{\"name\":\"thread_sort_index\",\"pid\":10330080,\"tid\":3,\"ph\":\"M\",\"args\":{\"sort_index\":3}},"
    "{\"name\":\"Free\",\"pid\":10330080,\"tid\":3,\"ts\":\"0.001\",\"dur\":0.035,\"ph\":\"X\",\"args\":{}},";
const std::string SCENARIOS6_JSON =
    "{\"name\":\"process_name\",\"pid\":10330080,\"tid\":0,\"ph\":\"M\",\"args\":{\"name\":\"Overlap Analysis\"}},"
    "{\"name\":\"process_labels\",\"pid\":10330080,\"tid\":0,\"ph\":\"M\",\"args\":{\"labels\":\"NPU 0\"}},"
    "{\"name\":\"process_sort_index\",\"pid\":10330080,\"tid\":0,\"ph\":\"M\",\"args\":{\"sort_index\":31}},"
    "{\"name\":\"thread_name\",\"pid\":10330080,\"tid\":0,\"ph\":\"M\",\"args\":{\"name\":\"Communication\"}},"
    "{\"name\":\"thread_sort_index\",\"pid\":10330080,\"tid\":0,\"ph\":\"M\",\"args\":{\"sort_index\":0}},"
    "{\"name\":\"thread_name\",\"pid\":10330080,\"tid\":1,\"ph\":\"M\",\"args\":"
    "{\"name\":\"Communication(Not Overlapped)\"}},"
    "{\"name\":\"thread_sort_index\",\"pid\":10330080,\"tid\":1,\"ph\":\"M\",\"args\":{\"sort_index\":1}},"
    "{\"name\":\"thread_name\",\"pid\":10330080,\"tid\":2,\"ph\":\"M\",\"args\":{\"name\":\"Computing\"}},"
    "{\"name\":\"thread_sort_index\",\"pid\":10330080,\"tid\":2,\"ph\":\"M\",\"args\":{\"sort_index\":2}},"
    "{\"name\":\"thread_name\",\"pid\":10330080,\"tid\":3,\"ph\":\"M\",\"args\":{\"name\":\"Free\"}},"
    "{\"name\":\"thread_sort_index\",\"pid\":10330080,\"tid\":3,\"ph\":\"M\",\"args\":{\"sort_index\":3}},"
    "{\"name\":\"Computing\",\"pid\":10330080,\"tid\":2,\"ts\":\"0.005\",\"dur\":0.002,\"ph\":\"X\",\"args\":{}},"
    "{\"name\":\"Computing\",\"pid\":10330080,\"tid\":2,\"ts\":\"0.010\",\"dur\":0.002,\"ph\":\"X\",\"args\":{}},"
    "{\"name\":\"Computing\",\"pid\":10330080,\"tid\":2,\"ts\":\"0.014\",\"dur\":0.002,\"ph\":\"X\",\"args\":{}},"
    "{\"name\":\"Computing\",\"pid\":10330080,\"tid\":2,\"ts\":\"0.020\",\"dur\":0.003,\"ph\":\"X\",\"args\":{}},"
    "{\"name\":\"Computing\",\"pid\":10330080,\"tid\":2,\"ts\":\"0.024\",\"dur\":0.001,\"ph\":\"X\",\"args\":{}},"
    "{\"name\":\"Computing\",\"pid\":10330080,\"tid\":2,\"ts\":\"0.030\",\"dur\":0.007,\"ph\":\"X\",\"args\":{}},"
    "{\"name\":\"Free\",\"pid\":10330080,\"tid\":3,\"ts\":\"0.007\",\"dur\":0.003,\"ph\":\"X\",\"args\":{}},"
    "{\"name\":\"Free\",\"pid\":10330080,\"tid\":3,\"ts\":\"0.012\",\"dur\":0.002,\"ph\":\"X\",\"args\":{}},"
    "{\"name\":\"Free\",\"pid\":10330080,\"tid\":3,\"ts\":\"0.016\",\"dur\":0.004,\"ph\":\"X\",\"args\":{}},"
    "{\"name\":\"Free\",\"pid\":10330080,\"tid\":3,\"ts\":\"0.023\",\"dur\":0.001,\"ph\":\"X\",\"args\":{}},"
    "{\"name\":\"Free\",\"pid\":10330080,\"tid\":3,\"ts\":\"0.025\",\"dur\":0.005,\"ph\":\"X\",\"args\":{}},";

}

class OverlapAnalysisAssemblerUTest : public testing::Test {
protected:
    virtual void TearDown()
    {
        EXPECT_TRUE(File::RemoveDir(BASE_PATH, DEPTH));
        dataInventory_.RemoveRestData({});
        GlobalMockObject::verify();
    }
    virtual void SetUp()
    {
        if (File::Check(BASE_PATH)) {
            File::RemoveDir(BASE_PATH, DEPTH);
        }
        EXPECT_TRUE(File::CreateDir(BASE_PATH));
        EXPECT_TRUE(File::CreateDir(PROF_PATH));
        EXPECT_TRUE(File::CreateDir(RESULT_PATH));
    }
protected:
    DataInventory dataInventory_;
};
static std::vector<AscendTaskData> GenerateAscendTaskDataS1ByDevice(uint16_t deviceId)
{
    std::vector<AscendTaskData> res;
    // 注意有2个streamId=3, taskId=2
    std::vector<TaskId> ids = {{1, 0, 0, 0}, {1, 0, 1, 0}, {1, 0, 2, 0}, {2, 0, 0, 0}, {2, 0, 1, 0}, {2, 0, 2, 0},
                               {2, 0, 3, 0}, {2, 0, 4, 0}, {2, 0, 5, 0}, {3, 0, 0, 0}, {3, 0, 1, 0}, {3, 0, 2, 0},
                               {3, 0, 3, 0}, {3, 0, 2, 0}, {3, 0, 5, 0}};
    std::vector<std::pair<uint64_t, double>> times{{5, 5.0}, {20, 3.0}, {31, 4.0}, {1, 2.0}, {6, 2.0}, {10, 2.0},
                                                   {16, 3.0}, {20, 1.0}, {26, 7.0}, {1, 4.0}, {7, 2.0}, {15, 3.0},
                                                   {19, 5.0}, {29, 3.0}, {34, 2.0}};
    for (uint32_t i = 0; i < ids.size(); i++) {
        AscendTaskData data;
        data.deviceId = deviceId;
        data.streamId = ids[i].streamId;
        data.taskId = ids[i].taskId;
        data.contextId = ids[i].contextId;
        data.batchId = ids[i].batchId;
        data.timestamp = times[i].first;
        data.duration = times[i].second;
        res.push_back(data);
    }
    return res;
}
static std::vector<AscendTaskData> GenerateAscendTaskDataS2ByDevice(uint16_t deviceId)
{
    std::vector<AscendTaskData> res;
    // 注意有2个streamId=3, taskId=2
    std::vector<TaskId> ids = {{2, 0, 0, 0}, {2, 0, 1, 0}, {2, 0, 2, 0}, {2, 0, 3, 0}, {2, 0, 4, 0}, {2, 0, 5, 0},
                               {3, 0, 0, 0}, {3, 0, 1, 0}, {3, 0, 2, 0}, {3, 0, 3, 0}, {3, 0, 2, 0}, {3, 0, 5, 0}};
    std::vector<std::pair<uint64_t, double>> times{{1, 2.0}, {6, 2.0}, {10, 2.0}, {16, 3.0}, {20, 1.0}, {26, 7.0},
                                                   {1, 4.0}, {7, 2.0}, {15, 3.0}, {19, 5.0}, {29, 3.0}, {34, 2.0}};
    for (uint32_t i = 0; i < ids.size(); i++) {
        AscendTaskData data;
        data.deviceId = deviceId;
        data.streamId = ids[i].streamId;
        data.taskId = ids[i].taskId;
        data.contextId = ids[i].contextId;
        data.batchId = ids[i].batchId;
        data.timestamp = times[i].first;
        data.duration = times[i].second;
        res.push_back(data);
    }
    return res;
}
static std::vector<AscendTaskData> GeneratePureCompAscendTaskDataByDevice(uint16_t deviceId)
{
    std::vector<AscendTaskData> res;
    std::vector<TaskId> ids = {{2, 0, 0, 0}, {2, 0, 1, 0}, {2, 0, 2, 0}, {2, 0, 3, 0}, {2, 0, 4, 0}, {2, 0, 5, 0}};
    std::vector<std::pair<uint64_t, double>> times{{5, 2.0}, {10, 2.0}, {14, 2.0}, {20, 3.0}, {24, 1.0}, {30, 7.0}};
    for (uint32_t i = 0; i < ids.size(); i++) {
        AscendTaskData data;
        data.deviceId = deviceId;
        data.streamId = ids[i].streamId;
        data.taskId = ids[i].taskId;
        data.contextId = ids[i].contextId;
        data.batchId = ids[i].batchId;
        data.timestamp = times[i].first;
        data.duration = times[i].second;
        data.hostType = "KERNEL_AICORE";
        res.push_back(data);
    }
    return res;
}
static std::vector<TaskInfoData> GeneratePureCompTasksByDevice(uint16_t deviceId)
{
    std::vector<TaskInfoData> res;
    std::vector<TaskId> ids = {{2, 0, 0, 0}, {2, 0, 1, 0}, {2, 0, 2, 0}, {2, 0, 3, 0}, {2, 0, 4, 0}, {2, 0, 5, 0}};
    for (auto &id : ids) {
        TaskInfoData data;
        data.deviceId = deviceId;
        data.streamId = id.streamId;
        data.taskId = id.taskId;
        data.contextId = id.contextId;
        data.batchId = id.batchId;
        res.push_back(data);
    }
    return res;
}
static std::vector<TaskInfoData> GenerateCompTasksS1ByDevice(uint16_t deviceId)
{
    std::vector<TaskInfoData> res;
    // 注意有此处只有一个streamId=3, taskId=2
    std::vector<TaskId> ids = {{1, 0, 0, 0}, {1, 0, 1, 0}, {1, 0, 2, 0},
                               {2, 0, 0, 0}, {2, 0, 1, 0},
                               {2, 0, 3, 0}, {2, 0, 4, 0}, {2, 0, 5, 0},
                               {3, 0, 1, 0}, {3, 0, 2, 0},
                               {3, 0, 3, 0}, {3, 0, 5, 0}};
    for (auto &id : ids) {
        TaskInfoData data;
        data.deviceId = deviceId;
        data.streamId = id.streamId;
        data.taskId = id.taskId;
        data.contextId = id.contextId;
        data.batchId = id.batchId;
        res.push_back(data);
    }
    return res;
}
static std::vector<TaskInfoData> GenerateCompTasksS2ByDevice(uint16_t deviceId)
{
    std::vector<TaskInfoData> res;
    // 注意有此处只有一个streamId=3, taskId=2
    std::vector<TaskId> ids = {{2, 0, 0, 0}, {2, 0, 1, 0}, {2, 0, 3, 0}, {2, 0, 4, 0}, {2, 0, 5, 0},
                               {3, 0, 1, 0}, {3, 0, 2, 0}, {3, 0, 3, 0}, {3, 0, 5, 0}};
    for (auto &id : ids) {
        TaskInfoData data;
        data.deviceId = deviceId;
        data.streamId = id.streamId;
        data.taskId = id.taskId;
        data.contextId = id.contextId;
        data.batchId = id.batchId;
        res.push_back(data);
    }
    return res;
}
static std::vector<MC2CommInfoData> GenerateMC2CommInfoS1ByDeviceId(uint16_t deviceId)
{
    uint16_t kfcStream = 1;
    MC2CommInfoData data;
    data.aiCpuKfcStreamId = kfcStream;
    return {data};
}
static std::vector<CommunicationOpData> GenerateCommOpDataS1ByDevice(uint16_t deviceId)
{
    std::vector<std::pair<uint64_t, uint64_t>> times{{4, 8}, {9, 13}, {21, 22}, {26, 39},
                                                     {5, 7}, {14, 17}, {20, 24}, {25, 27}, {38, 41}};
    std::vector<CommunicationOpData> res;
    for (auto &time : times) {
        CommunicationOpData data;
        data.timestamp = time.first;
        data.end = time.second;
        data.deviceId = deviceId;
        res.emplace_back(data);
    }
    return res;
}
static std::vector<KfcOpData> GenerateKfcOpDataS1ByDevice(uint16_t deviceId)
{
    std::vector<std::pair<uint64_t, uint64_t>> times{{6, 9}, {20, 22}, {32, 34}};
    std::vector<KfcOpData> res;
    for (auto &time : times) {
        KfcOpData data;
        data.timestamp = time.first;
        data.end = time.second;
        data.deviceId = deviceId;
        res.emplace_back(data);
    }
    return res;
}
TEST_F(OverlapAnalysisAssemblerUTest, UnionOneSetShouldReturnContinuousSections)
{
    std::vector<TimeDuration> originData1 = {{1, 3}, {2, 6}, {8, 10}, {15, 18}};
    std::vector<TimeDuration> originData2 = {{1, 8}, {2, 6}, {9, 10}, {9, 13}};
    std::vector<TimeDuration> expectData1 = {{1, 6}, {8, 10}, {15, 18}};
    std::vector<TimeDuration> expectData2 = {{1, 8}, {9, 13}};
    auto vec1 = OverlapAnalysisAssembler::UnionOneSet(originData1);
    EXPECT_EQ(expectData1.size(), vec1.size());
    for (uint32_t i = 0; i < expectData1.size(); i++) {
        EXPECT_EQ(expectData1[i].start, vec1[i].start);
        EXPECT_EQ(expectData1[i].end, vec1[i].end);
    }
    auto vec2 = OverlapAnalysisAssembler::UnionOneSet(originData2);
    EXPECT_EQ(expectData2.size(), vec2.size());
    for (uint32_t i = 0; i < expectData2.size(); i++) {
        EXPECT_EQ(expectData2[i].start, vec2[i].start);
        EXPECT_EQ(expectData2[i].end, vec2[i].end);
    }
}
TEST_F(OverlapAnalysisAssemblerUTest, UnionTwoSetShouldReturnMergedContinuousSections)
{
    std::vector<TimeDuration> vecA = {{0, 3}, {7, 8}, {9, 15}, {21, 25}};
    std::vector<TimeDuration> vecB = {{1, 2}, {5, 9}, {9, 10}, {17, 22}, {23, 28}};
    std::vector<TimeDuration> expectData = {{0, 3}, {5, 15}, {17, 28}};
    auto vec = OverlapAnalysisAssembler::UnionTwoSet(vecA, vecB);
    EXPECT_EQ(expectData.size(), vec.size());
    for (uint32_t i = 0; i < expectData.size(); i++) {
        EXPECT_EQ(expectData[i].start, vec[i].start);
        EXPECT_EQ(expectData[i].end, vec[i].end);
    }
}
TEST_F(OverlapAnalysisAssemblerUTest, GetDifferenceSetShouldReturnDiffSections)
{
    std::vector<TimeDuration> vecA = {{0, 3}, {7, 8}, {9, 15}, {21, 25},
                                      {31, 35}, {80, 90}, {200, 205}, {300, 305}};
    std::vector<TimeDuration> vecB = {{1, 2}, {5, 9}, {9, 10},
                                      {17, 22}, {23, 28}, {34, 40},
                                      {43, 46}, {100, 101}, {190, 205},
                                      {301, 305}};
    std::vector<TimeDuration> expectData = {{0, 1}, {2, 3}, {10, 15}, {22, 23},
                                            {31, 34}, {80, 90}, {300, 301}};
    auto vec = OverlapAnalysisAssembler::GetDifferenceSet(vecA, vecB);

    EXPECT_EQ(expectData.size(), vec.size());
    for (uint32_t i = 0; i < expectData.size(); i++) {
        EXPECT_EQ(expectData[i].start, vec[i].start);
        EXPECT_EQ(expectData[i].end, vec[i].end);
    }
}
static void CheckScenarios1Data(DataInventory &dataInventory)
{
    MOCKER_CPP(&Context::GetPidFromInfoJson).stubs().will(returnValue(10087)); // pid 10087
    OverlapAnalysisAssembler assembler;
    EXPECT_TRUE(assembler.Run(dataInventory, PROF_PATH));
    auto files = File::GetOriginData(RESULT_PATH, {"msprof"}, {});
    EXPECT_EQ(1ul, files.size());
    FileReader reader(files.back());
    std::vector<std::string> res;
    EXPECT_EQ(Analysis::ANALYSIS_OK, reader.ReadText(res));
    EXPECT_EQ(SCENARIOS1_JSON, res.back());
}
TEST_F(OverlapAnalysisAssemblerUTest, AssembleDataShouldContainCompleteDataInScenarios1)
{
    // 场景1 三类任务全覆盖
    // 设计思路：
    // 1. 计算、通信、kfc三类数据两两之间4种关系全覆盖, 部分计算算子走图模式，部分走单算子模式
    // 2. 计算类和通信类内部两两关系全覆盖
    // 3. 2个device数据一致
    // 其中 [A,B]表示三类任务中的某一类，(A,B) 表示非计算非通信非MC2的任务, {A, B}表示图模式中一个算子跑多次

    // *Ascend Hardware (device 0)*
    // S1 (AICPU KFC大通信)           [5,     10],                                 [20, 23]           [31,   35]
    // S2 (Comp)           [1,3]        [6,8]      (10,12)     [16,  19]          [20,21]        [26,    33]
    // S3 (Comp)           (1,5)         [7,9]             {15,18}              [19,   24]       {29, 32}    [34, 36]
    // *HCCL*
    // Group1                      [4,   8] [9,13]                                 [21,22]       [26,             39]
    // Group2                       [5, 7]                [14,17]                 [20,  24]     [25,  27]       [38,41]
    // Group3 Aicpu                   [6,  9],                                    [20, 22]             [32, 34]

    // 合并后：
    // Communication:                      [4,13] [14,17] [20,24] [25,41]
    // Computing:                          [1,3] [6,9] [15,24] [26,33] [34,36]
    // Communication(Not Overlapped):      [4,6] [9,13] [14,15] [25,26] [33,34] [36, 41]
    // Free:                               [3,4] [13,14] [24,25]
    std::shared_ptr<std::vector<AscendTaskData>> tasksPtr;
    std::shared_ptr<std::vector<TaskInfoData>> compTasksPtr;
    std::shared_ptr<std::vector<CommunicationOpData>> commOpsPtr;
    std::shared_ptr<std::vector<KfcOpData>> kfcOpsPtr;
    std::shared_ptr<std::vector<MC2CommInfoData>> mc2CommInfoPtr;

    std::vector<uint16_t> devices = {0, 1};
    std::vector<AscendTaskData> taskData;
    std::vector<TaskInfoData> compTaskData;
    std::vector<CommunicationOpData> commOpData;
    std::vector<KfcOpData> kfcOpData;
    std::vector<MC2CommInfoData> mc2CommInfoData;
    for (auto &id : devices) {
        auto ascendTasks = GenerateAscendTaskDataS1ByDevice(id);
        taskData.insert(taskData.end(), ascendTasks.begin(), ascendTasks.end());
        auto compTasks = GenerateCompTasksS1ByDevice(id);
        compTaskData.insert(compTaskData.end(), compTasks.begin(), compTasks.end());
        auto commOps = GenerateCommOpDataS1ByDevice(id);
        commOpData.insert(commOpData.end(), commOps.begin(), commOps.end());
        auto kfcOps = GenerateKfcOpDataS1ByDevice(id);
        kfcOpData.insert(kfcOpData.end(), kfcOps.begin(), kfcOps.end());
        auto mc2CommInfos = GenerateMC2CommInfoS1ByDeviceId(id);
        mc2CommInfoData.insert(mc2CommInfoData.end(), mc2CommInfos.begin(), mc2CommInfos.end());
    }
    MAKE_SHARED_NO_OPERATION(tasksPtr, std::vector<AscendTaskData>, taskData);
    MAKE_SHARED_NO_OPERATION(compTasksPtr, std::vector<TaskInfoData>, compTaskData);
    MAKE_SHARED_NO_OPERATION(commOpsPtr, std::vector<CommunicationOpData>, commOpData);
    MAKE_SHARED_NO_OPERATION(kfcOpsPtr, std::vector<KfcOpData>, kfcOpData);
    MAKE_SHARED_NO_OPERATION(mc2CommInfoPtr, std::vector<MC2CommInfoData>, mc2CommInfoData);
    dataInventory_.Inject(tasksPtr);
    dataInventory_.Inject(compTasksPtr);
    dataInventory_.Inject(commOpsPtr);
    dataInventory_.Inject(kfcOpsPtr);
    dataInventory_.Inject(mc2CommInfoPtr);
    CheckScenarios1Data(dataInventory_);
}
static void CheckScenarios2Data(DataInventory &dataInventory)
{
    MOCKER_CPP(&Context::GetPidFromInfoJson).stubs().will(returnValue(10087)); // pid 10087
    OverlapAnalysisAssembler assembler;
    EXPECT_TRUE(assembler.Run(dataInventory, PROF_PATH));
    auto files = File::GetOriginData(RESULT_PATH, {"msprof"}, {});
    EXPECT_EQ(1ul, files.size());
    FileReader reader(files.back());
    std::vector<std::string> res;
    EXPECT_EQ(Analysis::ANALYSIS_OK, reader.ReadText(res));
    EXPECT_EQ(SCENARIOS2_JSON, res.back());
}

TEST_F(OverlapAnalysisAssemblerUTest, AssembleDataShouldContainCompleteDataInScenarios2)
{
    // 场景2 只有通信和计算类任务
    // 设计思路：
    // 1. 计算、通信2类数据两两之间4种关系全覆盖
    // 2. 计算类和通信类内部两两关系全覆盖
    // 3. 1个device
    // 其中 [A,B]表示三类任务中的某一类，(A,B) 表示非计算非通信的任务, {A, B}表示图模式中一个算子跑多次

    // *Ascend Hardware (device 0)*
    // S2 (Comp)           [1,3]        [6,8]      (10,12)     [16,  19]          [20,21]        [26,    33]
    // S3 (Comp)           (1,5)         [7,9]             {15,18}              [19,   24]       {29, 32}    [34, 36]
    // *HCCL*
    // Group1                      [4,   8] [9,13]                                 [21,22]       [26,             39]
    // Group2                       [5, 7]                [14,17]                 [20,  24]     [25,  27]       [38,41]

    // 合并后：
    // Communication:                      [4,8] [9,13] [14,17] [20,24] [25,41]
    // Computing:                          [1,3] [6,9] [15,24] [26,33] [34,36]
    // Communication(Not Overlapped):      [4,6] [9,13] [14,15] [25,26] [33,34] [36, 41]
    // Free:                               [3,4] [13,14] [24,25]
    std::shared_ptr<std::vector<AscendTaskData>> tasksPtr;
    std::shared_ptr<std::vector<TaskInfoData>> compTasksPtr;
    std::shared_ptr<std::vector<CommunicationOpData>> commOpsPtr;

    std::vector<uint16_t> devices = {0};
    std::vector<AscendTaskData> taskData;
    std::vector<TaskInfoData> compTaskData;
    std::vector<CommunicationOpData> commOpData;
    for (auto &id : devices) {
        auto ascendTasks = GenerateAscendTaskDataS2ByDevice(id);
        taskData.insert(taskData.end(), ascendTasks.begin(), ascendTasks.end());
        auto compTasks = GenerateCompTasksS2ByDevice(id);
        compTaskData.insert(compTaskData.end(), compTasks.begin(), compTasks.end());
        auto commOps = GenerateCommOpDataS1ByDevice(id);
        commOpData.insert(commOpData.end(), commOps.begin(), commOps.end());
    }
    MAKE_SHARED_NO_OPERATION(tasksPtr, std::vector<AscendTaskData>, taskData);
    MAKE_SHARED_NO_OPERATION(compTasksPtr, std::vector<TaskInfoData>, compTaskData);
    MAKE_SHARED_NO_OPERATION(commOpsPtr, std::vector<CommunicationOpData>, commOpData);
    dataInventory_.Inject(tasksPtr);
    dataInventory_.Inject(compTasksPtr);
    dataInventory_.Inject(commOpsPtr);
    CheckScenarios2Data(dataInventory_);
}

static void CheckScenarios3Data(DataInventory &dataInventory)
{
    MOCKER_CPP(&Context::GetPidFromInfoJson).stubs().will(returnValue(10087)); // pid 10087
    OverlapAnalysisAssembler assembler;
    EXPECT_TRUE(assembler.Run(dataInventory, PROF_PATH));
    auto files = File::GetOriginData(RESULT_PATH, {"msprof"}, {});
    EXPECT_EQ(1ul, files.size());
    FileReader reader(files.back());
    std::vector<std::string> res;
    EXPECT_EQ(Analysis::ANALYSIS_OK, reader.ReadText(res));
    EXPECT_EQ(SCENARIOS3_JSON, res.back());
}

TEST_F(OverlapAnalysisAssemblerUTest, AssembleDataShouldContainCompleteDataInScenarios3)
{
    // 场景3 只有计算类任务
    // 其中 [A,B]表示三类任务中的某一类，(A,B) 表示非计算非通信的任务, {A, B}表示图模式中一个算子跑多次

    // *Ascend Hardware (device 0)*
    // S2 (Comp)           [1,3]        [6,8]      (10,12)     [16,  19]          [20,21]        [26,    33]
    // S3 (Comp)           (1,5)         [7,9]             {15,18}              [19,   24]       {29, 32}    [34, 36]

    // 合并后：
    // Communication:
    // Computing:                          [1,3] [6,9] [15,24] [26,33] [34,36]
    // Communication(Not Overlapped):
    // Free:                               [3,6] [9,15] [24,26] [33,34]
    // 通信：
    std::shared_ptr<std::vector<AscendTaskData>> tasksPtr;
    std::shared_ptr<std::vector<TaskInfoData>> compTasksPtr;

    std::vector<uint16_t> devices = {0};
    std::vector<AscendTaskData> taskData;
    std::vector<TaskInfoData> compTaskData;
    for (auto &id : devices) {
        auto ascendTasks = GenerateAscendTaskDataS2ByDevice(id);
        taskData.insert(taskData.end(), ascendTasks.begin(), ascendTasks.end());
        auto compTasks = GenerateCompTasksS2ByDevice(id);
        compTaskData.insert(compTaskData.end(), compTasks.begin(), compTasks.end());
    }
    MAKE_SHARED_NO_OPERATION(tasksPtr, std::vector<AscendTaskData>, taskData);
    MAKE_SHARED_NO_OPERATION(compTasksPtr, std::vector<TaskInfoData>, compTaskData);
    dataInventory_.Inject(tasksPtr);
    dataInventory_.Inject(compTasksPtr);
    CheckScenarios3Data(dataInventory_);
}

static void CheckScenarios4Data(DataInventory &dataInventory)
{
    MOCKER_CPP(&Context::GetPidFromInfoJson).stubs().will(returnValue(10087)); // pid 10087
    OverlapAnalysisAssembler assembler;
    EXPECT_TRUE(assembler.Run(dataInventory, PROF_PATH));
    auto files = File::GetOriginData(RESULT_PATH, {"msprof"}, {});
    EXPECT_EQ(1ul, files.size());
    FileReader reader(files.back());
    std::vector<std::string> res;
    EXPECT_EQ(Analysis::ANALYSIS_OK, reader.ReadText(res));
    EXPECT_EQ(SCENARIOS4_JSON, res.back());
}

TEST_F(OverlapAnalysisAssemblerUTest, AssembleDataShouldContainCompleteDataInScenarios4)
{
    // 场景4 只有通信类任务
    // 其中 [A,B]表示三类任务中的某一类，(A,B) 表示非计算非通信的任务, {A, B}表示图模式中一个算子跑多次

    // *HCCL*
    // Group1                      [4,   8] [9,13]                                 [21,22]       [26,             39]
    // Group2                       [5, 7]                [14,17]                 [20,  24]     [25,  27]       [38,41]

    // 合并后：
    // Communication:                      [4,8] [9,13] [14,17] [20,24] [25,41]
    // Computing:
    // Communication(Not Overlapped):      [4,8] [9,13] [14,17] [20,24] [25,41]
    // Free:                               [8,9] [13,14] [17,20] [24,25]
    std::shared_ptr<std::vector<AscendTaskData>> tasksPtr;
    std::shared_ptr<std::vector<CommunicationOpData>> commOpsPtr;

    std::vector<uint16_t> devices = {0};
    AscendTaskData data;
    uint64_t start = 5;
    double dur = 1.0;
    data.deviceId = 0;
    data.timestamp = start;
    data.duration = dur;
    // 不可能存在一个 任何task都没有的场景
    std::vector<AscendTaskData> taskData = {data};
    std::vector<CommunicationOpData> commOpData;
    for (auto &id : devices) {
        auto commOps = GenerateCommOpDataS1ByDevice(id);
        commOpData.insert(commOpData.end(), commOps.begin(), commOps.end());
    }
    MAKE_SHARED_NO_OPERATION(tasksPtr, std::vector<AscendTaskData>, taskData);
    MAKE_SHARED_NO_OPERATION(commOpsPtr, std::vector<CommunicationOpData>, commOpData);
    dataInventory_.Inject(tasksPtr);
    dataInventory_.Inject(commOpsPtr);
    CheckScenarios4Data(dataInventory_);
}

static void CheckScenarios5Data(DataInventory &dataInventory)
{
    MOCKER_CPP(&Context::GetPidFromInfoJson).stubs().will(returnValue(10087)); // pid 10087
    OverlapAnalysisAssembler assembler;
    EXPECT_TRUE(assembler.Run(dataInventory, PROF_PATH));
    auto files = File::GetOriginData(RESULT_PATH, {"msprof"}, {});
    EXPECT_EQ(1ul, files.size());
    FileReader reader(files.back());
    std::vector<std::string> res;
    EXPECT_EQ(Analysis::ANALYSIS_OK, reader.ReadText(res));
    EXPECT_EQ(SCENARIOS5_JSON, res.back());
}

static void CheckScenarios6Data(DataInventory &dataInventory)
{
    MOCKER_CPP(&Context::GetPidFromInfoJson).stubs().will(returnValue(10087)); // pid 10087
    OverlapAnalysisAssembler assembler;
    EXPECT_TRUE(assembler.Run(dataInventory, PROF_PATH));
    auto files = File::GetOriginData(RESULT_PATH, {"msprof"}, {});
    EXPECT_EQ(1ul, files.size());
    FileReader reader(files.back());
    std::vector<std::string> res;
    EXPECT_EQ(Analysis::ANALYSIS_OK, reader.ReadText(res));
    EXPECT_EQ(SCENARIOS6_JSON, res.back());
}

TEST_F(OverlapAnalysisAssemblerUTest, AssembleDataShouldContainCompleteDataInScenarios5)
{
    // 场景5 只有调度类任务(无taskInfo类) 对齐python逻辑 overlap生成free
    // 但是会有meta头,不影响呈现
    // 其中 [A,B]表示三类任务中的某一类，(A,B) 表示非计算非通信的任务, {A, B}表示图模式中一个算子跑多次

    // *Ascend Hardware (device 0)*
    // S2 (Comp)           (1,3)        (6,8)      (10,12)     (16,  19)          (20,21)        (26,    33)
    // S3 (Comp)           (1,5)         (7,9)             (15,18)              (19,   24)       (29, 32)    (34, 36)

    // 合并后：
    std::shared_ptr<std::vector<AscendTaskData>> tasksPtr;

    std::vector<uint16_t> devices = {0};
    std::vector<AscendTaskData> taskData;
    for (auto &id : devices) {
        auto ascendTasks = GenerateAscendTaskDataS2ByDevice(id);
        taskData.insert(taskData.end(), ascendTasks.begin(), ascendTasks.end());
    }
    MAKE_SHARED_NO_OPERATION(tasksPtr, std::vector<AscendTaskData>, taskData);
    dataInventory_.Inject(tasksPtr);
    CheckScenarios5Data(dataInventory_);
}

TEST_F(OverlapAnalysisAssemblerUTest, AssembleDataShouldContainCompleteDataInScenarios6)
{
    // 场景5 Ascend只有计算类任务 overlap生成free和compute
    // 但是会有meta头,不影响呈现
    // 其中 [A,B]表示三类任务中的某一类，(A,B) 表示非计算非通信的任务, {A, B}表示图模式中一个算子跑多次

    // *Ascend Hardware (device 0)*
    // S2 (Comp)    (5,7)    (10,12)    (14,16)    (20,23)    (24,25)    (30,37)

    std::shared_ptr<std::vector<AscendTaskData>> tasksPtr;
    std::shared_ptr<std::vector<TaskInfoData>> compTasksPtr;
    std::vector<AscendTaskData> taskData;
    std::vector<TaskInfoData> compTaskData;

    std::vector<uint16_t> devices = {0};
    for (auto &id : devices) {
        auto ascendTasks = GeneratePureCompAscendTaskDataByDevice(id);
        taskData.insert(taskData.end(), ascendTasks.begin(), ascendTasks.end());
        auto compTasks = GeneratePureCompTasksByDevice(id);
        compTaskData.insert(compTaskData.end(), compTasks.begin(), compTasks.end());
    }
    MAKE_SHARED_NO_OPERATION(tasksPtr, std::vector<AscendTaskData>, taskData);
    MAKE_SHARED_NO_OPERATION(compTasksPtr, std::vector<TaskInfoData>, compTaskData);

    dataInventory_.Inject(tasksPtr);
    dataInventory_.Inject(compTasksPtr);

    CheckScenarios6Data(dataInventory_);
}
