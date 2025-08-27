#pragma once

#include <QDialog>

namespace Ui {
class ExportSettingsDialog;
}

class ExportSettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ExportSettingsDialog(QWidget *parent = nullptr);
    ~ExportSettingsDialog();

    bool useFilters() const;
    bool csvFormat() const;

private:
    Ui::ExportSettingsDialog *ui;
};
