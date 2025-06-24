#pragma once

#include "LogManagement/LogEntry.h"

#include <QRegularExpression>

#include <unordered_set>


class LogFilter
{
public:
    LogFilter() = default;
    LogFilter(const std::unordered_map<int, QRegularExpression>& columnFilters, const std::unordered_map<int, std::unordered_set<QString>>& variants, const QStringList& fields);

    void setFilterWildcard(int column, const QString& pattern);
    void setFilterRegularExpression(int column, const QString& pattern);
    void setVariantList(int column, const QStringList& values);

    bool check(const LogEntry& entry) const;

private:
    std::unordered_map<int, QRegularExpression> columnFilters;
    std::unordered_map<int, std::unordered_set<QString>> variants;
    QStringList fields;
};

Q_DECLARE_METATYPE(LogFilter);
