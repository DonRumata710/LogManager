#include "SearchResultsWidget.h"

#include "Utils.h"

#include <QListWidget>
#include <QHideEvent>


SearchResultsWidget::SearchResultsWidget(QWidget* parent) :
    QDockWidget(parent)
{
    mResults = new QListWidget(this);

    QSizePolicy sizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);
    sizePolicy.setHorizontalStretch(1);
    sizePolicy.setVerticalStretch(1);
    mResults->setSizePolicy(sizePolicy);

    setWidget(mResults);
    hide();

    setAllowedAreas(Qt::DockWidgetArea::TopDockWidgetArea | Qt::DockWidgetArea::BottomDockWidgetArea);
}

void SearchResultsWidget::clearResults()
{
    mResults->clear();
}

void SearchResultsWidget::showResults(const QStringList& results)
{
    QT_SLOT_BEGIN

    mResults->clear();
    if (results.isEmpty())
    {
        hide();
    }
    else
    {
        mResults->addItems(results);
        show();
    }

    QT_SLOT_END
}

void SearchResultsWidget::hideEvent(QHideEvent* event)
{
    QDockWidget::hideEvent(event);
    clearResults();
}
