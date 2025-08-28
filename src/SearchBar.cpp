#include "SearchBar.h"
#include "ui_SearchBar.h"

#include "Utils.h"
#include <QToolButton>


QPixmap rotatePixmap(const QPixmap &pixmap, qreal angle)
{
    QTransform transform;
    transform.rotate(angle);
    return pixmap.transformed(transform, Qt::SmoothTransformation);
}

SearchBar::SearchBar(QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::SearchBar)
{
    mWidget = new QWidget(this);
    ui->setupUi(mWidget);
    setWidget(mWidget);

    ui->extendedSearchWidget->hide();

    connect(ui->toolButton, &QToolButton::clicked, this, &SearchBar::on_toolButton_clicked);
    connect(ui->bFindNext, &QToolButton::clicked, this, &SearchBar::on_bFindNext_clicked);
    connect(ui->bFindAll, &QToolButton::clicked, this, &SearchBar::on_bFindAll_clicked);
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

void SearchBar::on_bFindNext_clicked()
{
    QT_SLOT_BEGIN

    bool local = ui->cbSearchMode->currentIndex() == 0;
    if (local)
        emit localSearch(ui->lineEdit->text(), ui->cbLastColumnSearch->isChecked(), ui->cbRegExpSuppor->isChecked(), ui->cbBackward->isChecked(), ui->cbUseFilters->isChecked(), false);
    else
        emit commonSearch(ui->lineEdit->text(), ui->cbLastColumnSearch->isChecked(), ui->cbRegExpSuppor->isChecked(), ui->cbBackward->isChecked(), ui->cbUseFilters->isChecked(), false);

    QT_SLOT_END
}


void SearchBar::on_bFindAll_clicked()
{
    QT_SLOT_BEGIN

    bool local = ui->cbSearchMode->currentIndex() == 0;
    if (local)
        emit localSearch(ui->lineEdit->text(), ui->cbLastColumnSearch->isChecked(), ui->cbRegExpSuppor->isChecked(), ui->cbBackward->isChecked(), ui->cbUseFilters->isChecked(), true);
    else
        emit commonSearch(ui->lineEdit->text(), ui->cbLastColumnSearch->isChecked(), ui->cbRegExpSuppor->isChecked(), ui->cbBackward->isChecked(), ui->cbUseFilters->isChecked(), true);

    QT_SLOT_END
}

