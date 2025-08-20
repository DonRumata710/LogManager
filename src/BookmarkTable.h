#pragma once

#include "LogManagement/LogEntry.h"

#include <QTableWidget>
#include <QDateTime>
#include <vector>

class BookmarkTable : public QTableWidget
{
    Q_OBJECT
public:
    explicit BookmarkTable(QWidget* parent = nullptr);

    void setBookmarks(const std::vector<LogEntry>& bookmarks);
    void clearBookmarks();

signals:
    void bookmarkActivated(const QDateTime& time);

private slots:
    void handleCellActivated(int row, int column);
};

