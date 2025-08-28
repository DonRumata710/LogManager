#include "SearchResultsWidget.h"
#include <QListWidget>

#include "Utils.h"


SearchResultsWidget::SearchResultsWidget(QWidget* parent) :
    QDockWidget(parent)
{
    mResults = new QListWidget(this);
    setWidget(mResults);
    hide();
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

