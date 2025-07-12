#include "SearchController.h"

#include "Application.h"
#include "LogService.h"
#include "LogView/LogModel.h"
#include "Utils.h"

#include <QSortFilterProxyModel>


SearchController::SearchController(SearchBar* searchBar, QAbstractItemView* logView, QObject* parent) : QObject(parent)
{
    connect(searchBar, &SearchBar::localSearch, this, &SearchController::localSearch);
    connect(searchBar, &SearchBar::commonSearch, this, &SearchController::commonSearch);

    auto app = qobject_cast<Application*>(qApp);
    if (!app)
    {
        qWarning() << "Application instance is not available, SearchController cannot be initialized.";
        return;
    }

    auto logService = app->getLogService();
    if (!logService)
    {
        qWarning() << "LogService is not available, SearchController cannot be initialized.";
        return;
    }

    connect(this, &SearchController::startGlobalSearch, logService, &LogService::search);
}

void SearchController::localSearch(const QString& searchTerm, bool lastColumn, bool regexEnabled, bool backward)
{
    QT_SLOT_BEGIN

    search(logView->currentIndex(), searchTerm, lastColumn, regexEnabled, backward, false);

    QT_SLOT_END
}

void SearchController::commonSearch(const QString& searchTerm, bool lastColumn, bool regexEnabled, bool backward)
{
    QT_SLOT_BEGIN

    search(logView->currentIndex(), searchTerm, lastColumn, regexEnabled, backward, true);

    QT_SLOT_END
}

void SearchController::handleSearchResult(const QString& searchTerm, const QDateTime& entryTime)
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

void SearchController::search(const QModelIndex& from, const QString& searchTerm, bool lastColumn, bool regexEnabled, bool backward, bool globalSearch)
{
    QT_SLOT_BEGIN

    auto proxyModel = qobject_cast<QSortFilterProxyModel*>(logView->model());
    auto model = qobject_cast<LogModel*>(logView->model());
    if (!model && proxyModel)
        model = qobject_cast<LogModel*>(proxyModel->sourceModel());

    size_t start = from.row() + (backward ? -1 : 1);
    for (size_t i = start; i < model->rowCount(); backward ? --i : ++i)
    {
        QModelIndex index{ model->index(i, 0) };

        QString textToSearch = model->data(index, static_cast<int>(lastColumn ? LogModel::MetaData::Message : LogModel::MetaData::Line)).toString();

        bool flag = false;
        if (regexEnabled)
        {
            QRegularExpression regex(searchTerm, QRegularExpression::CaseInsensitiveOption);
            if (regex.match(textToSearch).hasMatch())
                flag = true;
        }
        else
        {
            if (textToSearch.contains(searchTerm))
                flag = true;
        }

        if (flag)
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
        startGlobalSearch(searchTerm, lastColumn, regexEnabled, backward);
    }

    QT_SLOT_END
}
