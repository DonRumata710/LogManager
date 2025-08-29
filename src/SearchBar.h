#pragma once

#include <QDockWidget>
#include <QStringList>

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
    void localSearch(const QString& searchTerm, bool regexEnabled, bool backward, bool useFilters, bool findAll, int column);
    void commonSearch(const QString& searchTerm, bool regexEnabled, bool backward, bool useFilters, bool findAll, int column);

    void handleError(const QString& message);

public slots:
    void handleColumnCountChanged(int count);

private slots:
    void on_toolButton_clicked();

    void on_bFindNext_clicked();
    void on_bFindAll_clicked();

    void on_cbSpecificColumn_toggled(bool checked);

private:
    Ui::SearchBar *ui;
};
