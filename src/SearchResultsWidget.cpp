#include "SearchResultsWidget.h"
#include "ui_SearchResultsWidget.h"

#include "Utils.h"


SearchResultsWidget::SearchResultsWidget(QWidget* parent) :
    QWidget(parent),
    ui(new Ui::SearchResultsWidget)
{
    ui->setupUi(this);
    hide();
}

SearchResultsWidget::~SearchResultsWidget()
{
    delete ui;
}

void SearchResultsWidget::showResults(const QStringList& results)
{
    QT_SLOT_BEGIN

    ui->lwResults->clear();
    if (results.isEmpty())
    {
        hide();
    }
    else
    {
        ui->lwResults->addItems(results);
        show();
    }

    QT_SLOT_END
}

void SearchResultsWidget::on_bHideResults_clicked()
{
    QT_SLOT_BEGIN
    hide();
    QT_SLOT_END
}

