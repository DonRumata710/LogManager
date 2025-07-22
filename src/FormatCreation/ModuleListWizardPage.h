#pragma once

#include <QWizardPage>
#include <QStringListModel>


namespace Ui {
class ModuleListWizardPage;
}

class ModuleListWizardPage : public QWizardPage
{
    Q_OBJECT

    Q_PROPERTY(QStringList modules READ stringList NOTIFY modulesChanged)

public:
    explicit ModuleListWizardPage(QWidget *parent = nullptr);
    ~ModuleListWizardPage();

    QStringList stringList() const;

signals:
    void modulesChanged();

    void handleError(const QString& message);

private slots:
    void on_bAdd_clicked();
    void on_bRemove_clicked();

private:
    void updateModules();

private:
    Ui::ModuleListWizardPage *ui;
    QStringListModel* model;
};
