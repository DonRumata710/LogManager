#include "LogFieldsWizardPage.h"
#include "ui_LogFieldsWizardPage.h"

#include "FieldCreationDialog.h"
#include "../Utils.h"


LogFieldsWizardPage::LogFieldsWizardPage(QWidget *parent) :
    QWizardPage(parent),
    ui(new Ui::LogFieldsWizardPage)
{
    ui->setupUi(this);

    ui->tableWidget->setColumnCount(4);
    ui->tableWidget->setHorizontalHeaderLabels({ "Name", "Regex", "Type", "Optional" });
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    registerField("fields", this, "fields", SIGNAL(fieldsChanged()));
}

LogFieldsWizardPage::~LogFieldsWizardPage()
{
    delete ui;
}

bool LogFieldsWizardPage::isComplete() const
{
    return ui->tableWidget->rowCount() > 1;
}

QVariantList LogFieldsWizardPage::getFields() const
{
    QVariantList fields;
    for (int i = 0; i < ui->tableWidget->rowCount(); ++i)
    {
        QVariantMap fieldData;
        fieldData["name"] = ui->tableWidget->item(i, 0)->text();
        fieldData["regex"] = ui->tableWidget->item(i, 1)->text();
        fieldData["type"] = static_cast<int>(QMetaType::fromName(ui->tableWidget->item(i, 2)->text().toLatin1()).id());
        fieldData["optional"] = ui->tableWidget->item(i, 3)->text() == "Yes";
        fields.append(fieldData);
    }
    return fields;
}

void LogFieldsWizardPage::on_bAdd_clicked()
{
    QT_SLOT_BEGIN

    FieldCreationDialog dialog(this);
    connect(&dialog, &FieldCreationDialog::handleError, this, &LogFieldsWizardPage::handleError);
    if (dialog.exec() == QDialog::Accepted)
    {
        int row = ui->tableWidget->rowCount();
        ui->tableWidget->insertRow(row);

        QTableWidgetItem* nameItem = new QTableWidgetItem(dialog.getField().name);
        QTableWidgetItem* regexItem = new QTableWidgetItem(dialog.getField().regex.pattern());
        QTableWidgetItem* typeItem = new QTableWidgetItem(QMetaType::typeName(dialog.getField().type));
        QTableWidgetItem* optionalItem = new QTableWidgetItem(dialog.getField().isOptional ? "Yes" : "No");

        ui->tableWidget->setItem(row, 0, nameItem);
        ui->tableWidget->setItem(row, 1, regexItem);
        ui->tableWidget->setItem(row, 2, typeItem);
        ui->tableWidget->setItem(row, 3, optionalItem);

        updateFields();
    }

    QT_SLOT_END
}


void LogFieldsWizardPage::on_bRemove_clicked()
{
    QT_SLOT_BEGIN

    int row = ui->tableWidget->currentRow();
    if (row >= 0)
        ui->tableWidget->removeRow(row);

    QT_SLOT_END
}

void LogFieldsWizardPage::updateFields()
{
    emit fieldsChanged();
    emit completeChanged();
}

