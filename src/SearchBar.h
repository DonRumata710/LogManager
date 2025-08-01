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

signals:
    void localSearch(const QString& searchTerm, bool lastColumn, bool regexEnabled, bool backward, bool useFilters);
    void commonSearch(const QString& searchTerm, bool lastColumn, bool regexEnabled, bool backward, bool useFilters);

    void handleError(const QString& message);

private slots:
    void on_toolButton_clicked();

    void on_bLocalSearch_clicked();
    void on_bCommonSearch_clicked();

private:
    Ui::SearchBar *ui;
};
