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

signals:
    void handleError(const QString& message);

private:
    Ui::FilenameFormatWizardPage *ui;
};
