#include "ModuleListWizardPage.h"
#include "ui_ModuleListWizardPage.h"

#include "../Utils.h"

#include <QInputDialog>


ModuleListWizardPage::ModuleListWizardPage(QWidget *parent) :
    QWizardPage(parent),
    ui(new Ui::ModuleListWizardPage),
    model(new QStringListModel(this))
{
    ui->setupUi(this);
    ui->listView->setModel(model);

    connect(model, &QStringListModel::dataChanged, this, &ModuleListWizardPage::updateModules);

    registerField("modules", this, "modules", SIGNAL(modulesChanged()));
}

ModuleListWizardPage::~ModuleListWizardPage()
{
    delete ui;
}

QStringList ModuleListWizardPage::stringList() const
{
    return model->stringList();
}

void ModuleListWizardPage::on_bAdd_clicked()
{
    QT_SLOT_BEGIN

    auto module = QInputDialog::getText(this, tr("Input module name"), tr("Module name"));
    if (module.isEmpty())
        return;

    QStringList modules = model->stringList();
    if (modules.contains(module))
        qWarning() << QString("The module '%1' is already in the list.").arg(module);

    modules.append(module);
    model->setStringList(modules);

    updateModules();

    QT_SLOT_END
}

void ModuleListWizardPage::on_bRemove_clicked()
{
    QT_SLOT_BEGIN

    auto index = ui->listView->currentIndex();
    if (!index.isValid())
    {
        qWarning() << "No module selected for removal.";
        return;
    }

    QStringList modules = model->stringList();
    modules.removeAt(index.row());
    model->setStringList(modules);

    updateModules();

    QT_SLOT_END
}

void ModuleListWizardPage::updateModules()
{
    emit modulesChanged();
    emit completeChanged();
}

