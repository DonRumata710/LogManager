#pragma once

#include <QVariant>

#include <chrono>
#include <unordered_map>


struct LogEntry
{
    QString module;

    QString line;
    std::chrono::system_clock::time_point time;

    std::unordered_map<QString, QVariant> values;
    QString additionalLines;
};
