#pragma once

#include "../LogManagement/Format.h"

#include <QDialog>


namespace Ui {
class CommentCreationDialog;
}

class CommentCreationDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CommentCreationDialog(QWidget *parent = nullptr);
    ~CommentCreationDialog();

    Format::Comment getComment() const;

private:
    Ui::CommentCreationDialog *ui;
};
