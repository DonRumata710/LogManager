#include "LogView.h"

#include "FilterHeader.h"
#include "LogModel.h"
#include "../Utils.h"

#include <QScrollBar>
#include <QAbstractProxyModel>


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
    setModel(newModel);

    auto* logModel = qobject_cast<LogModel*>(newModel);
    auto* proxyModel = qobject_cast<QAbstractProxyModel*>(newModel);
    if (!logModel && proxyModel)
        logModel = qobject_cast<LogModel*>(proxyModel->sourceModel());

    if (!logModel)
    {
        qWarning() << "Unexpected model type in log view";
        return;
    }

    connect(logModel, &LogModel::modelReset, this, &LogView::handleReset);
    connect(logModel, &LogModel::modelReset, this, &LogView::handleFirstDataLoaded);

    connect(logModel, &LogModel::rowsAboutToBeInserted, this, &LogView::handleFirstLineChangeStart);
    connect(logModel, &LogModel::rowsAboutToBeRemoved, this, &LogView::handleFirstLineChangeStart);
    connect(logModel, &LogModel::rowsInserted, this, &LogView::handleFirstLineAddition);
    connect(logModel, &LogModel::rowsRemoved, this, &LogView::handleFirstLineRemoving);
}

void LogView::checkFetchNeeded()
{
    QT_SLOT_BEGIN

    QScrollBar* sb = verticalScrollBar();
    int min = sb->minimum();
    int max = sb->maximum();
    int value = sb->value();
    int range = max - min;

    auto* logModel = qobject_cast<LogModel*>(model());
    auto* proxyModel = qobject_cast<QAbstractProxyModel*>(model());
    if (proxyModel)
        logModel = qobject_cast<LogModel*>(proxyModel->sourceModel());

    if (!logModel || logModel->rowCount() == 0)
    {
        qWarning() << "Unexpected model type in log view";
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
