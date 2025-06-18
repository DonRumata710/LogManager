#pragma once

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

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const override;

private:
    std::unordered_map<int, QRegularExpression> columnFilters;
    std::unordered_map<int, std::unordered_set<QString>> variants;
};
