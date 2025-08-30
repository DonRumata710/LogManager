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
    connect(searchBar, &SearchBar::search, this, &SearchController::search);

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
    connect(searchService, &SearchService::searchResults, this, &SearchController::searchResults);
}

void SearchController::updateModel()
{
    auto proxyModel = qobject_cast<QSortFilterProxyModel*>(logView->model());
    auto model = qobject_cast<LogModel*>(logView->model());
    if (!model && proxyModel)
        model = qobject_cast<LogModel*>(proxyModel->sourceModel());
}

bool SearchController::checkEntry(const QString& textToSearch, const QString& searchTerm, bool regexEnabled)
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

void SearchController::search(const QString& searchTerm, bool regexEnabled, bool backward, bool useFilters, bool global, bool findAll, int column)
{
    QT_SLOT_BEGIN

    searchImpl(logView->currentIndex(), searchTerm, regexEnabled, backward, useFilters, global, findAll, column);

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

void SearchController::searchImpl(const QModelIndex& from, const QString& searchTerm, bool regexEnabled, bool backward, bool useFilters, bool globalSearch, bool findAll, int column)
{
    QT_SLOT_BEGIN

    auto proxyModel = qobject_cast<LogFilterModel*>(logView->model());
    auto model = qobject_cast<LogModel*>(logView->model());
    if (!model && proxyModel)
        model = qobject_cast<LogModel*>(proxyModel->sourceModel());

    if (findAll)
    {
        if (globalSearch)
        {
            auto startTime = ChronoSystemClockFromDateTime(model->getStartTime());
            auto filters = proxyModel->exportFilter();
            if (useFilters && !filters.isEmpty())
                startGlobalSearchWithFilter(startTime, searchTerm, regexEnabled, backward, findAll, column, model->getFieldsName(), filters);
            else
                startGlobalSearch(startTime, searchTerm, regexEnabled, backward, findAll, column, model->getFieldsName());
        }
        else
        {
            QMap<std::chrono::system_clock::time_point, QString> results;
            for (int i = 0; i < model->rowCount(); ++i)
            {
                bool specifiedColumn = (column >= 0 && column < model->columnCount() - 1);
                bool lastColumn = (column == model->columnCount() - 1);
                QModelIndex index{ model->index(i, specifiedColumn ? column : 0) };
                if (proxyModel && !proxyModel->filterAcceptsRow(i, QModelIndex()))
                    continue;

                QString textToSearch = model->data(index, specifiedColumn ? Qt::DisplayRole : static_cast<int>((lastColumn ? LogModel::MetaData::Message : LogModel::MetaData::Line))).toString();
                if (checkEntry(textToSearch, searchTerm, regexEnabled))
                {
                    auto time = model->data(index, static_cast<int>(LogModel::MetaData::Time)).value<std::chrono::system_clock::time_point>();
                    auto line = model->data(index, static_cast<int>(LogModel::MetaData::Line)).toString();
                    results.insert(time, line);
                }
            }

            emit searchResults(results);
            qDebug() << "Found" << results.size() << "entries for term:" << searchTerm;
        }
    }
    else
    {
        size_t start = from.row() + (backward ? -1 : 1);
        bool found = false;
        for (size_t i = start; i < model->rowCount(); backward ? --i : ++i)
        {
            bool specifiedColumn = (column >= 0 && column < model->columnCount() - 1);
            bool lastColumn = (column == model->columnCount() - 1);
            QModelIndex index{ model->index(i, specifiedColumn ? column : 0) };
            if (proxyModel && !proxyModel->filterAcceptsRow(i, QModelIndex()))
                continue;

            QString textToSearch = model->data(index, specifiedColumn ? Qt::DisplayRole : static_cast<int>((lastColumn ? LogModel::MetaData::Message : LogModel::MetaData::Line))).toString();
            if (checkEntry(textToSearch, searchTerm, regexEnabled))
            {
                qDebug() << "Search found at row:" << i << "text:" << textToSearch;

                if (proxyModel)
                    index = proxyModel->mapFromSource(index);

                logView->selectionModel()->setCurrentIndex(index, QItemSelectionModel::SelectionFlag::SelectCurrent | QItemSelectionModel::SelectionFlag::Rows);
                logView->scrollTo(index, QTreeView::PositionAtCenter);
                found = true;
                break;
            }
        }

        if (!found)
        {
            if (globalSearch)
            {
                qDebug() << "Starting global search for term:" << searchTerm;
                currentSearchTerm = searchTerm;

                auto startTime = ChronoSystemClockFromDateTime(backward ? model->getStartTime() : model->getLastEntryTime());
                auto filters = proxyModel->exportFilter();
                if (useFilters && !filters.isEmpty())
                    startGlobalSearchWithFilter(startTime, searchTerm, regexEnabled, backward, findAll, column, model->getFieldsName(), filters);
                else
                    startGlobalSearch(startTime, searchTerm, regexEnabled, backward, findAll, column, model->getFieldsName());
            }
            else
            {
                qDebug() << "Search term not found:" << searchTerm;
            }
        }
    }

    QT_SLOT_END
}
