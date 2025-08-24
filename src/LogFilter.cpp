#include "LogFilter.h"


LogFilter::LogFilter(const std::unordered_map<int, RegexFilter>& columnFilters,
                     const std::unordered_map<int, VariantFilter>& variants,
                     const QStringList& fields,
                     const std::unordered_set<QString>& modules,
                     FilterType modulesType) :
    columnFilters(columnFilters),
    variants(variants),
    fields(fields),
    modules(modules),
    modulesType(modulesType)
{}

bool LogFilter::isEmpty() const
{
    return columnFilters.empty() && variants.empty() && modules.empty();
}

bool LogFilter::check(const LogEntry& entry) const
{
    if (!modules.empty())
    {
        bool contains = modules.contains(entry.module);
        if (modulesType == FilterType::Whitelist && !contains)
            return false;
        if (modulesType == FilterType::Blacklist && contains)
            return false;
    }

    for (const auto& filter : columnFilters)
    {
        int column = filter.first;
        const QRegularExpression& rx = filter.second.regex;
        auto it = entry.values.find(fields[column]);
        if (rx.pattern().isEmpty())
            continue;
        bool match = it != entry.values.end() && rx.match(it->second.toString()).hasMatch();
        if (filter.second.type == FilterType::Whitelist && !match)
            return false;
        if (filter.second.type == FilterType::Blacklist && match)
            return false;
    }

    for (const auto& filter : variants)
    {
        int column = filter.first;
        auto it = entry.values.find(fields[column]);
        bool contains = it != entry.values.end() && filter.second.values.find(it->second.toString()) != filter.second.values.end();
        if (filter.second.type == FilterType::Whitelist && !contains)
            return false;
        if (filter.second.type == FilterType::Blacklist && contains)
            return false;
    }

    return true;
}

void LogFilter::apply(const LogFilter& other)
{
    for (const auto& filter : other.columnFilters)
    {
        if (!filter.second.regex.pattern().isEmpty() && filter.second.regex.isValid())
            columnFilters[filter.first] = filter.second;
    }

    for (const auto& variant : other.variants)
    {
        if (!variant.second.values.empty())
            variants[variant.first] = variant.second;
    }

    if (!other.modules.empty())
    {
        modules = other.modules;
        modulesType = other.modulesType;
    }
}
