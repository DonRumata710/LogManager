#include "LogStorage.h"

#include <QDebug>
#include <QDateTime>


LogStorage::LogStorage() :
    maxTime(std::chrono::system_clock::now())
{}

void LogStorage::addLog(const QString& module, const std::chrono::system_clock::time_point& time, const std::shared_ptr<Format>& format, LogMetadata&& log)
{
    auto& logMap = docs[module];
    auto it = logMap.find(time);
    if (it == logMap.end())
    {
        logMap.emplace(time, std::move(log));
        usedFormats.insert(format);
        modules.insert(module);

        if (minTime == std::chrono::system_clock::time_point() || time < minTime)
            minTime = time;
    }
    else
    {
        qWarning() << "Log already exists for module" << module << "at time" << QDateTime::fromSecsSinceEpoch(std::chrono::system_clock::to_time_t(time));
    }
}

const std::unordered_set<std::shared_ptr<Format>>& LogStorage::getFormats() const
{
    return usedFormats;
}

const std::unordered_set<QString>& LogStorage::getModules() const
{
    return modules;
}

const std::pair<const std::chrono::system_clock::time_point, LogStorage::LogMetadata>& LogStorage::findLog(const QString& module, const std::chrono::system_clock::time_point& time) const
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
        if (logIt != it->second.end())
            return *logIt;
    }

    qDebug() << "Log not found for module" << module << "at time" << QDateTime::fromSecsSinceEpoch(std::chrono::system_clock::to_time_t(time));

    static const std::pair<const std::chrono::system_clock::time_point, LogStorage::LogMetadata> emptyPair;
    return emptyPair;
}

const std::pair<const std::chrono::system_clock::time_point, LogStorage::LogMetadata>& LogStorage::findNextLog(const QString& module, const std::chrono::system_clock::time_point& time) const
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
            return *logIt;
        }
    }

    static const std::pair<const std::chrono::system_clock::time_point, LogStorage::LogMetadata> emptyPair;
    return emptyPair;
}

std::unordered_set<QVariant, VariantHash>& LogStorage::getEnumList(const QString& field)
{
    auto it = enumLists.find(field);
    if (it == enumLists.end())
    {
        return enumLists.emplace().first->second;
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
