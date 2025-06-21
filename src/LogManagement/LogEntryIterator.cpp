#include "LogEntryIterator.h"

#include "LogUtils.h"

#include <QDateTime>


LogEntryIterator::LogEntryIterator(const std::shared_ptr<LogStorage>& logStorage, const std::chrono::system_clock::time_point& startTime) :
    logStorage(logStorage)
{
    for (const auto& module : logStorage->getModules())
    {
        const auto& metadata = logStorage->findLog(module, startTime);
        if (metadata.second.fileBuilder)
        {
            HeapItem heapItem;
            heapItem.metadata = &metadata.second;
            heapItem.module = module;
            heapItem.startTime = metadata.first;
            heapItem.log = metadata.second.fileBuilder(metadata.second.filename, metadata.second.format);

            heapItem.line = heapItem.log->nextLine().value_or(QString());

            while (auto entry = getEntry(heapItem))
            {
                if (entry->time >= startTime)
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

bool LogEntryIterator::hasLogs() const
{
    return !mergeHeap.empty();
}

std::optional<LogEntry> LogEntryIterator::next()
{
    if (!hasLogs())
        return std::nullopt;

    HeapItem top = mergeHeap.top();
    mergeHeap.pop();

    auto nextEntry = getPreparedEntry(top);
    if (nextEntry)
        mergeHeap.emplace(std::move(top.metadata), std::move(top.module), std::move(top.startTime), std::move(top.log), *nextEntry, std::move(top.line));

    return std::move(top.entry);
}

std::optional<LogEntry> LogEntryIterator::getPreparedEntry(HeapItem& heapItem)
{
    auto res = getEntry(heapItem);
    if (res)
        prepareEntry(res.value());
    return res;
}

void LogEntryIterator::prepareEntry(LogEntry& entry)
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

std::optional<LogEntry> LogEntryIterator::getEntry(HeapItem& heapItem)
{
    LogEntry entry;
    entry.module = heapItem.module;

    std::optional<QString> line = heapItem.line;
    do
    {
        auto parts = splitLine(line.value(), heapItem.metadata->format);
        if (!checkFormat(parts, heapItem.metadata->format) || parts.size() <= heapItem.metadata->format->timeFieldIndex)
        {
            if (!entry.line.isEmpty())
            {
                entry.line += '\n' + line.value();
                if (!entry.additionalLines.isEmpty())
                    entry.additionalLines += '\n';
                entry.additionalLines += line.value();
            }
            continue;
        }

        if (!entry.line.isEmpty())
        {
            heapItem.line = line.value();
            return entry;
        }

        entry.line = line.value();
        entry.time = parseTime(parts[heapItem.metadata->format->timeFieldIndex], heapItem.metadata->format);
        for (size_t i = 0, fieldCount = 0; i < heapItem.metadata->format->fields.size(); ++i)
        {
            const auto& field = heapItem.metadata->format->fields[i];

            QRegularExpressionMatch match = field.regex.match(parts[fieldCount]);
            if (match.hasMatch())
            {
                auto fieldValue = getValue(match.captured(0), field, heapItem.metadata->format);
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
                    for (const auto& val : field.values)
                        logStorage->getEnumList(field.name).emplace(val);
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
    }
    while ((line = heapItem.log->nextLine()));

    switchToNextLog(heapItem);

    if (!entry.line.isEmpty())
        return entry;
    return std::nullopt;
}

void LogEntryIterator::switchToNextLog(HeapItem& heapItem)
{
    while (true)
    {
        const auto& log = logStorage->findNextLog(heapItem.module, heapItem.startTime);
        if (!log.second.fileBuilder)
        {
            heapItem.line.clear();
            return;
        }

        heapItem.startTime = log.first;
        heapItem.metadata = &log.second;

        auto line = heapItem.log->nextLine();
        if (line)
        {
            heapItem.line = line.value();
            return;
        }
    }
}

bool LogEntryIterator::HeapItem::operator<(const HeapItem& other) const
{
    return entry.time < other.entry.time;
}

bool LogEntryIterator::HeapItem::operator>(const HeapItem& other) const
{
    return entry.time > other.entry.time;
}
