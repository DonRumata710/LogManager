#include "BookmarkTable.h"

#include "Utils.h"

#include <QAbstractItemView>
#include <QDateTime>

BookmarkTable::BookmarkTable(QWidget* parent)
    : QTableWidget(parent)
{
    setColumnCount(2);
    setHorizontalHeaderLabels(QStringList{ tr("Time"), tr("Line") });
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    connect(this, &QTableWidget::cellActivated, this, &BookmarkTable::handleCellActivated);
}

void BookmarkTable::setBookmarks(const std::vector<LogEntry>& bookmarks)
{
    clearContents();
    setRowCount(static_cast<int>(bookmarks.size()));
    int row = 0;
    for (const auto& entry : bookmarks)
    {
        auto time = DateTimeFromChronoSystemClock(entry.time);
        auto timeItem = new QTableWidgetItem(time.toString(Qt::ISODateWithMs));
        timeItem->setData(Qt::UserRole, time);
        auto lineItem = new QTableWidgetItem(entry.line);
        setItem(row, 0, timeItem);
        setItem(row, 1, lineItem);
        ++row;
    }
}

void BookmarkTable::clearBookmarks()
{
    clearContents();
    setRowCount(0);
}

void BookmarkTable::handleCellActivated(int row, int column)
{
    Q_UNUSED(column);
    auto item = this->item(row, 0);
    if (!item)
        return;
    emit bookmarkActivated(ChronoSystemClockFromDateTime(item->data(Qt::UserRole).toDateTime()));
}

