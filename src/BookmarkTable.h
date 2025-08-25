#pragma once

#include "LogManagement/LogEntry.h"

#include <QTableWidget>
#include <vector>
#include <chrono>

class BookmarkTable : public QTableWidget
{
    Q_OBJECT
public:
    explicit BookmarkTable(QWidget* parent = nullptr);

    void setBookmarks(const std::vector<LogEntry>& bookmarks);
    void clearBookmarks();

signals:
    void bookmarkActivated(const std::chrono::system_clock::time_point& time);

private slots:
    void handleCellActivated(int row, int column);
};

