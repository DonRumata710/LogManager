#pragma once

#include "LogFilter.h"

#include <QSortFilterProxyModel>
#include <QRegularExpression>

#include <memory>
#include <unordered_map>
#include <unordered_set>

class LogModel;
class FilteredLogModel;


#ifndef FILTER_TYPE_ENUM
#define FILTER_TYPE_ENUM
enum class FilterType
{
    Whitelist,
    Blacklist
};
Q_DECLARE_METATYPE(FilterType);
#endif

class LogFilterModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    explicit LogFilterModel(QObject *parent = nullptr);

    void setFilterWildcard(int column, const QString& pattern);
    void setFilterRegularExpression(int column, const QString& pattern);
    void setVariantList(int column, const QStringList& values);
    void setFilterMode(int column, FilterType type);
    FilterType filterMode(int column) const;

    void setSourceModel(QAbstractItemModel* sourceModel) override;

    LogFilter exportFilter() const;

    bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const override;

    void ensureFilteredModel();

signals:
    void handleError(const QString& message);
    void sourceModelChanged(QAbstractItemModel* model);

private:
    void updateSourceModel();
    void clearFilters();

    std::unordered_map<int, RegexFilter> columnFilters;
    std::unordered_map<int, VariantFilter> variants;
    std::unordered_map<int, double> filterEstimates;

    LogModel* baseModel = nullptr;
    std::unique_ptr<FilteredLogModel> filteredModel;
};
