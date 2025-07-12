#pragma once

#include "SearchBar.h"

#include <QObject>
#include <QAbstractItemView>


class SearchController : public QObject
{
    Q_OBJECT

public:
    SearchController(SearchBar* searchBar, QAbstractItemView* logView, QObject* parent = nullptr);

signals:
    void startGlobalSearch(const QString& searchTerm, bool lastColumn, bool regexEnabled, bool backward);

public slots:
    void localSearch(const QString& searchTerm, bool lastColumn, bool regexEnabled, bool backward);
    void commonSearch(const QString& searchTerm, bool lastColumn, bool regexEnabled, bool backward);

    void handleSearchResult(const QString& searchTerm, const QDateTime& entryTime);

private:
    void search(const QModelIndex& from, const QString& searchTerm, bool lastColumn, bool regexEnabled, bool backward, bool globalSearch);

private:
    QAbstractItemView* logView;
    QString currentSearchTerm;
};
