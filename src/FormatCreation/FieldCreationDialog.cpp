#include "FieldCreationDialog.h"
#include "ui_FieldCreationDialog.h"

#include <QPushButton>


FieldCreationDialog::FieldCreationDialog(QWidget* parent) :
    QDialog(parent),
    ui(new Ui::FieldCreationDialog)
{
    ui->setupUi(this);

    ui->cbType->addItem("Boolean", static_cast<int>(QMetaType::Bool));
    ui->cbType->addItem("Integer", static_cast<int>(QMetaType::Int));
    ui->cbType->addItem("Unsigned Integer", static_cast<int>(QMetaType::UInt));
    ui->cbType->addItem("Float", static_cast<int>(QMetaType::Double));
    ui->cbType->addItem("String", static_cast<int>(QMetaType::QString));
    ui->cbType->addItem("DateTime", static_cast<int>(QMetaType::QDateTime));
    ui->cbType->setCurrentIndex(4);

    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    connect(ui->leFieldName, &QLineEdit::textChanged, this, &FieldCreationDialog::updateOkButtonState);
    connect(ui->leRegex, &QLineEdit::textChanged, this, &FieldCreationDialog::updateOkButtonState);
    connect(ui->bOptional, &QPushButton::clicked, this, &FieldCreationDialog::updateOkButtonState);
}

FieldCreationDialog::~FieldCreationDialog()
{
    delete ui;
}

Format::Field FieldCreationDialog::getField() const
{
    Format::Field field;
    field.name = ui->leFieldName->text();
    field.type = static_cast<QMetaType::Type>(ui->cbType->currentData().toInt());
    field.regex = QRegularExpression{ ui->leRegex->text() };
    field.isOptional = ui->bOptional->isChecked();
    return field;
}

void FieldCreationDialog::updateOkButtonState()
{
    bool isValid = !ui->leFieldName->text().isEmpty();
    if (isValid && ui->bOptional->isChecked() && (ui->leRegex->text().isEmpty() || ui->leRegex->text() == ".*"))
    {
        ui->lWarning->setText("Regex is empty or matches everything, which may lead to unexpected results.");
        isValid = false;
    }
    else
    {
        ui->lWarning->setText(QString{});
    }

    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(isValid);
}

