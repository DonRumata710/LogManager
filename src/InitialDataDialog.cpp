#include "InitialDataDialog.h"
#include "ui_InitialDataDialog.h"

#include "Utils.h"

#include <QPushButton>


InitialDataDialog::InitialDataDialog(const LogManager& manager, QWidget* parent) :
    QDialog(parent),
    ui(new Ui::InitialDataDialog)
{
    ui->setupUi(this);

    QDateTime minDateTime = DateTimeFromChronoSystemClock(manager.getMinTime());
    QDateTime maxDateTime = DateTimeFromChronoSystemClock(manager.getMaxTime());

    ui->startTimeEdit->setMinimumDateTime(minDateTime);
    ui->startTimeEdit->setMaximumDateTime(maxDateTime);
    ui->startTimeEdit->setDateTime(minDateTime);
    connect(ui->endTimeEdit, &QDateTimeEdit::dateTimeChanged, this, [this](const QDateTime& dateTime) {
        ui->startTimeEdit->setMaximumDateTime(dateTime);
    });

    ui->endTimeEdit->setMinimumDateTime(minDateTime);
    ui->endTimeEdit->setMaximumDateTime(maxDateTime);
    ui->endTimeEdit->setDateTime(maxDateTime);
    connect(ui->startTimeEdit, &QDateTimeEdit::dateTimeChanged, this, [this](const QDateTime& dateTime) {
        ui->endTimeEdit->setMinimumDateTime(dateTime);
    });

    for (const auto& module : manager.getModules())
    {
        QListWidgetItem* item = new QListWidgetItem(module, ui->modulesListWidget);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(Qt::Unchecked);
    }

    connect(ui->modulesListWidget, SIGNAL(itemChanged(QListWidgetItem*)), this, SLOT(onModulesSelectionChanged()));

    updateOkButtonState(false);
}

InitialDataDialog::~InitialDataDialog()
{
    delete ui;
}

QDateTime InitialDataDialog::getStartDate() const
{
    return ui->startTimeEdit->dateTime();
}

QDateTime InitialDataDialog::getEndDate() const
{
    return ui->endTimeEdit->dateTime();
}

std::unordered_set<QString> InitialDataDialog::getModules() const
{
    std::unordered_set<QString> checkedItems;
    for (int i = 0; i < ui->modulesListWidget->count(); ++i)
    {
        QListWidgetItem* item = ui->modulesListWidget->item(i);
        if (item->checkState() == Qt::Checked)
            checkedItems.emplace(item->text());
    }
    return checkedItems;
}

bool InitialDataDialog::scrollToEnd() const
{
    return ui->cbScrollToEnd->isChecked();
}

void InitialDataDialog::on_bSelect_clicked()
{
    QT_SLOT_BEGIN

    for (int i = 0; i < ui->modulesListWidget->count(); ++i)
    {
        QListWidgetItem* item = ui->modulesListWidget->item(i);
        item->setCheckState(Qt::CheckState::Checked);
    }

    QT_SLOT_END
}

void InitialDataDialog::on_bDeselect_clicked()
{
    QT_SLOT_BEGIN

    for (int i = 0; i < ui->modulesListWidget->count(); ++i)
    {
        QListWidgetItem* item = ui->modulesListWidget->item(i);
        item->setCheckState(Qt::CheckState::Unchecked);
    }

    QT_SLOT_END
}

void InitialDataDialog::onModulesSelectionChanged()
{
    QT_SLOT_BEGIN

    bool flag = false;
    for (int i = 0; i < ui->modulesListWidget->count(); ++i)
    {
        QListWidgetItem* item = ui->modulesListWidget->item(i);
        if (item->checkState() == Qt::Checked)
            flag = true;
    }

    updateOkButtonState(flag);

    QT_SLOT_END
}

void InitialDataDialog::updateOkButtonState(bool state)
{
    ui->buttonBox->button(QDialogButtonBox::StandardButton::Ok)->setEnabled(state);
}
