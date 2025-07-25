#include "LogStorage.h"
#include "LogManagement/LogUtils.h"

#include "Utils.h"

#include <QDebug>
#include <QDateTime>


LogStorage::LogStorage(std::vector<DirectoryScanner::LogFile>&& files) : maxTime(std::chrono::system_clock::time_point::min())
{
    std::unordered_map<QString, std::chrono::system_clock::time_point> endTimes;
    for (auto&& file : files)
    {
        addFile(std::move(file));
        endTimes[file.module] = file.end;
    }
    finalize(endTimes);
}

LogStorage LogStorage::getNarrowedStorage(const std::unordered_set<QString>& modules, const std::chrono::system_clock::time_point& newMinTime, const std::chrono::system_clock::time_point& newMaxTime) const
{
    LogStorage res;

    if (newMinTime == std::chrono::system_clock::time_point() || newMinTime < minTime)
    {
        res.minTime = minTime;
        qWarning() << "Narrowed storage requested with minTime before existing minTime:" << QDateTime::fromSecsSinceEpoch(std::chrono::system_clock::to_time_t(minTime));
    }
    else
    {
        res.minTime = newMinTime;
    }

    if (newMaxTime == std::chrono::system_clock::time_point() || newMaxTime > maxTime)
    {
        res.maxTime = maxTime;
        qWarning() << "Narrowed storage requested with maxTime after existing maxTime:" << QDateTime::fromSecsSinceEpoch(std::chrono::system_clock::to_time_t(maxTime));
    }
    else
    {
        res.maxTime = newMaxTime;
    }

    res.modules = modules;
    for (const auto& module : modules)
    {
        res.docs[module] = docs.at(module);

        const auto& format = res.docs[module].rbegin()->second.format;
        res.usedFormats.insert(format);
    }

    for (const auto& format : res.usedFormats)
    {
        for (const auto& field : format->fields)
            res.enumLists[format->name] = getEnumList(field.name);
    }

    return res;
}

const std::unordered_set<std::shared_ptr<Format>>& LogStorage::getFormats() const
{
    return usedFormats;
}

const std::unordered_set<QString>& LogStorage::getModules() const
{
    return modules;
}

const LogStorage::LogMetaEntry& LogStorage::findLog(const QString& module, const std::chrono::system_clock::time_point& time) const
{
    auto it = docs.find(module);
    if (it == docs.end())
    {
        qCritical() << "Unknown module:" << module;
    }
    else
    {
        auto logIt = it->second.lower_bound(time);
        if (logIt != it->second.begin() && logIt == it->second.end())
            --logIt;
        if (logIt != it->second.end() && !logIt->second.fileBuilder)
            ++logIt;
        if (logIt != it->second.end())
            return *logIt;
    }

    qDebug() << "Log not found for module" << module << "at time" << QDateTime::fromSecsSinceEpoch(std::chrono::system_clock::to_time_t(time));

    static const std::pair<const std::chrono::system_clock::time_point, LogMetadata> emptyPair;
    return emptyPair;
}

const LogStorage::LogMetaEntry& LogStorage::findPrevLog(const QString& module, const std::chrono::system_clock::time_point& time) const
{
    auto it = docs.find(module);
    if (it == docs.end())
    {
        qCritical() << "Unknown module:" << module;
    }
    else
    {
        auto logIt = it->second.find(time);
        if (logIt != it->second.end())
        {
            ++logIt;
            if (logIt != it->second.end())
                return *logIt;
        }
    }

    qDebug() << "Previous log not found for module" << module << "at time" << QDateTime::fromSecsSinceEpoch(std::chrono::system_clock::to_time_t(time));

    static const std::pair<const std::chrono::system_clock::time_point, LogMetadata> emptyPair;
    return emptyPair;
}

const LogStorage::LogMetaEntry& LogStorage::findNextLog(const QString& module, const std::chrono::system_clock::time_point& time) const
{
    auto it = docs.find(module);
    if (it == docs.end())
    {
        qCritical() << "Unknown module:" << module;
    }
    else
    {
        auto logIt = it->second.find(time);
        if (logIt != it->second.begin())
        {
            --logIt;
            if (logIt->second.fileBuilder)
            return *logIt;
        }
    }

    static const std::pair<const std::chrono::system_clock::time_point, LogMetadata> emptyPair;
    return emptyPair;
}

std::unordered_set<QVariant, VariantHash>& LogStorage::getEnumList(const QString& field)
{
    auto it = enumLists.find(field);
    if (it == enumLists.end())
    {
        return enumLists.emplace(field, std::unordered_set<QVariant, VariantHash>{}).first->second;
    }
    else
    {
        return it->second;
    }
}

const std::unordered_set<QVariant, VariantHash>& LogStorage::getEnumList(const QString& field) const
{
    auto it = enumLists.find(field);
    if (it != enumLists.end())
    {
        return it->second;
    }
    else
    {
        static const std::unordered_set<QVariant, VariantHash> emptySet;
        return emptySet;
    }
}

std::chrono::system_clock::time_point LogStorage::getMinTime() const
{
    return minTime;
}

std::chrono::system_clock::time_point LogStorage::getMaxTime() const
{
    return maxTime;
}

void LogStorage::addFile(DirectoryScanner::LogFile&& file)
{
    auto& logMap = docs[file.module];
    auto it = logMap.lower_bound(file.start);
    if (it == logMap.end() || it->first != file.start)
    {
        usedFormats.insert(file.metadata.format);
        logMap.emplace_hint(it, file.start, std::move(file.metadata));
        modules.insert(file.module);

        if (minTime == std::chrono::system_clock::time_point() || file.start < minTime)
            minTime = file.start;
        if (maxTime == std::chrono::system_clock::time_point() || file.end > maxTime)
            maxTime = file.end;
    }
    else
    {
        qWarning() << "Log already exists for module" << file.module << "at time" << DateTimeFromChronoSystemClock(file.start);
    }
}

void LogStorage::finalize(const std::unordered_map<QString, std::chrono::system_clock::time_point>& endTimes)
{
    for (auto& module : docs)
    {
        auto& logMap = module.second;
        if (logMap.empty())
            throw std::logic_error("LogStorage::finalize: No logs found for module " + module.first.toStdString());

        const auto& endTime = endTimes.at(module.first) + std::chrono::milliseconds(1);
        maxTime = std::max(maxTime, endTime);
        module.second.emplace(endTime, LogMetadata{});
    }
}
