#include "BookmarkTable.h"

#include "EntryLinkModel.h"

#include <QAbstractItemView>
#include <QDateTime>
#include <QHeaderView>


BookmarkTable::BookmarkTable(QWidget* parent) : QDockWidget(parent), mTable(new EntryLinkView(this))
{
    setWidget(mTable);
    mTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    mTable->setSelectionMode(QAbstractItemView::SingleSelection);
    mTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    QSizePolicy sizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);
    sizePolicy.setHorizontalStretch(1);
    sizePolicy.setVerticalStretch(1);
    mTable->setSizePolicy(sizePolicy);

    setAllowedAreas(Qt::DockWidgetArea::TopDockWidgetArea | Qt::DockWidgetArea::BottomDockWidgetArea);

    connect(mTable, &EntryLinkView::entryActivated, this, &BookmarkTable::bookmarkActivated);
}

void BookmarkTable::setBookmarks(const std::vector<LogEntry>& bookmarks)
{
    QMap<std::chrono::system_clock::time_point, QString> map;
    for (const auto& bookmark : bookmarks)
        map.insert(bookmark.time, bookmark.line);
    resetModel(new EntryLinkModel(map, this));
}

void BookmarkTable::clearBookmarks()
{
    resetModel(nullptr);
}

void BookmarkTable::resetModel(QAbstractItemModel* model)
{
    auto oldModel = mTable->model();
    mTable->setModel(model);

    if (oldModel)
        oldModel->deleteLater();
}
