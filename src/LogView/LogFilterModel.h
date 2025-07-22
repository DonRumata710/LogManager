#pragma once

#include "LogFilter.h"

#include <QSortFilterProxyModel>
#include <QRegularExpression>

#include <unordered_map>
#include <unordered_set>


class LogFilterModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    explicit LogFilterModel(QObject *parent = nullptr);

    void setFilterWildcard(int column, const QString& pattern);
    void setFilterRegularExpression(int column, const QString& pattern);
    void setVariantList(int column, const QStringList& values);

    LogFilter exportFilter() const;

    bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const override;

signals:
    void handleError(const QString& message);

private:
    std::unordered_map<int, QRegularExpression> columnFilters;
    std::unordered_map<int, std::unordered_set<QString>> variants;
};
