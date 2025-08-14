#include "LogView.h"

#include "FilterHeader.h"
#include "LogModel.h"
#include "LogFilterModel.h"
#include "../Utils.h"

#include <QScrollBar>
#include <QAbstractProxyModel>
#include <algorithm>


LogView::LogView(QWidget* parent) : QTreeView(parent)
{
    setHeader(new FilterHeader(Qt::Horizontal, this));
    setUniformRowHeights(false);

    connect(verticalScrollBar(), &QScrollBar::valueChanged, this, &LogView::checkFetchNeeded);
    connect(verticalScrollBar(), &QScrollBar::rangeChanged, this, &LogView::checkFetchNeeded);

    connect(static_cast<FilterHeader*>(header()), &FilterHeader::handleError, this, &LogView::handleError);
}

void LogView::setLogModel(QAbstractItemModel* newModel)
{
    if (currentLogModel)
        disconnect(currentLogModel, nullptr, this, nullptr);
    if (currentProxyModel)
        disconnect(currentProxyModel, nullptr, this, nullptr);

    setModel(newModel);

    currentProxyModel = qobject_cast<LogFilterModel*>(newModel);
    currentLogModel = qobject_cast<LogModel*>(newModel);
    if (!currentLogModel && currentProxyModel)
        currentLogModel = qobject_cast<LogModel*>(currentProxyModel->sourceModel());

    if (!currentLogModel)
    {
        qWarning() << "Unexpected model type in log view";
        return;
    }

    if (currentProxyModel)
    {
        connect(currentProxyModel, &LogFilterModel::sourceModelChanged, this, [this](QAbstractItemModel*) {
            setLogModel(model());
        });
    }

    connect(currentLogModel, &LogModel::modelReset, this, &LogView::handleReset);
    connect(currentLogModel, &LogModel::modelReset, this, &LogView::handleFirstDataLoaded);

    connect(currentLogModel, &LogModel::rowsAboutToBeInserted, this, &LogView::handleFirstLineChangeStart);
    connect(currentLogModel, &LogModel::rowsAboutToBeRemoved, this, &LogView::handleFirstLineChangeStart);
    connect(currentLogModel, &LogModel::rowsInserted, this, &LogView::handleFirstLineAddition);
    connect(currentLogModel, &LogModel::rowsRemoved, this, &LogView::handleFirstLineRemoving);
}

void LogView::checkFetchNeeded()
{
    QT_SLOT_BEGIN
    QScrollBar* sb = verticalScrollBar();

    auto* proxyModel = qobject_cast<QAbstractProxyModel*>(model());
    auto* logModel = proxyModel
                     ? qobject_cast<LogModel*>(proxyModel->sourceModel())
                     : qobject_cast<LogModel*>(model());

    if (!logModel)
    {
        qWarning() << "Unexpected model type in log view";
        return;
    }

    auto* filterModel = qobject_cast<LogFilterModel*>(model());

    // If the proxy model shows no rows while the source model still has data,
    // the filter hides everything — don't fetch more.
    if (model()->rowCount() == 0 && logModel->rowCount() > 0)
        return;

    int visibleRows = viewport()->height() / std::max(1, rowHeight(0));
    if (model()->rowCount() < visibleRows &&
        logModel->rowCount() > model()->rowCount())
        return;

    int min = sb->minimum();
    int max = sb->maximum();
    int value = sb->value();
    int range = max - min;

    if (range == 0 && logModel->rowCount() > 0 && filterModel &&
        !filterModel->exportFilter().isEmpty() && logModel->isFulled())
    {
        filterModel->ensureFilteredModel();
        return;
    }

    if (value - min >= range * 0.9 || max == 0)
        logModel->fetchDownMore();

    if (value - min <= range * 0.1 || max == 0)
        logModel->fetchUpMore();

    QT_SLOT_END
}

void LogView::handleFirstDataLoaded()
{
    QT_SLOT_BEGIN

    for (int i = 0; i < model()->columnCount() - 1; ++i)
        resizeColumnToContents(i);
    disconnect(model(), &QAbstractItemModel::modelReset, this, &LogView::handleFirstDataLoaded);

    QT_SLOT_END
}

void LogView::handleReset()
{
    QT_SLOT_BEGIN

    auto* logModel = qobject_cast<LogModel*>(model());
    auto* proxyModel = qobject_cast<QAbstractProxyModel*>(model());
    if (proxyModel)
        logModel = qobject_cast<LogModel*>(proxyModel->sourceModel());

    if (!logModel->canFetchUpMore())
    {
        scrollToTop();
    }
    else if (!logModel->canFetchDownMore())
    {
        scrollToBottom();
    }
    else
    {
        scrollTo(logModel->index(model()->rowCount() / 2, 0));
    }

    QT_SLOT_END
}

void LogView::handleFirstLineChangeStart()
{
    QT_SLOT_BEGIN

    auto* logModel = qobject_cast<LogModel*>(model());
    auto* proxyModel = qobject_cast<QAbstractProxyModel*>(model());
    if (proxyModel)
        logModel = qobject_cast<LogModel*>(proxyModel->sourceModel());

    lastScrollPosition = indexAt(QPoint{});
    if (proxyModel)
        lastScrollPosition = proxyModel->mapToSource(lastScrollPosition.value());

    QT_SLOT_END
}

void LogView::handleFirstLineRemoving(const QModelIndex& parent, int first, int last)
{
    QT_SLOT_BEGIN
    if (lastScrollPosition)
    {
        if (parent.isValid() || lastScrollPosition->row() < first)
            return;

        auto* logModel = qobject_cast<LogModel*>(model());
        auto* proxyModel = qobject_cast<QAbstractProxyModel*>(model());
        if (proxyModel)
            logModel = qobject_cast<LogModel*>(proxyModel->sourceModel());

        auto newIndex = logModel->index(lastScrollPosition->row() - last + first, lastScrollPosition->column());
        scrollTo(proxyModel->mapFromSource(newIndex), QAbstractItemView::ScrollHint::PositionAtTop);
        lastScrollPosition.reset();
    }
    else
    {
        qWarning() << "Last scroll position not set";
    }
    QT_SLOT_END
}

void LogView::handleFirstLineAddition(const QModelIndex& parent, int first, int last)
{
    QT_SLOT_BEGIN
    if (lastScrollPosition)
    {
        if (parent.isValid() || lastScrollPosition->row() < first)
            return;

        auto* logModel = qobject_cast<LogModel*>(model());
        auto* proxyModel = qobject_cast<QAbstractProxyModel*>(model());
        if (proxyModel)
            logModel = qobject_cast<LogModel*>(proxyModel->sourceModel());

        auto newIndex = logModel->index(lastScrollPosition->row() + last - first, lastScrollPosition->column());
        scrollTo(proxyModel->mapFromSource(newIndex), QAbstractItemView::ScrollHint::PositionAtTop);
        lastScrollPosition.reset();
    }
    else
    {
        qWarning() << "Last scroll position not set";
    }
    QT_SLOT_END
}

void LogView::scrollContentsBy(int dx, int dy)
{
    QTreeView::scrollContentsBy(dx, dy);

    if (dx != 0)
    {
        auto* header = qobject_cast<FilterHeader*>(this->header());
        if (header)
            header->scroll(dx);
    }
}
