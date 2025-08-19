#include "LogFilter.h"


LogFilter::LogFilter(const std::unordered_map<int, QRegularExpression>& columnFilters, const std::unordered_map<int, std::unordered_set<QString>>& variants, const QStringList& fields, const std::unordered_set<QString>& modules) :
    columnFilters(columnFilters),
    variants(variants),
    fields(fields),
    modules(modules)
{}

bool LogFilter::isEmpty() const
{
    return columnFilters.empty() && variants.empty() && modules.empty();
}

bool LogFilter::check(const LogEntry& entry) const
{
    if (!modules.empty() && !modules.contains(entry.module))
        return false;

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

void LogFilter::apply(const LogFilter& other)
{
    for (const auto& filter : other.columnFilters)
    {
        if (!filter.second.pattern().isEmpty() && filter.second.isValid())
            columnFilters[filter.first] = filter.second;
    }

    for (const auto& variant : other.variants)
    {
        if (!variant.second.empty())
            variants[variant.first] = variant.second;
    }

    if (!other.modules.empty())
        modules = other.modules;
}
