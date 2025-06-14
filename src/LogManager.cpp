#include "LogManager.h"

#include <QFile>
#include <QDebug>

#include <filesystem>


LogManager::ScanResult& LogManager::ScanResult::operator+=(const ScanResult& other)
{
    if (other.minTime < minTime)
        minTime = other.minTime;
    if (other.maxTime > maxTime)
        maxTime = other.maxTime;
    modules.insert(other.modules.begin(), other.modules.end());
    return *this;
}

LogManager::ScanResult LogManager::loadFolders(const std::vector<QString>& folders, const std::vector<std::shared_ptr<Format>>& formats)
{
    clear();

    ScanResult scanResult;
    scanResult.minTime = std::chrono::system_clock::now();
    scanResult.maxTime = std::chrono::system_clock::now();

    for (const auto& folder : folders)
    {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(folder.toStdString()))
        {
            if (!entry.is_regular_file())
                continue;

            auto filename = QString::fromStdString(entry.path().string());
            auto stem = QString::fromStdString(entry.path().stem().string());
            auto extension = QString::fromStdString(entry.path().extension().string());
            auto result = addFile(filename, stem, extension, formats);
            if (!result)
            {
                qDebug() << "No suitable format found for file:" << filename;
                continue;
            }

            scanResult += result.value();
        }
    }

    minTime = scanResult.minTime;
    maxTime = scanResult.maxTime;

    return scanResult;
}

std::optional<LogManager::ScanResult> LogManager::loadFile(const QString& filename, const std::vector<std::shared_ptr<Format>> &formats)
{
    clear();

    std::filesystem::path path = filename.toStdString();
    auto result = addFile(QString::fromStdString(path.string()), QString::fromStdString(path.stem().string()), QString::fromStdString(path.extension().string()), formats);

    if (!result)
    {
        qDebug() << "No suitable format found for file:" << filename;
        return std::nullopt;
    }

    ScanResult scanResult = result.value();
    scanResult.maxTime = std::chrono::system_clock::now();
    
    minTime = scanResult.minTime;
    maxTime = scanResult.maxTime;
    return scanResult;
}

const std::unordered_set<std::shared_ptr<Format>>& LogManager::getFormats() const
{
    return usedFormats;
}

const std::unordered_set<QString>& LogManager::getModules() const
{
    return modules;
}

void LogManager::setTimeRange(const std::chrono::system_clock::time_point& _minTime, const std::chrono::system_clock::time_point& _maxTime)
{
    minTime = _minTime;
    maxTime = _maxTime;

    setTimePoint(minTime);
}

void LogManager::setTimePoint(const std::chrono::system_clock::time_point& time)
{
    mergeHeap = std::priority_queue<HeapItem>();

    for (auto& module : docs)
    {
        auto logIt = module.second.upper_bound(time);
        if (logIt == module.second.begin())
            continue;

        --logIt;

        HeapItem heapItem;
        heapItem.module = module.first;
        heapItem.startTime = logIt->first;
        heapItem.metadata = &logIt->second;

        heapItem.line = logIt->second.log.nextLine().value_or(QString());

        while (auto entry = getEntry(heapItem))
        {
            if (entry->time >= time)
            {
                heapItem.entry = std::move(*entry);
                mergeHeap.emplace(std::move(heapItem));
                break;
            }
        }
    }
}

bool LogManager::hasLogs() const
{
    return !mergeHeap.empty();
}

std::optional<LogEntry> LogManager::next()
{
    if (!hasLogs())
        return std::nullopt;

    HeapItem top = mergeHeap.top();
    mergeHeap.pop();

    auto nextEntry = getEntry(top);
    if (nextEntry)
        mergeHeap.emplace(std::move(top.module), std::move(top.startTime), std::move(top.metadata), *nextEntry, std::move(top.line));

    return std::move(top.entry);
}

void LogManager::clear()
{
    docs.clear();
    mergeHeap = std::priority_queue<HeapItem>();
    usedFormats.clear();
    modules.clear();
}

std::optional<LogManager::ScanResult> LogManager::addFile(const QString& filename, const QString& stem, const QString& extension, const std::vector<std::shared_ptr<Format>>& formats)
{
    QString module = stem;

    std::vector<std::shared_ptr<Format>> actualFormats;
    std::unordered_map<std::shared_ptr<Format>, QString> regexMatches;
    for (const auto& format : formats)
    {
        QString formatModule = module;
        if (format->logFileRegex.isValid() && !format->logFileRegex.pattern().isEmpty())
        {
            QRegularExpressionMatch match = format->logFileRegex.match(stem);
            if (!match.hasMatch())
                continue;

            if (match.hasCaptured("module"))
                regexMatches[format] = match.captured("module");
        }

        if ((format->modules.empty() || format->modules.contains(formatModule)) && format->extension == extension)
            actualFormats.push_back(format);
    }

    if (actualFormats.empty())
        return std::nullopt;

    auto result = scanLogFile(filename, actualFormats);
    if (!result)
        return std::nullopt;

    qDebug() << "File discovered:" << filename;

    if (result->first->logFileRegex.isValid() && !result->first->logFileRegex.pattern().isEmpty())
    {
        auto matchIt = regexMatches.find(result->first);
        if (matchIt != regexMatches.end())
            module = matchIt->second;
        else
            module = result->first->name;
    }

    docs[module].emplace(result->second, LogMetadata{ result->first, createLog(filename, result->first) });
    usedFormats.insert(result->first);
    modules.insert(module);

    ScanResult scanResult;
    scanResult.minTime = result->second;
    scanResult.modules.insert(module);
    return scanResult;
}

