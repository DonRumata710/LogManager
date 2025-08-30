#include "SearchBar.h"
#include "ui_SearchBar.h"

#include "Utils.h"
#include <QToolButton>


QPixmap rotatePixmap(const QPixmap& pixmap, qreal angle)
{
    QTransform transform;
    transform.rotate(angle);
    return pixmap.transformed(transform, Qt::SmoothTransformation);
}

SearchBar::SearchBar(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SearchBar)
{
    ui->setupUi(this);
    ui->extendedSearchWidget->hide();
    ui->sbColumn->setEnabled(false);
}

SearchBar::~SearchBar()
{
    delete ui;
}

void SearchBar::handleColumnCountChanged(int count)
{
    QT_SLOT_BEGIN

    if (ui->sbColumn->value() > count - 1)
        ui->sbColumn->setValue(count - 1);
    ui->sbColumn->setMaximum(count - 1);

    QT_SLOT_END
}

void SearchBar::on_toolButton_clicked()
{
    QT_SLOT_BEGIN

    auto pixmap = QPixmap(":/LogManager/right-arrow.png");
    if (ui->extendedSearchWidget->isVisible())
    {
        ui->extendedSearchWidget->hide();
        ui->toolButton->setIcon(pixmap);
    }
    else
    {
        ui->extendedSearchWidget->show();
        ui->toolButton->setIcon(rotatePixmap(pixmap, 90));
    }

    QT_SLOT_END
}

void SearchBar::on_bFindNext_clicked()
{
    QT_SLOT_BEGIN

    triggerSearch(false);

    QT_SLOT_END
}


void SearchBar::on_bFindAll_clicked()
{
    QT_SLOT_BEGIN

    triggerSearch(true);

    QT_SLOT_END
}

void SearchBar::on_cbSpecificColumn_toggled(bool checked)
{
    QT_SLOT_BEGIN

    ui->sbColumn->setEnabled(checked);

    QT_SLOT_END
}

void SearchBar::triggerSearch(bool findAll)
{
    QString text = ui->lineEdit->text();
    bool regexSupport = ui->cbRegExpSuppor->isChecked();
    bool backward = ui->cbBackward->isChecked();
    bool useFilters = ui->cbUseFilters->isChecked();
    bool global = ui->cbGlobalSearch->isChecked();
    int column = ui->cbSpecificColumn->isChecked() ? ui->sbColumn->value() : -1;
    emit search(text, regexSupport, backward, useFilters, global, findAll, column);
}

