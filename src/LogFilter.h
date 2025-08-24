#pragma once

#include "LogManagement/LogEntry.h"

#include <QRegularExpression>

#include <unordered_set>

#ifndef FILTER_TYPE_ENUM
#define FILTER_TYPE_ENUM
enum class FilterType
{
    Whitelist,
    Blacklist
};
Q_DECLARE_METATYPE(FilterType);
#endif

struct RegexFilter
{
    QRegularExpression regex;
    FilterType type = FilterType::Whitelist;
};

struct VariantFilter
{
    std::unordered_set<QString> values;
    FilterType type = FilterType::Whitelist;
};

class LogFilter
{
public:
    LogFilter() = default;
    LogFilter(const std::unordered_map<int, RegexFilter>& columnFilters,
              const std::unordered_map<int, VariantFilter>& variants,
              const QStringList& fields,
              const std::unordered_set<QString>& modules,
              FilterType modulesType = FilterType::Whitelist);

    bool isEmpty() const;

    bool check(const LogEntry& entry) const;

    void apply(const LogFilter& other);

private:
    std::unordered_map<int, RegexFilter> columnFilters;
    std::unordered_map<int, VariantFilter> variants;
    QStringList fields;
    std::unordered_set<QString> modules;
    FilterType modulesType = FilterType::Whitelist;
};

Q_DECLARE_METATYPE(LogFilter);
