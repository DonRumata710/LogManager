#pragma once

#include "LogStorage.h"
#include "LogEntry.h"

#include <queue>


class LogEntryIterator
{
public:
    LogEntryIterator(const std::shared_ptr<LogStorage>& logStorage, const std::chrono::system_clock::time_point& startTime);

    bool hasLogs() const;
    std::optional<LogEntry> next();

private:
    struct HeapItem
    {
        const LogStorage::LogMetadata* metadata;
        QString module;
        std::chrono::system_clock::time_point startTime;
        std::shared_ptr<Log> log;

        LogEntry entry;
        QString line;

        bool operator<(const HeapItem& other) const;
        bool operator>(const HeapItem& other) const;
    };

private:
    std::optional<LogEntry> getPreparedEntry(HeapItem& heapItem);
    std::optional<LogEntry> getEntry(HeapItem& heapItem);
    void prepareEntry(LogEntry& entry);

    void switchToNextLog(HeapItem &heapItem);

private:
    std::priority_queue<HeapItem, std::vector<HeapItem>, std::greater<HeapItem>> mergeHeap;
    std::shared_ptr<LogStorage> logStorage;
};
