#include "InitialDataDialog.h"
#include "ui_InitialDataDialog.h"

#include "Utils.h"

#include <QPushButton>


InitialDataDialog::InitialDataDialog(const LogManager::ScanResult& scanResult, QWidget* parent) :
    QDialog(parent),
    ui(new Ui::InitialDataDialog)
{
    ui->setupUi(this);

    auto minMs = std::chrono::duration_cast<std::chrono::milliseconds>(scanResult.minTime.time_since_epoch()).count();
    QDateTime minDateTime = QDateTime::fromMSecsSinceEpoch(minMs, Qt::UTC);

    auto maxMs = std::chrono::duration_cast<std::chrono::milliseconds>(scanResult.maxTime.time_since_epoch()).count();
    QDateTime maxDateTime = QDateTime::fromMSecsSinceEpoch(maxMs, Qt::UTC);

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

    for (const auto& module : scanResult.modules)
    {
        QListWidgetItem* item = new QListWidgetItem(module, ui->modulesListWidget);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(Qt::Unchecked);
        connect(ui->modulesListWidget, SIGNAL(itemChanged(QListWidgetItem*)), this, SLOT(onModulesSelectionChanged()));
    }

    updateOkButtonState(false);
}

InitialDataDialog::~InitialDataDialog()
{
    delete ui;
}

std::chrono::system_clock::time_point InitialDataDialog::getStartDate() const
{
    qint64 ms_since_epoch = ui->startTimeEdit->dateTime().toMSecsSinceEpoch();
    return std::chrono::system_clock::time_point(std::chrono::milliseconds(ms_since_epoch));
}

std::chrono::system_clock::time_point InitialDataDialog::getEndDate() const
{
    qint64 ms_since_epoch = ui->endTimeEdit->dateTime().toMSecsSinceEpoch();
    return std::chrono::system_clock::time_point(std::chrono::milliseconds(ms_since_epoch));
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
