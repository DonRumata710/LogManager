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

    QDateTime getStartDate() const;
    QDateTime getEndDate() const;
    std::unordered_set<QString> getModules() const;
    bool scrollToEnd() const;

private slots:
    void on_bSelect_clicked();
    void on_bDeselect_clicked();

    void onModulesSelectionChanged();

private:
    void updateOkButtonState(bool state);

private:
    Ui::InitialDataDialog *ui;
};
