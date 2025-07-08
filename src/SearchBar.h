#pragma once

#include <QWidget>

namespace Ui {
class SearchBar;
}

class SearchBar : public QWidget
{
    Q_OBJECT

public:
    explicit SearchBar(QWidget *parent = nullptr);
    ~SearchBar();

private slots:
    void on_toolButton_clicked();

private:
    Ui::SearchBar *ui;
};
