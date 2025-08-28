#include "BookmarkTable.h"

#include "Utils.h"

#include <QAbstractItemView>
#include <QDateTime>
#include <QHeaderView>


BookmarkTable::BookmarkTable(QWidget* parent) : QDockWidget(parent), mTable(new QTableWidget(this))
{
    setWidget(mTable);
    mTable->setColumnCount(2);
    mTable->setHorizontalHeaderLabels(QStringList{ tr("Time"), tr("Line") });
    mTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    mTable->setSelectionMode(QAbstractItemView::SingleSelection);
    mTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    mTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeMode::ResizeToContents);
    mTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeMode::Stretch);

    connect(mTable, &QTableWidget::cellActivated, this, &BookmarkTable::handleCellActivated);
}

void BookmarkTable::setBookmarks(const std::vector<LogEntry>& bookmarks)
{
    mTable->clearContents();
    mTable->setRowCount(static_cast<int>(bookmarks.size()));
    int row = 0;
    for (const auto& entry : bookmarks)
    {
        auto time = DateTimeFromChronoSystemClock(entry.time);
        auto timeItem = new QTableWidgetItem(time.toString(Qt::ISODateWithMs));
        timeItem->setData(Qt::UserRole, time);
        auto lineItem = new QTableWidgetItem(entry.line);
        mTable->setItem(row, 0, timeItem);
        mTable->setItem(row, 1, lineItem);
        ++row;
    }
}

void BookmarkTable::clearBookmarks()
{
    mTable->clearContents();
    mTable->setRowCount(0);
}

void BookmarkTable::handleCellActivated(int row, int column)
{
    Q_UNUSED(column);
    auto item = mTable->item(row, 0);
    if (!item)
        return;
    emit bookmarkActivated(ChronoSystemClockFromDateTime(item->data(Qt::UserRole).toDateTime()));
}

