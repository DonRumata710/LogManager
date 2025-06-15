#pragma once

#include <QWizardPage>


namespace Ui {
class FilenameFormatWizardPage;
}

class FilenameFormatWizardPage : public QWizardPage
{
    Q_OBJECT

public:
    explicit FilenameFormatWizardPage(QWidget *parent = nullptr);
    ~FilenameFormatWizardPage();

    bool isComplete() const;

private:
    Ui::FilenameFormatWizardPage *ui;
};
