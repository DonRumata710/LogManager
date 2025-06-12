#include "LogFilterModel.h"

#include <QRegularExpression>


LogFilterModel::LogFilterModel(QObject *parent) : QSortFilterProxyModel(parent)
{}

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
