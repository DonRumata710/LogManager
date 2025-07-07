#pragma once

#include "Format.h"
#include "LogStorage.h"
#include "LogEntryIterator.h"

#include <QDateTime>

#include <unordered_set>
#include <memory>


class Session
{
public:
    Session(const std::shared_ptr<LogStorage>&);

    const std::unordered_set<std::shared_ptr<Format>>& getFormats() const;
    const std::unordered_set<QString>& getModules() const;

    const std::unordered_set<QVariant, VariantHash>& getEnumList(const QString& field) const;

    std::chrono::system_clock::time_point getMinTime() const;
    std::chrono::system_clock::time_point getMaxTime() const;

    template<bool straight = true>
    LogEntryIterator<straight> getIterator(const std::chrono::system_clock::time_point& startTime = std::chrono::system_clock::time_point(), const std::chrono::system_clock::time_point& endTime = std::chrono::system_clock::time_point::max())
    {
        return LogEntryIterator<straight>(logStorage, startTime, endTime);
    }

    template<bool straight = true>
    LogEntryIterator<straight> createIterator(const std::vector<HeapItemCache>& cache, const std::chrono::system_clock::time_point& startTime = std::chrono::system_clock::time_point(), const std::chrono::system_clock::time_point& endTime = std::chrono::system_clock::time_point::max())
    {
        return LogEntryIterator<straight>(cache, logStorage, startTime, endTime);
    }

private:
    std::shared_ptr<LogStorage> logStorage;
};
