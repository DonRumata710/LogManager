#include "LogHistogram.h"

namespace Statistics
{
std::vector<Bucket> LogHistogram::calculate(Session& session,
                                            const std::chrono::system_clock::time_point& start,
                                            const std::chrono::system_clock::time_point& end,
                                            const std::chrono::system_clock::duration& bucketSize)
{
    std::vector<Bucket> result;
    if (end <= start || bucketSize <= std::chrono::system_clock::duration::zero())
        return result;

    auto diff = end - start;
    auto bucketCount = diff / bucketSize;
    if (diff % bucketSize != std::chrono::system_clock::duration::zero())
        ++bucketCount;
    result.resize(static_cast<std::size_t>(bucketCount));
    for (std::size_t i = 0; i < result.size(); ++i)
        result[i].start = start + bucketSize * i;

    auto iterator = session.getIterator<>(start, end);
    while (iterator.hasLogs())
    {
        auto entry = iterator.next();
        if (!entry)
            break;
        auto index = (entry->time - start) / bucketSize;
        if (index >= 0 && static_cast<std::size_t>(index) < result.size())
            ++result[static_cast<std::size_t>(index)].count;
    }

    return result;
}
}