std::optional<std::pair<std::shared_ptr<Format>, std::chrono::system_clock::time_point>> LogManager::scanLogFile(const QString& filename, const std::vector<std::shared_ptr<Format>>& formats)
{
    for (const auto& format : formats)
    {
        Log log(createLog(filename, format));
        auto line = log.nextLine();
        if (!line)
            continue;

        auto parts = line->split(format->separator);

        if (parts.size() <= format->timeFieldIndex)
            continue;

        if (!checkFormat(line.value(), format))
            continue;

        auto time = parseTime(parts[format->timeFieldIndex], format);
        return std::make_pair(format, time);
    }

    return std::nullopt;
}

std::chrono::system_clock::time_point LogManager::parseTime(const QString& timeStr, const std::shared_ptr<Format>& format) const
{
    std::istringstream ss(timeStr.toStdString());
    std::chrono::system_clock::time_point tp;
    ss >> std::chrono::parse(format->timeRegex.toStdString(), tp);
    return tp;
}

bool LogManager::checkFormat(const QString& line, const std::shared_ptr<Format>& format)
{
    for (const auto& field : format->fields)
    {
        QRegularExpressionMatch match = field.regex.match(line);
        if (!match.hasMatch())
        {
            return false;
        }
    }
    return true;
}

Log LogManager::createLog(const QString& path, std::shared_ptr<Format> format)
{
    return Log(std::make_unique<QFile>(path), format->encoding, std::shared_ptr<std::vector<Format::Comment>>(format, &format->comments));
}

std::optional<LogEntry> LogManager::getEntry(HeapItem& heapItem)
{
    LogEntry entry;
    entry.module = heapItem.module;

    std::optional<QString> line = heapItem.line;
    do
    {
        auto parts = line->split(heapItem.metadata->format->separator);
        if (!checkFormat(line.value(), heapItem.metadata->format) || parts.size() <= heapItem.metadata->format->timeFieldIndex)
        {
            if (!entry.line.isEmpty())
                entry.line += '\n' + line.value();
            continue;
        }

        auto time = parseTime(parts[heapItem.metadata->format->timeFieldIndex], heapItem.metadata->format);
        if (time < minTime || time > maxTime)
            continue;

        if (!entry.line.isEmpty())
        {
            heapItem.line = line.value();
            return entry;
        }

        entry.line = line.value();
        entry.time = time;
        for (size_t i = 0; i < heapItem.metadata->format->fields.size(); ++i)
        {
            const auto& field = heapItem.metadata->format->fields[i];
            auto& fieldValue = entry.values[field.name];

            QRegularExpressionMatch match = field.regex.match(parts[i].trimmed());
            if (match.hasMatch())
            {
                switch (field.type)
                {
                case QMetaType::Bool:
                {
                    static const std::unordered_set<QString> trueValues = {
                        "true", "t", "1", "yes", "y", "on", "enabled"
                    };
                    fieldValue = trueValues.contains(match.captured(0).toLower());
                    break;
                }
                case QMetaType::Int:
                    fieldValue = match.captured(0).toInt();
                    break;
                case QMetaType::UInt:
                    fieldValue = match.captured(0).toUInt();
                    break;
                case QMetaType::Double:
                    fieldValue = match.captured(0).toDouble();
                    break;
                case QMetaType::QString:
                    fieldValue = match.captured(0);
                    break;
                case QMetaType::QDateTime:
                    fieldValue = QDateTime::fromString(match.captured(0), heapItem.metadata->format->timeRegex);
                    break;
                default:
                    qCritical() << "Unsupported field type for field" << field.name << ": " << field.type;
                    continue;
                }
            }
            else
            {
                qCritical() << "Failed to match field" << field.name << "in line:" << line.value();
            }
        }
    }
    while ((line = heapItem.metadata->log.nextLine()));

    switchToNextLog(heapItem);

    if (!entry.line.isEmpty())
        return entry;
    return std::nullopt;
}

std::optional<QString> LogManager::getLine(HeapItem& heapItem)
{
    auto line = heapItem.metadata->log.nextLine();
    if (line)
        return line;

    auto docIt = docs.find(heapItem.module);
    if (docIt == docs.end())
    {
        qDebug() << "Unknown module: " << heapItem.module;
        return std::nullopt;
    }

    auto fileIt = docIt->second.upper_bound(heapItem.startTime);
    if (fileIt == docIt->second.begin())
        return std::nullopt;

    --fileIt;
    heapItem.startTime = fileIt->first;
    heapItem.metadata = &fileIt->second;

    return getLine(heapItem);
}

void LogManager::switchToNextLog(HeapItem& heapItem)
{
    while (true)
    {
        auto moduleIt = docs.find(heapItem.module);
        if (moduleIt == docs.end())
        {
            qDebug() << "Unknown module: " << heapItem.module;
            return;
        }

        auto fileIt = moduleIt->second.find(heapItem.startTime);
        if (fileIt == moduleIt->second.begin())
        {
            qDebug() << "No more logs for module:" << heapItem.module;
            heapItem.line.clear();
            return;
        }
        else if (fileIt == moduleIt->second.end())
        {
            qCritical() << "Unexpected result after next log search:" << heapItem.module;
            heapItem.line.clear();
            return;
        }

        --fileIt;
        heapItem.startTime = fileIt->first;
        heapItem.metadata = &fileIt->second;

        auto line = heapItem.metadata->log.nextLine();
        if (line)
        {
            heapItem.line = line.value();
            return;
        }
    }
}

bool LogManager::HeapItem::operator<(const HeapItem& other) const
{
    return entry.time < other.entry.time;
}
