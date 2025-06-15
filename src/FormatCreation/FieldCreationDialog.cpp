#include "FieldCreationDialog.h"
#include "ui_FieldCreationDialog.h"

#include <QPushButton>


FieldCreationDialog::FieldCreationDialog(QWidget *parent) :
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
    connect(ui->leFieldName, &QLineEdit::textChanged, this, [this](const QString& text) {
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(!text.isEmpty());
    });
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
    return field;
}
