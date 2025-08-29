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

    bool global = !ui->cbGlobalSearch->isChecked();
    if (global)
        emit commonSearch(ui->lineEdit->text(), ui->cbRegExpSuppor->isChecked(), ui->cbBackward->isChecked(), ui->cbUseFilters->isChecked(), false, ui->cbSpecificColumn->isChecked() ? ui->sbColumn->value() : -1);
    else
        emit localSearch(ui->lineEdit->text(), ui->cbRegExpSuppor->isChecked(), ui->cbBackward->isChecked(), ui->cbUseFilters->isChecked(), false, ui->cbSpecificColumn->isChecked() ? ui->sbColumn->value() : -1);

    QT_SLOT_END
}


void SearchBar::on_bFindAll_clicked()
{
    QT_SLOT_BEGIN

    bool global = !ui->cbGlobalSearch->isChecked();
    if (global)
        emit commonSearch(ui->lineEdit->text(), ui->cbRegExpSuppor->isChecked(), ui->cbBackward->isChecked(), ui->cbUseFilters->isChecked(), true, ui->cbSpecificColumn->isChecked() ? ui->sbColumn->value() : -1);
    else
        emit localSearch(ui->lineEdit->text(), ui->cbRegExpSuppor->isChecked(), ui->cbBackward->isChecked(), ui->cbUseFilters->isChecked(), true, ui->cbSpecificColumn->isChecked() ? ui->sbColumn->value() : -1);

    QT_SLOT_END
}

void SearchBar::on_cbSpecificColumn_toggled(bool checked)
{
    QT_SLOT_BEGIN

    ui->sbColumn->setEnabled(checked);

    QT_SLOT_END
}

