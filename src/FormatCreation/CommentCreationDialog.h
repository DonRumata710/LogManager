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

signals:
    void handleError(const QString& message);

private:
    Ui::CommentCreationDialog *ui;
};
