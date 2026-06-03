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

#ifndef ANALYSIS_INFRASTRUCTURE_DUMP_TOOLS_JSON_TOOL_INCLUDE_JSON_WRITER_H
#define ANALYSIS_INFRASTRUCTURE_DUMP_TOOLS_JSON_TOOL_INCLUDE_JSON_WRITER_H

#include <cstddef>
#include <string>
#include <memory>
#include <stdint.h>
#include "rapidjson/stringbuffer.h"
#include "rapidjson/prettywriter.h"

namespace Analysis {

namespace Infra {

class JsonWriter final {
public:
    JsonWriter();

    ~JsonWriter();

    /// Obtains the serialized JSON string.
    const char *GetString() const;
    size_t GetSize() const {return stream_->GetSize(); }

    JsonWriter &StartObject();

    JsonWriter &operator[](const char *name)
    {
        this->Member(name);
        return *this;
    };

    JsonWriter &Member(const char *name);

    JsonWriter &EndObject();

    JsonWriter &StartArray();

    JsonWriter &EndArray();

    JsonWriter &operator<<(const bool &b);

    JsonWriter &operator<<(const unsigned &u);

    JsonWriter &operator<<(const int &i);

    JsonWriter &operator<<(const double &d);

    JsonWriter &operator<<(const uint64_t &d);

    JsonWriter &operator<<(const int64_t &d);

    JsonWriter &operator<<(const std::string &s);

    JsonWriter &SetNull();

private:
    JsonWriter(const JsonWriter &);

    JsonWriter &operator=(const JsonWriter &);

    // PIMPL idiom
    std::unique_ptr<rapidjson::StringBuffer> stream_;      // Stream buffer.
    std::unique_ptr<rapidjson::Writer<rapidjson::StringBuffer>> writer_;      // JSON writer.
};

}

}

#endif