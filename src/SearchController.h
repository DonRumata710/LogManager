#pragma once

#include "SearchBar.h"
#include "LogManagement/LogEntry.h"
#include "LogFilter.h"

#include <QObject>
#include <QAbstractItemView>
#include <chrono>


class SearchController : public QObject
{
    Q_OBJECT

public:
    SearchController(SearchBar* searchBar, QAbstractItemView* logView, QObject* parent = nullptr);

    void updateModel();

public:
    static bool checkEntry(const QString& textToSearch, const QString& searchTerm, bool lastColumn, bool regexEnabled);

signals:
    void startGlobalSearch(const std::chrono::system_clock::time_point& startTime, const QString& searchTerm, bool lastColumn, bool regexEnabled, bool backward);
    void startGlobalSearchWithFilter(const std::chrono::system_clock::time_point& startTime, const QString& searchTerm, bool lastColumn, bool regexEnabled, bool backward, const LogFilter& filter);

    void handleError(const QString& message);

public slots:
    void localSearch(const QString& searchTerm, bool lastColumn, bool regexEnabled, bool backward, bool useFilters);
    void commonSearch(const QString& searchTerm, bool lastColumn, bool regexEnabled, bool backward, bool useFilters);

    void handleSearchResult(const QString& searchTerm, const std::chrono::system_clock::time_point& entryTime);
    void handleLoadingFinished(const QModelIndex& index);

private:
    void search(const QModelIndex& from, const QString& searchTerm, bool lastColumn, bool regexEnabled, bool backward, bool useFilters, bool globalSearch);

private:
    QAbstractItemView* logView;
    QString currentSearchTerm;
};
