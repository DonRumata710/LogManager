#include "CommentWizardPage.h"
#include "ui_CommentWizardPage.h"

#include "Utils.h"
#include "CommentCreationDialog.h"


CommentWizardPage::CommentWizardPage(QWidget *parent) :
    QWizardPage(parent),
    ui(new Ui::CommentWizardPage)
{
    ui->setupUi(this);

    ui->tableWidget->setColumnCount(2);
    ui->tableWidget->setHorizontalHeaderLabels({"Start", "Finish"});
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    registerField("comments", this, "comments", SIGNAL(commentsChanged()));
}

CommentWizardPage::~CommentWizardPage()
{
    delete ui;
}

QVariantList CommentWizardPage::getComments() const
{
    QVariantList comments;
    for (int i = 0; i < ui->tableWidget->rowCount(); ++i)
    {
        QVariantMap commentData;
        commentData["start"] = ui->tableWidget->item(i, 0)->text();
        if (ui->tableWidget->item(i, 1))
            commentData["finish"] = ui->tableWidget->item(i, 1)->text();
        comments.append(commentData);
    }
    return comments;
}

void CommentWizardPage::on_bAdd_clicked()
{
    QT_SLOT_BEGIN

    CommentCreationDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted)
    {
        int row = ui->tableWidget->rowCount();
        ui->tableWidget->insertRow(row);

        ui->tableWidget->setItem(row, 0, new QTableWidgetItem(dialog.getComment().start));
        if (dialog.getComment().finish)
            ui->tableWidget->setItem(row, 1, new QTableWidgetItem(dialog.getComment().finish.value()));
    }
    updateComments();

    QT_SLOT_END
}

void CommentWizardPage::on_bRemove_clicked()
{
    QT_SLOT_BEGIN

    auto index = ui->tableWidget->currentIndex();
    if (!index.isValid())
    {
        qWarning() << "No comment selected for removal.";
        return;
    }

    ui->tableWidget->removeRow(index.row());

    QT_SLOT_END
}

void CommentWizardPage::updateComments()
{
    emit commentsChanged();
    emit completeChanged();
}

