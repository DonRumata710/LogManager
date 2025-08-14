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
        if (iterator)
            advance();
    }

    bool hasLogs() const
    {
        return iterator && current.has_value();
    }

    std::optional<LogEntry> next()
    {
        if (!hasLogs())
            return std::nullopt;

        auto result = std::move(current);
        advance();
        return result;
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
    void advance()
    {
        current.reset();
        if (!iterator)
            return;

        while (auto entry = iterator->next())
        {
            if (filter.check(*entry))
            {
                current = std::move(entry);
                break;
            }
        }
    }

private:
    std::shared_ptr<LogEntryIterator<straight>> iterator;
    LogFilter filter;
    std::optional<LogEntry> current;
};

