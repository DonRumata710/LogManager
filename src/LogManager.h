#pragma once

#include "Format.h"
#include "LogEntry.h"
#include "Log.h"

#include <QDateTime>

#include <unordered_set>
#include <unordered_map>
#include <map>
#include <filesystem>
#include <queue>


class LogManager
{
public:
    struct ScanResult
    {
        std::chrono::system_clock::time_point minTime;
        std::chrono::system_clock::time_point maxTime;
        std::unordered_set<QString> modules;

        ScanResult& operator+=(const ScanResult& other);
    };

    ScanResult loadFolders(const std::vector<QString>& folders, const std::vector<std::shared_ptr<Format>>& formats);
    std::optional<ScanResult> loadFile(const QString& filename, const std::vector<std::shared_ptr<Format>>& formats);

    const std::unordered_set<std::shared_ptr<Format>>& getFormats() const;
    const std::unordered_set<QString>& getModules() const;

    void setTimeRange(const std::chrono::system_clock::time_point& minTime, const std::chrono::system_clock::time_point& maxTime);

    void setTimePoint(const std::chrono::system_clock::time_point& time);

    bool hasLogs() const;
    std::optional<LogEntry> next();

private:
    struct LogMetadata
    {
        std::shared_ptr<Format> format;
        Log log;
    };

    struct HeapItem
    {
        QString module;
        std::chrono::system_clock::time_point startTime;
        LogMetadata* metadata;

        LogEntry entry;
        QString line;

        bool operator<(const HeapItem& other) const;
        bool operator>(const HeapItem& other) const;
    };

private:
    void clear();

    std::optional<LogManager::ScanResult> addFile(const QString& filename, const QString& stem, const QString& extension, const std::vector<std::shared_ptr<Format>>& formats);
    std::optional<std::pair<std::shared_ptr<Format>, std::chrono::system_clock::time_point>> scanLogFile(const QString& filename, const std::vector<std::shared_ptr<Format>>& formats);

    std::chrono::system_clock::time_point parseTime(const QString& timeStr, const std::shared_ptr<Format>& format) const;

    bool checkFormat(const QString& line, const std::shared_ptr<Format>& format);

    Log createLog(const QString& filename, std::shared_ptr<Format> format);

    std::optional<LogEntry> getEntry(HeapItem& heapItem);
    std::optional<QString> getLine(HeapItem& heapItem);

    void switchToNextLog(HeapItem& heapItem);

private:
    std::unordered_map<QString, std::map<std::chrono::system_clock::time_point, LogMetadata, std::greater<std::chrono::system_clock::time_point>>> docs;
    std::unordered_set<std::shared_ptr<Format>> usedFormats;
    std::unordered_set<QString> modules;
    std::chrono::system_clock::time_point minTime;
    std::chrono::system_clock::time_point maxTime;
    std::priority_queue<HeapItem, std::vector<HeapItem>, std::greater<HeapItem>> mergeHeap;
};
