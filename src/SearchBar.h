#pragma once

#include <QDockWidget>
#include <QStringList>

namespace Ui {
class SearchBar;
}

class SearchBar : public QDockWidget
{
    Q_OBJECT

public:
    explicit SearchBar(QWidget *parent = nullptr);
    ~SearchBar();

signals:
    void localSearch(const QString& searchTerm, bool lastColumn, bool regexEnabled, bool backward, bool useFilters, bool findAll);
    void commonSearch(const QString& searchTerm, bool lastColumn, bool regexEnabled, bool backward, bool useFilters, bool findAll);

    void handleError(const QString& message);

private slots:
    void on_toolButton_clicked();

    void on_bFindNext_clicked();
    void on_bFindAll_clicked();

private:
    Ui::SearchBar *ui;
    QWidget* mWidget = nullptr;
};
