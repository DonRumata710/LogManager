#pragma once

#include "LogManagement/LogEntry.h"
#include "EntryLinkView.h"

#include <QDockWidget>
#include <QTableWidget>
#include <vector>
#include <chrono>

class BookmarkTable : public QDockWidget
{
    Q_OBJECT

public:
    explicit BookmarkTable(QWidget* parent = nullptr);

    void setBookmarks(const std::vector<LogEntry>& bookmarks);
    void clearBookmarks();

signals:
    void bookmarkActivated(const std::chrono::system_clock::time_point& time);

private:
    void resetModel(QAbstractItemModel* model);

private:
    EntryLinkView* mTable;
};

