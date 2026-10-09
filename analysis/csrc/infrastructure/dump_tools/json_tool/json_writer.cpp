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
#include "analysis/csrc/infrastructure/dump_tools/json_tool/include/json_writer.h"

#include "analysis/csrc/infrastructure/utils/utils.h"

using namespace rapidjson;

namespace Analysis
{

namespace Infra
{

JsonWriter::JsonWriter()
    : stream_(Utils::MAKE_UNIQUE_PTR<StringBuffer>()),
      writer_(Utils::MAKE_UNIQUE_PTR<PrettyWriter<StringBuffer>>(*stream_))
{
}

JsonWriter::~JsonWriter() {}

const char* JsonWriter::GetString() const { return stream_->GetString(); }

JsonWriter& JsonWriter::StartObject()
{
    writer_->StartObject();
    return *this;
}

JsonWriter& JsonWriter::EndObject()
{
    writer_->EndObject();
    return *this;
}

JsonWriter& JsonWriter::Member(const char* name)
{
    writer_->String(name, static_cast<SizeType>(strlen(name)));
    return *this;
}

JsonWriter& JsonWriter::StartArray()
{
    writer_->StartArray();
    return *this;
}

JsonWriter& JsonWriter::EndArray()
{
    writer_->EndArray();
    return *this;
}

JsonWriter& JsonWriter::operator<<(const bool& b)
{
    writer_->Bool(b);
    return *this;
}

JsonWriter& JsonWriter::operator<<(const unsigned& u)
{
    writer_->Uint(u);
    return *this;
}

JsonWriter& JsonWriter::operator<<(const int& i)
{
    writer_->Int(i);
    return *this;
}

JsonWriter& JsonWriter::operator<<(const double& d)
{
    writer_->Double(d);
    return *this;
}

JsonWriter& JsonWriter::operator<<(const uint64_t& d)
{
    writer_->Uint64(d);
    return *this;
}

JsonWriter& JsonWriter::operator<<(const int64_t& d)
{
    writer_->Int64(d);
    return *this;
}

JsonWriter& JsonWriter::operator<<(const std::string& s)
{
    writer_->String(s.c_str(), static_cast<SizeType>(s.size()));
    return *this;
}

JsonWriter& JsonWriter::SetNull()
{
    writer_->Null();
    return *this;
}

}  // namespace Infra

}  // namespace Analysis
