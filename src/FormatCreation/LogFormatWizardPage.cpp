#include "LogFormatWizardPage.h"
#include "ui_LogFormatWizardPage.h"


LogFormatWizardPage::LogFormatWizardPage(QWidget *parent) :
    QWizardPage(parent),
    ui(new Ui::LogFormatWizardPage)
{
    ui->setupUi(this);

    registerField("separator*", ui->leSeparator);
    registerField("timeFieldIndex*", ui->sbTimeFieldIndex);
    registerField("timeRegex*", ui->leTimeRegex);

    connect(ui->leSeparator, &QLineEdit::textChanged, this, &LogFormatWizardPage::completeChanged);
    connect(ui->leTimeRegex, &QLineEdit::textChanged, this, &LogFormatWizardPage::completeChanged);
}

LogFormatWizardPage::~LogFormatWizardPage()
{
    delete ui;
}

void LogFormatWizardPage::initializePage()
{
    auto fieldList = field("fields");
    if (!fieldList.isNull() && fieldList.isValid() && fieldList.canConvert<QVariantList>())
    {
        ui->sbTimeFieldIndex->setMaximum(fieldList.toList().size() - 1);
    }
}

bool LogFormatWizardPage::isComplete() const
{
    return !ui->leSeparator->text().isEmpty() && !ui->leTimeRegex->text().isEmpty();
}
