#pragma once

#include "LogStorage.h"
#include "LogEntry.h"
#include "LogUtils.h"

#include <QDebug>
#include <exception>

#include <boost/heap/priority_queue.hpp>


struct HeapItemCache
{
    QString module;
    std::chrono::system_clock::time_point time;
    int pos;
};

struct MergeHeapCache
{
    std::chrono::system_clock::time_point time;
    std::vector<HeapItemCache> heap;
};


template<bool straight = true>
class LogEntryIterator
{
public:
    static const bool IsStraight = straight;

    struct HeapItem
    {
        const LogStorage::LogMetaEntry* metadata;
        QString module;
        std::shared_ptr<Log> log;

        LogEntry entry;

        QString line;
        qint64 lineStart = 0;

        qint64 entryPos = 0;

        HeapItem() = default;
        HeapItem(const LogStorage::LogMetaEntry* metadata, QString&& module, std::shared_ptr<Log>&& log, LogEntry&& entry, QString&& line, qint64 lineStart, qint64 entryPos) :
            metadata(metadata), module(std::move(module)), log(std::move(log)), entry(std::move(entry)), line(std::move(line)), lineStart(lineStart), entryPos(entryPos)
        {}

        bool operator<(const HeapItem& other) const
        {
            return entry.time < other.entry.time;
        }

        bool operator>(const HeapItem& other) const
        {
            return entry.time > other.entry.time;
        }

        HeapItem(const HeapItemCache& cache, const std::shared_ptr<LogStorage>& logStorage)
        {
            const auto& md = logStorage->findLog(cache.module, cache.time);
            metadata = &md;
            module = cache.module;
            log = metadata->second.fileBuilder(metadata->second.filename, metadata->second.format);
            log->seek(cache.pos);
        }

        HeapItemCache getCache() const
        {
            HeapItemCache res;
            res.module = module;
            res.time = metadata->first;
            res.pos = entryPos;
            return res;
        }
    };

public:
    LogEntryIterator(const std::shared_ptr<LogStorage>& logStorage, const std::chrono::system_clock::time_point& _startTime, const std::chrono::system_clock::time_point& _endTime) :
        logStorage(logStorage),
        startTime(_startTime),
        endTime(_endTime)
    {
        ++endTime;
        for (const auto& module : logStorage->getModules())
        {
            const auto& metadata = logStorage->findLog(module, straight ? startTime : endTime);
            if (metadata.second.fileBuilder)
            {
                HeapItem heapItem;
                heapItem.metadata = &metadata;
                heapItem.module = module;
                heapItem.log = metadata.second.fileBuilder(metadata.second.filename, metadata.second.format);
                if (!straight)
                    heapItem.log->goToEnd();

                heapItem.line = heapItem.log->nextLine().value_or(QString());

                while (auto entry = getEntry(heapItem))
                {
                    if (entry->time >= startTime && entry->time < endTime)
                    {
                        prepareEntry(entry.value());
                        heapItem.entry = std::move(*entry);
                        mergeHeap.emplace(std::move(heapItem));
                        break;
                    }
                }
            }
        }
    }

    LogEntryIterator(const MergeHeapCache& heapCache,
                     const std::shared_ptr<LogStorage>& logStorage,
                     const std::chrono::system_clock::time_point& _startTime,
                     const std::chrono::system_clock::time_point& _endTime) :
        logStorage(logStorage),
        startTime(_startTime),
        endTime(_endTime)
    {
        auto leftModules = logStorage->getModules();
        for (const auto& heapItem : heapCache.heap)
        {
            leftModules.erase(heapItem.module);

            HeapItem item(heapItem, logStorage);
            if constexpr (straight)
                item.line = item.log->nextLine().value_or(QString());

            auto entry = getPreparedEntry(item);
            if (entry)
            {
                item.entry = entry.value();
                mergeHeap.emplace(std::move(item));
            }
        }

        if (!leftModules.empty())
        {
            for (const auto& module : leftModules)
            {
                const auto& metadata = logStorage->findLog(module, heapCache.time);
                if (metadata.second.fileBuilder)
                {
                    HeapItem heapItem;
                    heapItem.metadata = &metadata;
                    heapItem.module = module;
                    heapItem.log = metadata.second.fileBuilder(metadata.second.filename, metadata.second.format);
                    if (metadata.first < heapCache.time)
                        heapItem.log->goToEnd();

                    if constexpr (straight)
                        heapItem.line = heapItem.log->nextLine().value_or(QString());

                    while (auto entry = getEntry(heapItem))
                    {
                        if (entry->time >= startTime && entry->time < endTime)
                        {
                            prepareEntry(entry.value());
                            heapItem.entry = std::move(*entry);
                            mergeHeap.emplace(std::move(heapItem));
                            break;
                        }
                    }
                }
            }
        }

        if constexpr (!straight)
            endTime = heapCache.time;
    }

