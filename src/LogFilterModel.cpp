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

bool LogFilterModel::filterAcceptsRow(int source_row, const QModelIndex& source_parent) const
{
    if (columnFilters.empty())
        return true;

    for (auto& filter : columnFilters)
    {
        int column = filter.first;
        const QRegularExpression& rx = filter.second;
        QModelIndex idx = sourceModel()->index(source_row, column, source_parent);
        QString text = sourceModel()->data(idx, filterRole()).toString();
        if (!rx.pattern().isEmpty() && !rx.match(text).hasMatch())
            return false;
    }
    return true;
}
