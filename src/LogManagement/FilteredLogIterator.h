#pragma once

#include "LogEntryIterator.h"
#include "../LogFilter.h"


template<bool straight = true>
class FilteredLogIterator
{
public:
    FilteredLogIterator(const std::shared_ptr<LogEntryIterator<straight>>& baseIterator, const LogFilter& logFilter) :
        iterator(baseIterator),
        filter(logFilter)
    {
        if (!iterator)
            throw std::invalid_argument("Base iterator cannot be null");
    }

    bool hasLogs() const
    {
        return iterator->hasLogs();
    }

    std::optional<LogEntry> next()
    {
        if (!hasLogs())
            return std::nullopt;

        while (auto entry = iterator->next())
        {
            if (filter.check(*entry))
                return entry;
        }

        return std::nullopt;
    }

    bool isValueAhead(const std::chrono::system_clock::time_point& time) const
    {
        return iterator->isValueAhead(time);
    }

    std::chrono::system_clock::time_point getCurrentTime() const
    {
        return iterator->getCurrentTime();
    }

    MergeHeapCache getCache() const
    {
        return iterator->getCache();
    }

private:
    std::shared_ptr<LogEntryIterator<straight>> iterator;
    LogFilter filter;
};

