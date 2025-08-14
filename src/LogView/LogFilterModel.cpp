#include "LogFilterModel.h"

#include "LogModel.h"
#include "FilteredLogModel.h"

#include <QRegularExpression>
#include <chrono>


LogFilterModel::LogFilterModel(QObject *parent) : QSortFilterProxyModel(parent)
{
    setDynamicSortFilter(true);
}

void LogFilterModel::setFilterWildcard(int column, const QString& pattern)
{
    double before = rowCount();
    auto it = filterEstimates.find(column);
    if (before == 0)
        before = it != filterEstimates.end() ? it->second : 1;
    else if (it != filterEstimates.end())
        before *= it->second;

    if (pattern.isEmpty())
        columnFilters.erase(column);
    else
        columnFilters[column] = QRegularExpression{ QRegularExpression::wildcardToRegularExpression(pattern, QRegularExpression::WildcardConversionOption::UnanchoredWildcardConversion) };

    invalidateFilter();
    double after = rowCount();

    if (pattern.isEmpty())
        filterEstimates.erase(column);
    else
        filterEstimates[column] = after > 0 ? before / after : before;

    updateSourceModel();
}

void LogFilterModel::setFilterRegularExpression(int column, const QString& pattern)
{
    double before = rowCount();
    auto it = filterEstimates.find(column);
    if (before == 0)
        before = it != filterEstimates.end() ? it->second : 1;
    else if (it != filterEstimates.end())
        before *= it->second;

    if (pattern.isEmpty())
        columnFilters.erase(column);
    else
        columnFilters[column] = QRegularExpression(pattern);

    invalidateFilter();
    double after = rowCount();

    if (pattern.isEmpty())
        filterEstimates.erase(column);
    else
        filterEstimates[column] = after > 0 ? before / after : before;

    updateSourceModel();
}

void LogFilterModel::setVariantList(int column, const QStringList& values)
{
    double before = rowCount();
    auto it = filterEstimates.find(column);
    if (before == 0)
        before = it != filterEstimates.end() ? it->second : 1;
    else if (it != filterEstimates.end())
        before *= it->second;

    if (values.isEmpty())
        variants.erase(column);
    else
        variants[column] = std::unordered_set<QString>{ values.begin(), values.end() };

    invalidateFilter();
    double after = rowCount();

    if (values.isEmpty())
        filterEstimates.erase(column);
    else
        filterEstimates[column] = after > 0 ? before / after : before;

    updateSourceModel();
}

void LogFilterModel::setSourceModel(QAbstractItemModel* model)
{
    baseModel = qobject_cast<LogModel*>(model);
    filteredModel.reset();
    filterEstimates.clear();
    QSortFilterProxyModel::setSourceModel(model);

    emit sourceModelChanged(model);
}

LogFilter LogFilterModel::exportFilter() const
{
    auto filterVariants = variants;
    auto fieldFilters = columnFilters;
    QStringList fields;
    std::unordered_set<QString> modules;
    for (int i = 0; i < sourceModel()->columnCount(); ++i)
    {
        if (i == static_cast<int>(LogModel::PredefinedColumn::Module))
        {
            auto it = filterVariants.find(i);
            if (it != filterVariants.end())
            {
                modules = it->second;
                filterVariants.erase(it);
            }
            continue;
        }

        fields.append(sourceModel()->headerData(i, Qt::Horizontal, Qt::DisplayRole).toString());
    }

    for (int i = static_cast<int>(LogModel::PredefinedColumn::Module) + 1; i < sourceModel()->columnCount(); ++i)
    {
        auto fieldIt = fieldFilters.find(i);
        if (fieldIt != fieldFilters.end())
        {
            fieldFilters.emplace(i - 1, fieldIt->second);
            fieldFilters.erase(fieldIt);
        }

        auto variantIt = filterVariants.find(i);
        if (variantIt != filterVariants.end())
        {
            filterVariants.emplace(i - 1, variantIt->second);
            filterVariants.erase(variantIt);
        }
    }

    return LogFilter{ fieldFilters, filterVariants, fields, modules };
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

void LogFilterModel::ensureFilteredModel()
{
    if (filteredModel)
        return;

    if (exportFilter().isEmpty())
        return;

    updateSourceModel();
}

void LogFilterModel::updateSourceModel()
{
    if (!baseModel)
        return;

    auto filter = exportFilter();
    double totalRate = 1.0;
    for (const auto& est : filterEstimates)
        totalRate *= est.second;

    if (filter.isEmpty() || !baseModel->isFulled() || totalRate <= 1.0)
    {
        if (filteredModel)
        {
            filteredModel.reset();
            QSortFilterProxyModel::setSourceModel(baseModel);
            invalidateFilter();
            emit sourceModelChanged(baseModel);
        }
        return;
    }

    constexpr double recreateThreshold = 10.0;
    if (filteredModel && totalRate < recreateThreshold)
    {
        filteredModel->setFilter(filter);
    }
    else
    {
        filteredModel = std::make_unique<FilteredLogModel>(baseModel->getService(), filter);
        filteredModel->goToTime(std::chrono::system_clock::time_point::min());
    }

    QSortFilterProxyModel::setSourceModel(filteredModel.get());
    clearFilters();
    invalidateFilter();
    emit sourceModelChanged(filteredModel.get());
}

void LogFilterModel::clearFilters()
{
    columnFilters.clear();
    variants.clear();
    filterEstimates.clear();
}
