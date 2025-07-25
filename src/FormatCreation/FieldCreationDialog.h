#pragma once

#include "../LogManagement/Format.h"

#include <QDialog>


namespace Ui {
class FieldCreationDialog;
}

class FieldCreationDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FieldCreationDialog(QWidget* parent = nullptr);
    ~FieldCreationDialog();

    Format::Field getField() const;

signals:
    void handleError(const QString& message);

private slots:
    void updateOkButtonState();

private:
    Ui::FieldCreationDialog *ui;
};
