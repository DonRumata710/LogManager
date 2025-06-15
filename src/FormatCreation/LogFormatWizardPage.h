#pragma once

#include <QWizardPage>

namespace Ui {
class LogFormatWizardPage;
}

class LogFormatWizardPage : public QWizardPage
{
    Q_OBJECT

public:
    explicit LogFormatWizardPage(QWidget *parent = nullptr);
    ~LogFormatWizardPage();

    void initializePage() override;
    bool isComplete() const override;

private:
    Ui::LogFormatWizardPage *ui;
};
