#pragma once

#include "LogManagement/LogManager.h"

#include <QDialog>

#include <unordered_set>


namespace Ui {
class InitialDataDialog;
}

class InitialDataDialog : public QDialog
{
    Q_OBJECT

public:
    explicit InitialDataDialog(const LogManager&, QWidget *parent = nullptr);
    ~InitialDataDialog();

    std::chrono::system_clock::time_point getStartDate() const;
    std::chrono::system_clock::time_point getEndDate() const;
    std::unordered_set<QString> getModules() const;

private slots:
    void on_bSelect_clicked();
    void on_bDeselect_clicked();

    void onModulesSelectionChanged();

private:
    void updateOkButtonState(bool state);

private:
    Ui::InitialDataDialog *ui;
};
