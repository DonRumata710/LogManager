#include "SearchResultsWidget.h"

#include "Utils.h"
#include "EntryLinkModel.h"

#include <QListView>
#include <QHideEvent>


SearchResultsWidget::SearchResultsWidget(QWidget* parent) :
    QDockWidget(parent)
{
    mResults = new EntryLinkView(this);

    QSizePolicy sizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);
    sizePolicy.setHorizontalStretch(1);
    sizePolicy.setVerticalStretch(1);
    mResults->setSizePolicy(sizePolicy);

    setWidget(mResults);
    hide();

    setAllowedAreas(Qt::DockWidgetArea::TopDockWidgetArea | Qt::DockWidgetArea::BottomDockWidgetArea);

    connect(mResults, &EntryLinkView::entryActivated, this, &SearchResultsWidget::selectedResult);
}

void SearchResultsWidget::clearResults()
{
    mResults->setModel(nullptr);
}

void SearchResultsWidget::showResults(const QMap<std::chrono::system_clock::time_point, QString>& results)
{
    QT_SLOT_BEGIN

    if (results.isEmpty())
    {
        resetModel(nullptr);
        hide();
    }
    else
    {
        resetModel(new EntryLinkModel(results, this));
        show();
    }

    QT_SLOT_END
}

void SearchResultsWidget::hideEvent(QHideEvent* event)
{
    QDockWidget::hideEvent(event);
    clearResults();
}

void SearchResultsWidget::resetModel(EntryLinkModel* newModel)
{
    auto oldModel = mResults->model();
    mResults->setModel(newModel);

    if (oldModel)
        oldModel->deleteLater();
}
