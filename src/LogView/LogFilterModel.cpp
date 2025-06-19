#include "LogFilterModel.h"

#include <QRegularExpression>


LogFilterModel::LogFilterModel(QObject *parent) : QSortFilterProxyModel(parent)
{
    setDynamicSortFilter(true);
}

void LogFilterModel::setFilterWildcard(int column, const QString& pattern)
{
    if (pattern.isEmpty())
        columnFilters.erase(column);
    else
        columnFilters[column] = QRegularExpression{ QRegularExpression::wildcardToRegularExpression(pattern) };
    invalidateFilter();
}

void LogFilterModel::setFilterRegularExpression(int column, const QString& pattern)
{
    if (pattern.isEmpty())
        columnFilters.erase(column);
    else
        columnFilters[column] = QRegularExpression(pattern);
    invalidateFilter();
}

void LogFilterModel::setVariantList(int column, const QStringList& values)
{
    if (values.isEmpty())
        variants.erase(column);
    else
        variants[column] = std::unordered_set<QString>{ values.begin(), values.end() };
    invalidateFilter();
}

bool LogFilterModel::filterAcceptsRow(int source_row, const QModelIndex& source_parent) const
{
    if (source_parent.isValid())
        return true;

    for (const auto& filter : columnFilters)
    {
        int column = filter.first;
        const QRegularExpression& rx = filter.second;
        QModelIndex idx = sourceModel()->index(source_row, column, source_parent);
        QString text = sourceModel()->data(idx, filterRole()).toString();
        if (!rx.pattern().isEmpty() && !rx.match(text).hasMatch())
            return false;
    }

    for (const auto& filter : variants)
    {
        int column = filter.first;
        const auto& variantSet = filter.second;
        QModelIndex idx = sourceModel()->index(source_row, column, source_parent);
        QString text = sourceModel()->data(idx, filterRole()).toString();

        if (variantSet.find(text) == variantSet.end())
            return false;
    }

    return true;
}
