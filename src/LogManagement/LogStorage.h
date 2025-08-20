#pragma once

#include "Format.h"
#include "LogMetadata.h"
#include "DirectoryScanner.h"

#include <QString>

#include <unordered_map>
#include <chrono>


class LogStorage
{
public:
    typedef std::pair<const std::chrono::system_clock::time_point, LogMetadata> LogMetaEntry;

public:
    LogStorage(std::vector<DirectoryScanner::LogFile>&& files);

    LogStorage getNarrowedStorage(const std::unordered_set<QString>& modules, const std::chrono::system_clock::time_point& minTime, const std::chrono::system_clock::time_point& maxTime) const;

    const std::unordered_set<std::shared_ptr<Format>>& getFormats() const;
    const std::unordered_set<QString>& getModules() const;

    const LogMetaEntry& findLog(const QString& module, const std::chrono::system_clock::time_point& time) const;
    const LogMetaEntry& findPrevLog(const QString& module, const std::chrono::system_clock::time_point& time) const;
    const LogMetaEntry& findNextLog(const QString& module, const std::chrono::system_clock::time_point& time) const;

    void addEnumValue(const QString& field, const QVariant& value);
    std::unordered_set<QVariant, VariantHash> getEnumList(const QString& field) const;

    void setTimeRange(const std::chrono::system_clock::time_point& minTime, const std::chrono::system_clock::time_point& maxTime);

    void setTimePoint(const std::chrono::system_clock::time_point& time);

    std::chrono::system_clock::time_point getMinTime() const;
    std::chrono::system_clock::time_point getMaxTime() const;

private:
    typedef std::unordered_map<QString, std::map<std::chrono::system_clock::time_point, LogMetadata, std::greater<std::chrono::system_clock::time_point>>> LogMap;

private:
    LogStorage() = default;

    void addFile(DirectoryScanner::LogFile&& file);
    void finalize(const std::unordered_map<QString, std::chrono::system_clock::time_point>& endTimes);

private:
    LogMap docs;
    std::unordered_set<std::shared_ptr<Format>> usedFormats;
    std::unordered_set<QString> modules;
    std::chrono::system_clock::time_point minTime;
    std::chrono::system_clock::time_point maxTime;
    std::unordered_map<QString, std::unordered_set<QVariant, VariantHash>> enumLists;
};
