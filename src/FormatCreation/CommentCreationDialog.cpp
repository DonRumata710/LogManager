#include "CommentCreationDialog.h"
#include "ui_CommentCreationDialog.h"


CommentCreationDialog::CommentCreationDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CommentCreationDialog)
{
    ui->setupUi(this);
}

CommentCreationDialog::~CommentCreationDialog()
{
    delete ui;
}

Format::Comment CommentCreationDialog::getComment() const
{
    Format::Comment comment;
    comment.start = ui->leStart->text();
    if (!ui->leEnd->text().isEmpty())
        comment.finish = ui->leEnd->text();
    return comment;
}
