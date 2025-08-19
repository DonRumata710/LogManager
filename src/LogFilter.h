#pragma once

#include "LogManagement/LogEntry.h"

#include <QRegularExpression>

#include <unordered_set>


class LogFilter
{
public:
    LogFilter() = default;
    LogFilter(const std::unordered_map<int, QRegularExpression>& columnFilters,
              const std::unordered_map<int, std::unordered_set<QString>>& variants,
              const QStringList& fields,
              const std::unordered_set<QString>& modules);

    bool isEmpty() const;

    bool check(const LogEntry& entry) const;

    void apply(const LogFilter& other);

private:
    std::unordered_map<int, QRegularExpression> columnFilters;
    std::unordered_map<int, std::unordered_set<QString>> variants;
    QStringList fields;
    std::unordered_set<QString> modules;
};

Q_DECLARE_METATYPE(LogFilter);
