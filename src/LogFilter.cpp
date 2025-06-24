#include "LogFilter.h"


LogFilter::LogFilter(const std::unordered_map<int, QRegularExpression>& columnFilters, const std::unordered_map<int, std::unordered_set<QString>>& variants, const QStringList& fields) :
    columnFilters(columnFilters),
    variants(variants),
    fields(fields)
{}

void LogFilter::setFilterWildcard(int column, const QString& pattern)
{
    if (pattern.isEmpty())
        columnFilters.erase(column);
    else
        columnFilters[column] = QRegularExpression{ QRegularExpression::wildcardToRegularExpression(pattern) };
}

void LogFilter::setFilterRegularExpression(int column, const QString& pattern)
{
    if (pattern.isEmpty())
        columnFilters.erase(column);
    else
        columnFilters[column] = QRegularExpression(pattern);
}

void LogFilter::setVariantList(int column, const QStringList& values)
{
    if (values.isEmpty())
        variants.erase(column);
    else
        variants[column] = std::unordered_set<QString>{ values.begin(), values.end() };
}

bool LogFilter::check(const LogEntry& entry) const
{
    for (const auto& filter : columnFilters)
    {
        int column = filter.first;
        const QRegularExpression& rx = filter.second;
        auto it = entry.values.find(fields[column]);
        if (it == entry.values.end() || !rx.pattern().isEmpty() && !rx.match(it->second.toString()).hasMatch())
            return false;
    }

    for (const auto& filter : variants)
    {
        int column = filter.first;
        auto it = entry.values.find(fields[column]);
        if (it == entry.values.end() || variants.at(column).find(it->second.toString()) == variants.at(column).end())
            return false;
    }

    return true;
}