    std::chrono::system_clock::time_point getCurrentTime() const
    {
        if constexpr (straight)
        {
            if (hasLogs())
                return mergeHeap.top().entry.time;
        }
        else
        {
            return endTime;
        }
        return std::chrono::system_clock::time_point();
    }

    bool isValueAhead(const std::chrono::system_clock::time_point& time) const
    {
        if constexpr (straight)
            return mergeHeap.top().entry.time <= time;
        else
            return endTime > time;
    }

    bool hasLogs() const
    {
        return !mergeHeap.empty();
    }

    std::optional<LogEntry> next()
    {
        if (!hasLogs())
            return std::nullopt;

        HeapItem top = mergeHeap.top();
        mergeHeap.pop();

        auto nextEntry = getPreparedEntry(top);
        if (nextEntry && nextEntry->time >= startTime && nextEntry->time <= endTime)
            mergeHeap.emplace(std::move(top.metadata), std::move(top.module), std::move(top.log), std::move(*nextEntry), std::move(top.line), top.lineStart, top.entryPos);

        if constexpr (!straight)
            endTime = top.entry.time;

        return std::move(top.entry);
    }

    MergeHeapCache getCache() const
    {
        MergeHeapCache cache;
        if (!mergeHeap.empty())
        {
            cache.heap.reserve(mergeHeap.size());
            for (const auto& item : mergeHeap)
                cache.heap.emplace_back(item.getCache());
            cache.time = mergeHeap.top().entry.time;
        }
        return cache;
    }

private:
    std::optional<LogEntry> getPreparedEntry(HeapItem& heapItem)
    {
        auto res = getEntry(heapItem);
        if (res)
            prepareEntry(res.value());
        return res;
    }

