#pragma once

#include <QSortFilterProxyModel>
#include <QRegularExpression>

#include <unordered_map>


class LogFilterModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    explicit LogFilterModel(QObject *parent = nullptr);

    void setFilterWildcard(int column, const QString& pattern);
    void setFilterRegularExpression(int column, const QString& pattern);

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const override
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

private:
    std::unordered_map<int, QRegularExpression> columnFilters;
};
