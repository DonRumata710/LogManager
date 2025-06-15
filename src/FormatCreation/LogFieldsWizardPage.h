#pragma once

#include <QWizardPage>


namespace Ui {
class LogFieldsWizardPage;
}

class LogFieldsWizardPage : public QWizardPage
{
    Q_OBJECT

    Q_PROPERTY(QVariantList fields READ getFields NOTIFY fieldsChanged)

public:
    explicit LogFieldsWizardPage(QWidget *parent = nullptr);
    ~LogFieldsWizardPage();

    bool isComplete() const override;
    QVariantList getFields() const;

signals:
    void fieldsChanged();

private slots:
    void on_bAdd_clicked();

    void on_bRemove_clicked();

private:
    void updateFields();

private:
    Ui::LogFieldsWizardPage *ui;
};