    std::optional<LogEntry> getEntry(HeapItem& heapItem)
    {
        LogEntry entry;
        entry.module = heapItem.module;

        const auto& format = heapItem.metadata->second.format;

        std::optional<QString> line;
        if (!heapItem.line.isEmpty())
            line = heapItem.line;
        if constexpr (!straight)
            line = heapItem.log->prevLine();

        while (line.has_value())
        {
            QStringList parts;
            try
            {
                parts = splitLine(line.value(), format);
            }
            catch (const std::exception& ex)
            {
                qWarning() << "Failed to split line in" << heapItem.metadata->second.filename
                           << "at" << heapItem.log->getFilePosition() << ":" << line.value() << ':'
                           << ex.what();
                line = (heapItem.log.get()->*(straight ? &Log::nextLine : &Log::prevLine))();
                continue;
            }

            if (!checkFormat(parts, format) || parts.size() <= format->timeFieldIndex)
            {
                if constexpr (straight)
                {
                    if (!entry.line.isEmpty())
                    {
                        entry.line += '\n' + line.value();
                        if (!entry.additionalLines.isEmpty())
                            entry.additionalLines += '\n';
                        entry.additionalLines += line.value();
                    }
                    heapItem.lineStart = heapItem.log->getFilePosition();
                }
                else
                {
                    entry.line.prepend('\n' + line.value());
                    entry.additionalLines.prepend(line.value());
                }

                line = (heapItem.log.get()->*(straight ? &Log::nextLine : &Log::prevLine))();
                continue;
            }

            if constexpr (straight)
            {
                if (!entry.line.isEmpty())
                {
                    heapItem.line = line.value();
                    return entry;
                }
                entry.line = line.value();
                heapItem.entryPos = heapItem.lineStart;
            }
            else
            {
                entry.line.prepend(line.value());
            }

            try
            {
                entry.time = parseTime(parts[format->timeFieldIndex], format);
            }
            catch (const std::exception& ex)
            {
                qWarning() << "Failed to parse time in" << heapItem.metadata->second.filename
                           << "at" << heapItem.log->getFilePosition() << ':' << line.value() << ':'
                           << ex.what();
                if constexpr (straight)
                    heapItem.lineStart = heapItem.log->getFilePosition();
                line = (heapItem.log.get()->*(straight ? &Log::nextLine : &Log::prevLine))();
                continue;
            }
            for (size_t i = 0, fieldCount = 0; i < format->fields.size(); ++i)
            {
                const auto& field = format->fields[i];

                QRegularExpressionMatch match = field.regex.match(parts[fieldCount]);
                if (match.hasMatch())
                {
                    auto fieldValue = getValue(match.captured(0), field, format);
                    if (field.isEnum)
                    {
                        if (!field.values.empty() && !field.values.contains(fieldValue))
                        {
                            if (!field.isOptional)
                                qWarning() << "Enum value for field" << field.name << "is not defined in the format:" << fieldValue;
                            continue;
                        }

                        if (field.values.empty())
                            logStorage->getEnumList(field.name).emplace(fieldValue);
                    }

                    entry.values[field.name] = fieldValue;
                    ++fieldCount;
                }
                else
                {
                    if (!field.isOptional)
                    {
                        qCritical() << "Failed to match field" << field.name << "in line:" << line.value();
                    }
                    else if (parts[fieldCount].isEmpty())
                    {
                        ++fieldCount;
                    }
                }
            }

            if constexpr(!straight)
            {
                heapItem.entryPos = heapItem.log->getFilePosition();
                return entry;
            }

            if constexpr (straight)
                heapItem.lineStart = heapItem.log->getFilePosition();
            line = (heapItem.log.get()->*(straight ? &Log::nextLine : &Log::prevLine))();
        }

        switchToNextLog(heapItem);

        if (!entry.line.isEmpty())
            return entry;
        return std::nullopt;
    }

    void prepareEntry(LogEntry& entry)
    {
        auto lines = entry.additionalLines.split('\n');

        size_t minSpaces = std::numeric_limits<size_t>::max();
        for (auto& line : lines)
        {
            size_t spaces = 0;
            while (spaces < line.size() && (line[spaces] == ' '|| line[spaces] == '\t'))
                ++spaces;
            if (spaces < minSpaces)
                minSpaces = spaces;
        }

        if (minSpaces > 0)
        {
            for (auto& line : lines)
                line.remove(0, minSpaces);
            entry.additionalLines = lines.join('\n');
        }
    }

    void switchToNextLog(HeapItem& heapItem)
    {
        while (true)
        {
            const auto& log = (logStorage.get()->*(straight ? &LogStorage::findNextLog : &LogStorage::findPrevLog))(heapItem.module, heapItem.metadata->first);
            if (!log.second.fileBuilder)
            {
                heapItem.line.clear();
                return;
            }

            heapItem.metadata = &log;
            openLogFile(heapItem);
            if constexpr (!straight)
                heapItem.log->goToEnd();

            std::optional<QString> line = (heapItem.log.get()->*(straight ? &Log::nextLine : &Log::prevLine))();
            if (line)
            {
                heapItem.line = line.value();
                return;
            }
        }
    }

    void openLogFile(HeapItem& heapItem)
    {
        heapItem.log = heapItem.metadata->second.fileBuilder(heapItem.metadata->second.filename, heapItem.metadata->second.format);
    }

private:
    typedef std::conditional<straight, std::greater<HeapItem>, std::less<HeapItem>>::type Comparator;
    typedef boost::heap::priority_queue<HeapItem, boost::heap::compare<Comparator>> MergeHeap;

private:
    MergeHeap mergeHeap;
    std::shared_ptr<LogStorage> logStorage;

    std::chrono::system_clock::time_point startTime;
    std::chrono::system_clock::time_point endTime;
};
