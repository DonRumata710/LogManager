#pragma once

#include "Format.h"
#include "Log.h"

#include <QString>

#include <unordered_map>
#include <chrono>


class LogStorage
{
public:
    typedef std::function<std::shared_ptr<Log>(const QString&, const std::shared_ptr<Format>&)> FileBuilder;

    struct LogMetadata
    {
        std::shared_ptr<Format> format;
        FileBuilder fileBuilder;
        QString filename;
    };
    typedef std::pair<const std::chrono::system_clock::time_point, LogMetadata> LogMetaEntry;

public:
    LogStorage();

    void addLog(const QString& module, const std::chrono::system_clock::time_point& time, const std::shared_ptr<Format>& format, LogMetadata&& log);

    const std::unordered_set<std::shared_ptr<Format>>& getFormats() const;
    const std::unordered_set<QString>& getModules() const;

    const LogMetaEntry& findLog(const QString& module, const std::chrono::system_clock::time_point& time) const;
    const LogMetaEntry& findPrevLog(const QString& module, const std::chrono::system_clock::time_point& time) const;
    const LogMetaEntry& findNextLog(const QString& module, const std::chrono::system_clock::time_point& time) const;

    std::unordered_set<QVariant, VariantHash>& getEnumList(const QString& field);
    const std::unordered_set<QVariant, VariantHash>& getEnumList(const QString& field) const;

    void setTimeRange(const std::chrono::system_clock::time_point& minTime, const std::chrono::system_clock::time_point& maxTime);

    void setTimePoint(const std::chrono::system_clock::time_point& time);

    std::chrono::system_clock::time_point getMinTime() const;
    std::chrono::system_clock::time_point getMaxTime() const;

private:
    typedef std::unordered_map<QString, std::map<std::chrono::system_clock::time_point, LogMetadata, std::greater<std::chrono::system_clock::time_point>>> LogMap;

private:
    LogMap docs;
    std::unordered_set<std::shared_ptr<Format>> usedFormats;
    std::unordered_set<QString> modules;
    std::chrono::system_clock::time_point minTime;
    std::chrono::system_clock::time_point maxTime;
    std::unordered_map<QString, std::unordered_set<QVariant, VariantHash>> enumLists;
};
