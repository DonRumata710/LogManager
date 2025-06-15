#include "FilenameFormatWizardPage.h"
#include "ui_FilenameFormatWizardPage.h"


FilenameFormatWizardPage::FilenameFormatWizardPage(QWidget *parent) :
    QWizardPage(parent),
    ui(new Ui::FilenameFormatWizardPage)
{
    ui->setupUi(this);

    ui->cbEncoding->addItem("UTF-8");
    ui->cbEncoding->addItem("UTF-16");
    ui->cbEncoding->addItem("UTF-16BE");
    ui->cbEncoding->addItem("UTF-16LE");
    ui->cbEncoding->addItem("UTF-32");
    ui->cbEncoding->addItem("UTF-32BE");
    ui->cbEncoding->addItem("UTF-32LE");

    registerField("name*", ui->leName);
    registerField("logFileRegex*", ui->leFilenameRegex);
    registerField("extension*", ui->leExtension);
    registerField("encoding", ui->cbEncoding, "currentText", SIGNAL(currentTextChanged(QString)));
}

FilenameFormatWizardPage::~FilenameFormatWizardPage()
{
    delete ui;
}

bool FilenameFormatWizardPage::isComplete() const
{
    return !ui->leName->text().isEmpty() && !ui->leFilenameRegex->text().isEmpty() && !ui->leExtension->text().isEmpty() && !ui->cbEncoding->currentText().isEmpty();
}
