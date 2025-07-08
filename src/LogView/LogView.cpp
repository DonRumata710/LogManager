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
}

void LogView::setLogModel(QAbstractItemModel* newModel)
{
    setModel(newModel);
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
