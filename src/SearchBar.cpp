#include "SearchBar.h"
#include "ui_SearchBar.h"

#include "Utils.h"


QPixmap rotatePixmap(const QPixmap &pixmap, qreal angle)
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
}

SearchBar::~SearchBar()
{
    delete ui;
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


void SearchBar::on_bLocalSearch_clicked()
{
    QT_SLOT_BEGIN

    localSearch(ui->lineEdit->text(), ui->cbLastColumnSearch->isChecked(), ui->cbRegExpSuppor->isChecked(), ui->cbBackward->isChecked());

    QT_SLOT_END
}


void SearchBar::on_bCommonSearch_clicked()
{
    QT_SLOT_BEGIN

    commonSearch(ui->lineEdit->text(), ui->cbLastColumnSearch->isChecked(), ui->cbRegExpSuppor->isChecked(), ui->cbBackward->isChecked());

    QT_SLOT_END
}

