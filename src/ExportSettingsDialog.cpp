#include "ExportSettingsDialog.h"
#include "ui_ExportSettingsDialog.h"

#include "Settings.h"


ExportSettingsDialog::ExportSettingsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ExportSettingsDialog)
{
    ui->setupUi(this);

    Settings settings;
    ui->cbFilters->setChecked(settings.value("export/useFilters", false).toBool());
    ui->cbCsv->setChecked(settings.value("export/csvFormat", false).toBool());
}

ExportSettingsDialog::~ExportSettingsDialog()
{
    Settings settings;
    settings.setValue("export/useFilters", ui->cbFilters->isChecked());
    settings.setValue("export/csvFormat", ui->cbCsv->isChecked());

    delete ui;
}

bool ExportSettingsDialog::useFilters() const
{
    return ui->cbFilters->isChecked();
}

bool ExportSettingsDialog::csvFormat() const
{
    return ui->cbCsv->isChecked();
}
