#include "SearchController.h"

#include "Application.h"
#include "services/SearchService.h"
#include "LogView/LogModel.h"
#include "LogView/LogFilterModel.h"
#include "Utils.h"

#include <QSortFilterProxyModel>


SearchController::SearchController(SearchBar* searchBar, QAbstractItemView* logView, QObject* parent) :
    QObject(parent),
    logView(logView)
{
    connect(searchBar, &SearchBar::localSearch, this, &SearchController::localSearch);
    connect(searchBar, &SearchBar::commonSearch, this, &SearchController::commonSearch);

    auto app = qobject_cast<Application*>(qApp);
    if (!app)
    {
        qWarning() << "Application instance is not available, SearchController cannot be initialized.";
        return;
    }

    auto searchService = app->getSearchService();
    if (!searchService)
    {
        qWarning() << "SearchService is not available, SearchController cannot be initialized.";
        return;
    }
    connect(this, &SearchController::startGlobalSearch, searchService, &SearchService::search);
    connect(this, &SearchController::startGlobalSearchWithFilter, searchService, &SearchService::searchWithFilter);
    connect(searchService, &SearchService::searchFinished, this, &SearchController::handleSearchResult);
}

void SearchController::updateModel()
{
    auto proxyModel = qobject_cast<QSortFilterProxyModel*>(logView->model());
    auto model = qobject_cast<LogModel*>(logView->model());
    if (!model && proxyModel)
        model = qobject_cast<LogModel*>(proxyModel->sourceModel());

    connect(model, &LogModel::requestedTimeAvailable, this, &SearchController::handleLoadingFinished);
}

bool SearchController::checkEntry(const QString& textToSearch, const QString& searchTerm, bool lastColumn, bool regexEnabled)
{
    if (regexEnabled)
    {
        QRegularExpression regex(searchTerm, QRegularExpression::CaseInsensitiveOption);
        if (regex.match(textToSearch).hasMatch())
            return true;
    }
    else
    {
        if (textToSearch.contains(searchTerm))
            return true;
    }

    return false;
}

void SearchController::localSearch(const QString& searchTerm, bool lastColumn, bool regexEnabled, bool backward, bool useFilters)
{
    QT_SLOT_BEGIN

    search(logView->currentIndex(), searchTerm, lastColumn, regexEnabled, backward, useFilters, false);

    QT_SLOT_END
}

void SearchController::commonSearch(const QString& searchTerm, bool lastColumn, bool regexEnabled, bool backward, bool useFilters)
{
    QT_SLOT_BEGIN

    search(logView->currentIndex(), searchTerm, lastColumn, regexEnabled, backward, useFilters, true);

    QT_SLOT_END
}

void SearchController::handleSearchResult(const QString& searchTerm, const std::chrono::system_clock::time_point& entryTime)
{
    QT_SLOT_BEGIN

    if (currentSearchTerm == searchTerm)
    {
        auto proxyModel = qobject_cast<QSortFilterProxyModel*>(logView->model());
        auto model = qobject_cast<LogModel*>(logView->model());
        if (!model && proxyModel)
            model = qobject_cast<LogModel*>(proxyModel->sourceModel());

        if (!model)
            throw std::logic_error("LogModel is not available in SearchController::handleSearchResult");

        model->goToTime(entryTime);
    }

    QT_SLOT_END
}

void SearchController::handleLoadingFinished(const QModelIndex& index)
{
    QT_SLOT_BEGIN

    if (!currentSearchTerm.isEmpty())
    {
        logView->selectionModel()->setCurrentIndex(index, QItemSelectionModel::SelectionFlag::SelectCurrent | QItemSelectionModel::SelectionFlag::Rows);
        logView->scrollTo(index, QTreeView::ScrollHint::PositionAtCenter);
        currentSearchTerm.clear();
    }

    QT_SLOT_END
}

void SearchController::search(const QModelIndex& from, const QString& searchTerm, bool lastColumn, bool regexEnabled, bool backward, bool useFilters, bool globalSearch)
{
    QT_SLOT_BEGIN

    auto proxyModel = qobject_cast<LogFilterModel*>(logView->model());
    auto model = qobject_cast<LogModel*>(logView->model());
    if (!model && proxyModel)
        model = qobject_cast<LogModel*>(proxyModel->sourceModel());

    size_t start = from.row() + (backward ? -1 : 1);
    for (size_t i = start; i < model->rowCount(); backward ? --i : ++i)
    {
        QModelIndex index{ model->index(i, 0) };
        if (proxyModel && !proxyModel->filterAcceptsRow(i, QModelIndex()))
            continue;

        QString textToSearch = model->data(index, static_cast<int>(lastColumn ? LogModel::MetaData::Message : LogModel::MetaData::Line)).toString();
        if (checkEntry(textToSearch, searchTerm, lastColumn, regexEnabled))
        {
            qDebug() << "Search found at row:" << i << "text:" << textToSearch;

            if (proxyModel)
                index = proxyModel->mapFromSource(index);

            logView->selectionModel()->setCurrentIndex(index, QItemSelectionModel::SelectionFlag::SelectCurrent | QItemSelectionModel::SelectionFlag::Rows);
            logView->scrollTo(index, QTreeView::PositionAtCenter);
            return;
        }
    }

    if (globalSearch)
    {
        qDebug() << "Starting global search for term:" << searchTerm;
        currentSearchTerm = searchTerm;

        auto startTime = ChronoSystemClockFromDateTime(backward ? model->getStartTime() : model->getLastEntryTime());
        auto filters = proxyModel->exportFilter();
        if (useFilters && !filters.isEmpty())
            startGlobalSearchWithFilter(startTime, searchTerm, lastColumn, regexEnabled, backward, filters);
        else
            startGlobalSearch(startTime, searchTerm, lastColumn, regexEnabled, backward);
    }
    else
    {
        qDebug() << "Search term not found:" << searchTerm;
    }

    QT_SLOT_END
}
