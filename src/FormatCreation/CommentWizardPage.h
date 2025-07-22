#pragma once

#include <QWizardPage>

namespace Ui {
class CommentWizardPage;
}

class CommentWizardPage : public QWizardPage
{
    Q_OBJECT

    Q_PROPERTY(QVariantList comments READ getComments NOTIFY commentsChanged)

public:
    explicit CommentWizardPage(QWidget *parent = nullptr);
    ~CommentWizardPage();

    QVariantList getComments() const;

signals:
    void commentsChanged();

    void handleError(const QString& message);

private slots:
    void on_bAdd_clicked();

    void on_bRemove_clicked();

private:
    void updateComments();

private:
    Ui::CommentWizardPage *ui;
};
