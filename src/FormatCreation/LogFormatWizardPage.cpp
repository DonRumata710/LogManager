#include "LogFormatWizardPage.h"
#include "ui_LogFormatWizardPage.h"


LogFormatWizardPage::LogFormatWizardPage(QWidget *parent) :
    QWizardPage(parent),
    ui(new Ui::LogFormatWizardPage)
{
    ui->setupUi(this);

    registerField("separatorUsage", ui->rbSeparator);
    registerField("separator*", ui->leSeparator);
    registerField("lineRegexUsage", ui->rbRegex);
    registerField("lineRegex*", ui->leLineRegex);
    registerField("timeFieldIndex*", ui->sbTimeFieldIndex);
    registerField("timeMask*", ui->letimeMask);
    registerField("timeFractionalDigits", ui->sbTimeFieldIndex);

    connect(ui->leSeparator, &QLineEdit::textChanged, this, &LogFormatWizardPage::completeChanged);
    connect(ui->letimeMask, &QLineEdit::textChanged, this, &LogFormatWizardPage::completeChanged);
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
    return !ui->leSeparator->text().isEmpty() && !ui->letimeMask->text().isEmpty();
}
