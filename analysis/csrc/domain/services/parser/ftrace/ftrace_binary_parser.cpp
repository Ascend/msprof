/* -------------------------------------------------------------------------
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
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
 * MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------*/
#include "analysis/csrc/domain/services/parser/ftrace/include/ftrace_binary_parser.h"

#include <dirent.h>
#include <sys/stat.h>

#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <limits>
#include <set>
#include <utility>

#include "analysis/csrc/infrastructure/utils/utils.h"
#include "nlohmann/json.hpp"

namespace Analysis
{
namespace Domain
{
namespace
{
constexpr char FTRACE_SUB_DIR[] = "host/data/ftrace";  // prof/host/data/ftrace
constexpr char FTRACE_SUB_DIR_FLAT[] = "data/ftrace";  // fallback for a flat layout
constexpr char CPU_FILE_PREFIX[] = "trace.cpu";
constexpr char CPU_FILE_SUFFIX[] = ".bin";
constexpr char FOOTER_MAGIC[] = "FTBC0001";
constexpr size_t FOOTER_EVENTS_OFFSET = 24;
constexpr size_t CPU_FILE_PREFIX_LEN = sizeof(CPU_FILE_PREFIX) - 1;
constexpr size_t CPU_FILE_SUFFIX_LEN = sizeof(CPU_FILE_SUFFIX) - 1;
static_assert(FtraceBinaryParser::FOOTER_SIZE >= FOOTER_EVENTS_OFFSET + sizeof(uint64_t), "Invalid footer layout");

constexpr uint32_t RB_TYPE_MASK = 0x1f;
constexpr uint32_t RB_TYPE_MAX_DATA = 28;
constexpr size_t RB_EVENT_HDR_SIZE = 4;  // sizeof(u32) event header, not counted by type_len
constexpr uint32_t RB_TYPE_PADDING = 29;
constexpr uint32_t RB_TYPE_TIME_EXTEND = 30;
constexpr uint32_t RB_TYPE_TIME_STAMP = 31;
constexpr uint32_t RB_TIME_DELTA_SHIFT = 5;
constexpr uint32_t RB_TIME_LOW_BITS = 27;
constexpr uint64_t RB_TIMESTAMP_WRAP = 1ULL << (RB_TIME_LOW_BITS + 32);
constexpr uint64_t RB_TIMESTAMP_HIGH_MASK = ~(RB_TIMESTAMP_WRAP - 1);
constexpr size_t RB_TIME_RECORD_SIZE = 2 * sizeof(uint32_t);
constexpr uint64_t RB_MISSED_EVENTS = 1ULL << 31;
constexpr uint64_t RB_MISSED_STORED = 1ULL << 30;
constexpr uint64_t NS_PER_US = 1000;
constexpr uint64_t MAX_DB_TIMESTAMP = static_cast<uint64_t>(std::numeric_limits<int64_t>::max());

constexpr size_t DEFAULT_PAGE_SIZE = 4096;
constexpr size_t MAX_PAGE_SIZE = 1024 * 1024;
constexpr int32_t MAX_FIELD_SIZE = 64;

bool IsDir(const std::string &path)
{
    struct stat st
    {
    };
    return (stat(path.c_str(), &st) == 0) && S_ISDIR(st.st_mode);
}

using Json = nlohmann::json;

Json ReadJson(const std::string &path)
{
    std::ifstream stream(path);
    return stream.is_open() ? Json::parse(stream, nullptr, false) : Json();
}

bool ReadInteger(const Json &object, const char *key, int64_t &value)
{
    const auto item = object.find(key);
    if (item == object.end())
    {
        return false;
    }
    if (item->is_number_unsigned())
    {
        const auto number = item->get<uint64_t>();
        if (number > MAX_DB_TIMESTAMP)
        {
            return false;
        }
        value = static_cast<int64_t>(number);
        return true;
    }
    if (item->is_number_integer())
    {
        value = item->get<int64_t>();
        return true;
    }
    if (!item->is_string())
    {
        return false;
    }
    const auto &text = item->get_ref<const std::string &>();
    const auto start = text.find_first_not_of(" \t\r\n");
    if (start == std::string::npos)
    {
        return false;
    }
    const char *begin = text.c_str() + start;
    char *end = nullptr;
    errno = 0;
    const auto number = std::strtoll(begin, &end, 10);
    if (end == begin || errno == ERANGE ||
        text.find_first_not_of(" \t\r\n", static_cast<size_t>(end - text.c_str())) != std::string::npos)
    {
        return false;
    }
    value = static_cast<int64_t>(number);
    return true;
}

std::string ReadString(const Json &object, const char *key)
{
    const auto item = object.find(key);
    return item != object.end() && item->is_string() ? item->get<std::string>() : std::string();
}

bool ParseCpuId(const std::string &name, uint32_t &cpuId)
{
    if (name.size() <= CPU_FILE_PREFIX_LEN + CPU_FILE_SUFFIX_LEN || name.rfind(CPU_FILE_PREFIX, 0) != 0 ||
        name.compare(name.size() - CPU_FILE_SUFFIX_LEN, CPU_FILE_SUFFIX_LEN, CPU_FILE_SUFFIX) != 0)
    {
        return false;
    }
    const auto end = name.size() - CPU_FILE_SUFFIX_LEN;
    uint32_t id = 0;
    for (size_t index = CPU_FILE_PREFIX_LEN; index < end; ++index)
    {
        if (name[index] < '0' || name[index] > '9')
        {
            return false;
        }
        const auto digit = static_cast<uint32_t>(name[index] - '0');
        if (id > (std::numeric_limits<uint32_t>::max() - digit) / 10)
        {
            return false;
        }
        id = id * 10 + digit;
    }
    cpuId = id;
    return true;
}

GenericFieldKind KindOfCType(const std::string &cType, int64_t size)
{
    // "unsigned char" with size 1 is a scalar flag, "char comm[16]" with size 16 is a string
    if (cType.find("char") != std::string::npos)
    {
        if (size > 1)
        {
            return GenericFieldKind::STRING_VALUE;
        }
        return (cType.find("unsigned") != std::string::npos) ? GenericFieldKind::UINT_VALUE
                                                             : GenericFieldKind::INT_VALUE;
    }
    if (cType.find("string") != std::string::npos)
    {
        return GenericFieldKind::STRING_VALUE;
    }

    if (cType.find("double") != std::string::npos)
    {
        return GenericFieldKind::DOUBLE_VALUE;
    }
    if (cType.find("float") != std::string::npos)
    {
        return GenericFieldKind::FLOAT_VALUE;
    }
    if (cType.find("unsigned") != std::string::npos)
    {
        return GenericFieldKind::UINT_VALUE;
    }
    return GenericFieldKind::INT_VALUE;
}

size_t ClampPageSize(int64_t raw)
{
    if (raw < static_cast<int64_t>(DEFAULT_PAGE_SIZE) || raw > static_cast<int64_t>(MAX_PAGE_SIZE))
    {
        WARN("ftrace meta pageSize % is out of range, fall back to %.", raw, DEFAULT_PAGE_SIZE);
        return DEFAULT_PAGE_SIZE;
    }
    return static_cast<size_t>(raw);
}
// <base>/host/data/ftrace, then the flat <base>/data/ftrace written by older collectors.
bool ResolveFtraceDir(const std::string &base, std::string &ftraceDir, std::string &anchorPath)
{
    std::string candidate = Utils::File::PathJoin({base, FTRACE_SUB_DIR});
    const bool standardLayout = IsDir(candidate);
    if (!standardLayout)
    {
        candidate = Utils::File::PathJoin({base, FTRACE_SUB_DIR_FLAT});
    }
    if (!IsDir(candidate))
    {
        return false;
    }
    ftraceDir = candidate;
    anchorPath = standardLayout ? Utils::File::PathJoin({base, "host", "start_info"})
                                : Utils::File::PathJoin({base, "start_info"});
    return true;
}

}  // namespace

bool FtraceBinaryParser::LocateFiles(const std::string &profPath, std::vector<FtraceCpuFile> &cpuFiles,
                                     std::string &metaPath, std::string &threadMapPath, std::string &ftraceDir,
                                     std::string &anchorPath)
{
    if (!ResolveFtraceDir(profPath, ftraceDir, anchorPath))
    {
        // The caller may have handed us a parent directory that holds several PROF_* datasets.
        // Take the only child that carries ftrace; when more than one does, merging them into a
        // single database would mix two runs, so refuse instead of guessing.
        std::vector<std::string> owners;
        DIR *parent = opendir(profPath.c_str());
        if (parent == nullptr)
        {
            return false;
        }
        struct dirent *entry = nullptr;
        while ((entry = readdir(parent)) != nullptr)
        {
            const std::string name = entry->d_name;
            if (name == "." || name == ".." || name.rfind(".", 0) == 0)
            {
                continue;
            }
            const std::string child = Utils::File::PathJoin({profPath, name});
            if (!IsDir(child))
            {
                continue;
            }
            std::string ignoredDir;
            std::string ignoredAnchor;
            if (ResolveFtraceDir(child, ignoredDir, ignoredAnchor))
            {
                owners.emplace_back(child);
            }
        }
        closedir(parent);
        if (owners.empty())
        {
            return false;
        }
        if (owners.size() > 1)
        {
            WARN("ftrace artifacts found in % datasets below %, refuse to merge them; pass one dataset.", owners.size(),
                 profPath);
            return false;
        }
        const auto &datasetBase = owners.front();
        if (!ResolveFtraceDir(datasetBase, ftraceDir, anchorPath))
        {
            return false;
        }
        INFO("ftrace artifacts located in % (dataset below %).", datasetBase, profPath);
    }
    metaPath = Utils::File::PathJoin({ftraceDir, "trace_event_meta.json"});
    threadMapPath = Utils::File::PathJoin({ftraceDir, "thread_map.json"});
    DIR *dir = opendir(ftraceDir.c_str());
    if (dir == nullptr)
    {
        return false;
    }
    struct dirent *entry = nullptr;
    while ((entry = readdir(dir)) != nullptr)
    {
        const std::string name = entry->d_name;
        uint32_t cpuId = 0;
        if (ParseCpuId(name, cpuId))
        {
            cpuFiles.push_back({Utils::File::PathJoin({ftraceDir, name}), cpuId});
        }
        else if (name.rfind(CPU_FILE_PREFIX, 0) == 0)
        {
            WARN("Invalid ftrace CPU file name %, skip.", name);
        }
    }
    closedir(dir);
    std::sort(cpuFiles.begin(), cpuFiles.end(), [](const FtraceCpuFile &left, const FtraceCpuFile &right)
              { return left.cpuId == right.cpuId ? left.path < right.path : left.cpuId < right.cpuId; });
    return !cpuFiles.empty();
}

bool FtraceBinaryParser::ParseMeta(const std::string &metaPath, GenericEventMeta &meta, size_t &pageSize)
{
    const auto root = ReadJson(metaPath);
    const auto types = root.find("types");
    if (types == root.end() || !types->is_array())
    {
        return false;
    }
    meta = GenericEventMeta{};
    int64_t number = 0;
    pageSize = ReadInteger(root, "pageSize", number) ? ClampPageSize(number) : DEFAULT_PAGE_SIZE;
    meta.pageSize = pageSize;
    if (ReadInteger(root, "pageDataOffset", number) && number >= static_cast<int64_t>(PAGE_DATA_OFFSET_MIN) &&
        number <= static_cast<int64_t>(PAGE_DATA_OFFSET_MAX))
    {
        meta.pageDataOffset = static_cast<size_t>(number);
    }

    const auto ringbuf = root.find("ringbuf");
    const auto &layout = ringbuf != root.end() && ringbuf->is_object() ? *ringbuf : root;
    const auto readTimeLength = [&layout](const char *key, uint32_t &length)
    {
        int64_t bytes = 0;
        if (ReadInteger(layout, key, bytes) && bytes >= static_cast<int64_t>(RB_TIME_RECORD_SIZE) &&
            bytes <= MAX_FIELD_SIZE && bytes % static_cast<int64_t>(RB_EVENT_HDR_SIZE) == 0)
        {
            length = static_cast<uint32_t>(bytes);
        }
    };
    readTimeLength("timeExtendLen", meta.timeExtendLen);
    readTimeLength("timeStampLen", meta.timeStampLen);

    for (const auto &type : *types)
    {
        GenericEventTypeDef def;
        def.system = ReadString(type, "system");
        def.name = ReadString(type, "name");
        const auto fields = type.find("fields");
        if (def.name.empty() || fields == type.end() || !fields->is_array() || !ReadInteger(type, "kernelId", number) ||
            number < 0 || number > std::numeric_limits<uint16_t>::max())
        {
            WARN("Invalid ftrace event type %, skip.", def.name);
            continue;
        }
        def.kernelId = static_cast<uint16_t>(number);
        def.formatHash = ReadString(type, "formatHash");
        def.formatText = ReadString(type, "format");
        for (const auto &entry : *fields)
        {
            GenericEventTypeField field;
            field.name = ReadString(entry, "name");
            int64_t offset = -1;
            int64_t size = 0;
            if (!ReadInteger(entry, "offset", offset) || !ReadInteger(entry, "size", size) || offset < 0 ||
                offset > std::numeric_limits<int32_t>::max() || size <= 0 || size > MAX_FIELD_SIZE)
            {
                WARN("ftrace meta dropped field % of type %: offset=%, size=%", field.name, def.name, offset, size);
                continue;
            }
            field.offset = static_cast<int32_t>(offset);
            field.size = static_cast<int32_t>(size);
            const auto cType = ReadString(entry, "cType");
            field.isDataLoc = cType.find("__data_loc") != std::string::npos;
            field.isRelLoc = cType.find("__rel_loc") != std::string::npos;
            field.kind = field.isDataLoc || field.isRelLoc ? GenericFieldKind::STRING_VALUE : KindOfCType(cType, size);
            def.fields.emplace_back(std::move(field));
        }
        if (def.fields.empty())
        {
            continue;
        }
        if (!meta.typeIndex.emplace(def.kernelId, meta.types.size()).second)
        {
            WARN("Duplicate ftrace kernel event id %, skip.", def.kernelId);
            continue;
        }
        def.typeId = static_cast<uint32_t>(meta.types.size() + 1);
        meta.types.emplace_back(std::move(def));
    }
    return !meta.types.empty();
}

bool FtraceBinaryParser::ParseThreadMap(const std::string &threadMapPath,
                                        std::unordered_map<uint32_t, uint32_t> &tgidOf)
{
    const auto root = ReadJson(threadMapPath);
    const auto threads = root.find("threads");
    const auto &entries = threads != root.end() ? *threads : root;
    if (!entries.is_array())
    {
        return false;
    }
    for (const auto &entry : entries)
    {
        int64_t tid = 0;
        int64_t tgid = 0;
        if (ReadInteger(entry, "tid", tid) && ReadInteger(entry, "tgid", tgid) && tid >= 0 &&
            tid <= std::numeric_limits<uint32_t>::max() && tgid >= 0 && tgid <= std::numeric_limits<uint32_t>::max())
        {
            tgidOf[static_cast<uint32_t>(tid)] = static_cast<uint32_t>(tgid);
        }
    }
    return !tgidOf.empty();
}

bool FtraceBinaryParser::ParseEpochAnchor(const std::string &anchorPath, uint64_t &anchorEpochNs,
                                          uint64_t &anchorMonoRawNs)
{
    const auto root = ReadJson(anchorPath);
    int64_t monoRaw = 0;
    int64_t collectionBegin = 0;
    if (!ReadInteger(root, "clockMonotonicRaw", monoRaw) ||
        !ReadInteger(root, "collectionTimeBegin", collectionBegin) || monoRaw <= 0 || collectionBegin <= 0 ||
        static_cast<uint64_t>(collectionBegin) > MAX_DB_TIMESTAMP / NS_PER_US)
    {
        return false;
    }
    // The host start_info stores collectionTimeBegin in microseconds, and the monotonic clock in ns.
    const auto epochNs = static_cast<uint64_t>(collectionBegin) * NS_PER_US;
    if (epochNs < static_cast<uint64_t>(monoRaw))
    {
        return false;
    }
    anchorEpochNs = epochNs;
    anchorMonoRawNs = static_cast<uint64_t>(monoRaw);
    INFO("Using ftrace host time anchor %.", anchorPath);
    return true;
}

namespace
{
void DecodeEvent(const uint8_t *payload, size_t payloadLength, const GenericEventTypeDef &typeDef,
                 GenericEventData &event, std::set<std::pair<GenericFieldKind, size_t>> &unsupported)
{
    for (const auto &field : typeDef.fields)
    {
        GenericEventFieldValue value;
        value.fieldIdx = static_cast<uint32_t>(event.values.size());
        value.kind = field.kind;
        const size_t offset = static_cast<size_t>(field.offset);
        const size_t size = static_cast<size_t>(field.size);
        if (field.offset < 0 || size == 0 || offset > payloadLength || size > payloadLength - offset)
        {
            event.flags |= FTRACE_EVENT_FLAG_FIELD_FAILED;
        }
        else if (field.kind == GenericFieldKind::STRING_VALUE)
        {
            size_t start = offset;
            size_t length = size;
            if (field.isDataLoc || field.isRelLoc)
            {
                uint32_t locator = 0;
                if (size == sizeof(locator))
                {
                    memcpy(&locator, payload + offset, sizeof(locator));
                }
                start = (field.isRelLoc ? offset + sizeof(locator) : 0) + (locator & 0xFFFFU);
                length = locator >> 16;
            }
            if (length == 0 || start > payloadLength || length > payloadLength - start)
            {
                event.flags |= FTRACE_EVENT_FLAG_FIELD_FAILED;
            }
            else
            {
                const char *begin = reinterpret_cast<const char *>(payload + start);
                const char *end = std::find(begin, begin + length, '\0');
                value.strVal.assign(begin, end);
            }
        }
        else if (field.kind == GenericFieldKind::FLOAT_VALUE && size == sizeof(float))
        {
            float real = 0.0F;
            memcpy(&real, payload + offset, sizeof(real));
            value.realVal = static_cast<double>(real);
        }
        else if (field.kind == GenericFieldKind::DOUBLE_VALUE && size == sizeof(double))
        {
            memcpy(&value.realVal, payload + offset, sizeof(double));
        }
        else if ((field.kind == GenericFieldKind::UINT_VALUE || field.kind == GenericFieldKind::INT_VALUE) &&
                 size <= sizeof(uint64_t))
        {
            uint64_t raw = 0;
            memcpy(&raw, payload + offset, size);
            if (field.kind == GenericFieldKind::UINT_VALUE)
            {
                value.uintVal = raw;
            }
            else
            {
                const size_t bits = size * 8;
                const uint64_t signBit = 1ULL << (bits - 1);
                const uint64_t mask = std::numeric_limits<uint64_t>::max() >> (64 - bits);
                value.intVal =
                    (raw & signBit) == 0 ? static_cast<int64_t>(raw) : -1 - static_cast<int64_t>(~raw & mask);
            }
        }
        else
        {
            event.flags |= FTRACE_EVENT_FLAG_FIELD_FAILED;
            unsupported.emplace(field.kind, size);
        }
        event.values.emplace_back(std::move(value));
    }
}

}  // namespace

bool FtraceBinaryParser::ParseCpuFile(const std::string &cpuFilePath, uint32_t cpuId, const GenericEventMeta &meta,
                                      size_t pageSize, std::vector<GenericEventData> &events, uint64_t &footerEvents)
{
    footerEvents = 0;
    const size_t pageDataOffset = meta.pageDataOffset;
    std::ifstream stream(cpuFilePath, std::ios::binary);
    if (!stream.is_open() || pageDataOffset < PAGE_DATA_OFFSET_MIN || pageDataOffset > pageSize)
    {
        return false;
    }
    stream.seekg(0, std::ios::end);
    const std::streamoff fileSize = stream.tellg();
    if (fileSize < static_cast<std::streamoff>(FOOTER_SIZE))
    {
        return false;
    }
    const std::streamoff dataSize = fileSize - static_cast<std::streamoff>(FOOTER_SIZE);
    stream.seekg(0, std::ios::beg);

    std::vector<uint8_t> page(pageSize);
    size_t pageIndex = 0;
    size_t desyncPages = 0;
    size_t firstBadPage = 0;
    size_t firstBadOffset = 0;
    size_t firstBadEnd = 0;
    uint64_t firstBadCommitted = 0;
    uint64_t dataRecords = 0;
    uint64_t unknownTypes = 0;
    bool lostEvents = false;
    bool shortRead = false;
    std::set<std::pair<GenericFieldKind, size_t>> unsupported;
    for (auto remain = dataSize; remain > 0;)
    {
        const auto toRead = std::min(remain, static_cast<std::streamoff>(pageSize));
        stream.read(reinterpret_cast<char *>(page.data()), static_cast<std::streamsize>(toRead));
        const auto got = static_cast<size_t>(stream.gcount());
        shortRead = got != static_cast<size_t>(toRead) || got < pageDataOffset;
        if (got < pageDataOffset)
        {
            break;
        }
        remain -= static_cast<std::streamoff>(got);
        uint64_t eventTimestamp = 0;
        memcpy(&eventTimestamp, page.data(), sizeof(eventTimestamp));
        uint64_t committed = 0;
        memcpy(&committed, page.data() + sizeof(uint64_t), sizeof(committed));
        lostEvents = lostEvents || (committed & RB_MISSED_EVENTS) != 0;
        const uint64_t committedBytes = committed & ~(RB_MISSED_EVENTS | RB_MISSED_STORED);
        size_t offset = pageDataOffset;
        const size_t end =
            pageDataOffset + static_cast<size_t>(std::min<uint64_t>(got - pageDataOffset, committedBytes));
        while (end - offset >= RB_EVENT_HDR_SIZE)
        {
            uint32_t header = 0;
            memcpy(&header, page.data() + offset, sizeof(header));
            const uint32_t type = header & RB_TYPE_MASK;
            const uint32_t timeDelta = header >> RB_TIME_DELTA_SHIFT;
            size_t eventLen = 0;
            size_t headerLen = RB_EVENT_HDR_SIZE;
            if (type == RB_TYPE_TIME_EXTEND || type == RB_TYPE_TIME_STAMP)
            {
                eventLen = type == RB_TYPE_TIME_EXTEND ? meta.timeExtendLen : meta.timeStampLen;
                if (eventLen < RB_TIME_RECORD_SIZE || eventLen > end - offset)
                {
                    break;
                }
                uint32_t high = 0;
                memcpy(&high, page.data() + offset + RB_EVENT_HDR_SIZE, sizeof(high));
                const uint64_t stamp = (static_cast<uint64_t>(high) << RB_TIME_LOW_BITS) | timeDelta;
                if (type == RB_TYPE_TIME_EXTEND)
                {
                    if (!Utils::AddWithoutOverflow(eventTimestamp, stamp, eventTimestamp))
                    {
                        break;
                    }
                }
                else
                {
                    // Linux rb_fix_abs_ts restores the five high bits omitted by the wire format.
                    const uint64_t highBits = eventTimestamp & RB_TIMESTAMP_HIGH_MASK;
                    uint64_t absolute = stamp | highBits;
                    if (highBits != 0 && absolute < eventTimestamp &&
                        !Utils::AddWithoutOverflow(absolute, RB_TIMESTAMP_WRAP, absolute))
                    {
                        break;
                    }
                    eventTimestamp = absolute;
                }
            }
            else if (type == RB_TYPE_PADDING || type == 0)
            {
                if (end - offset < 2 * RB_EVENT_HDR_SIZE)
                {
                    break;
                }
                uint32_t length = 0;
                memcpy(&length, page.data() + offset + RB_EVENT_HDR_SIZE, sizeof(length));
                eventLen = static_cast<size_t>(length) + RB_EVENT_HDR_SIZE;
                if (type == 0)
                {
                    headerLen += RB_EVENT_HDR_SIZE;
                }
            }
            else
            {
                eventLen = static_cast<size_t>(type) * 4 + RB_EVENT_HDR_SIZE;
            }
            if (eventLen < headerLen || eventLen > end - offset)
            {
                break;
            }
            if (type <= RB_TYPE_MAX_DATA)
            {
                const size_t payloadLength = eventLen - headerLen;
                if (payloadLength < sizeof(uint16_t) ||
                    !Utils::AddWithoutOverflow(eventTimestamp, timeDelta, eventTimestamp))
                {
                    break;
                }
                ++dataRecords;
                const auto *payload = page.data() + offset + headerLen;
                uint16_t kernelType = 0;
                memcpy(&kernelType, payload, sizeof(kernelType));
                const auto found = meta.typeIndex.find(kernelType);
                if (found == meta.typeIndex.end())
                {
                    ++unknownTypes;
                }
                else
                {
                    const auto &def = meta.types[found->second];
                    GenericEventData event;
                    event.cpuId = cpuId;
                    event.typeId = def.typeId;
                    event.rawTimestamp = eventTimestamp;
                    DecodeEvent(payload, payloadLength, def, event, unsupported);
                    events.emplace_back(std::move(event));
                }
            }
            offset += eventLen;
        }
        if (offset != end || committedBytes > got - pageDataOffset)
        {
            if (desyncPages == 0)
            {
                firstBadPage = pageIndex;
                firstBadOffset = offset;
                firstBadEnd = end;
                firstBadCommitted = committed;
            }
            ++desyncPages;
        }
        ++pageIndex;
        if (shortRead)
        {
            break;
        }
    }

    if (desyncPages > 0)
    {
        WARN(
            "ftrace cpu % page walk desync on %/% pages, first page % stopped at % (end %, committed %), "
            "timeExtendLen=% timeStampLen=%",
            cpuId, desyncPages, pageIndex, firstBadPage, firstBadOffset, firstBadEnd, firstBadCommitted,
            meta.timeExtendLen, meta.timeStampLen);
    }
    if (unknownTypes > 0)
    {
        WARN("ftrace cpu % skipped % records with no matching kernel event type.", cpuId, unknownTypes);
    }
    for (const auto &layout : unsupported)
    {
        WARN("ftrace cpu % encountered unsupported field kind %, size %.", cpuId, static_cast<unsigned>(layout.first),
             layout.second);
    }

    stream.clear();
    stream.seekg(dataSize, std::ios::beg);
    uint8_t footer[FOOTER_SIZE] = {0};
    stream.read(reinterpret_cast<char *>(footer), FOOTER_SIZE);
    const bool footerValid = stream.gcount() == static_cast<std::streamsize>(FOOTER_SIZE) &&
                             memcmp(footer, FOOTER_MAGIC, sizeof(FOOTER_MAGIC) - 1) == 0;
    if (!footerValid)
    {
        return false;
    }
    memcpy(&footerEvents, footer + FOOTER_EVENTS_OFFSET, sizeof(footerEvents));
    if (footerEvents != dataRecords)
    {
        WARN("ftrace cpu % footer reports % events but the file contains % decoded records.", cpuId, footerEvents,
             dataRecords);
    }
    return !shortRead && desyncPages == 0 && !lostEvents && unknownTypes == 0 && footerEvents == dataRecords;
}

}  // namespace Domain
}  // namespace Analysis
