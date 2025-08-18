#pragma once

#include "LogManagement/Session.h"

#include <vector>
#include <chrono>

namespace Statistics
{
struct Bucket
{
    std::chrono::system_clock::time_point start;
    int count = 0;
};

class LogHistogram
{
public:
    static std::vector<Bucket> calculate(Session& session,
                                         const std::chrono::system_clock::time_point& start,
                                         const std::chrono::system_clock::time_point& end,
                                         const std::chrono::system_clock::duration& bucketSize);
};
}

Q_DECLARE_METATYPE(Statistics::Bucket)
Q_DECLARE_METATYPE(std::vector<Statistics::Bucket>)

