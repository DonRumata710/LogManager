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
    void search(const QString& searchTerm, bool regexEnabled, bool backward, bool useFilters, bool findAll, bool global, int column);

    void handleError(const QString& message);

public slots:
    void handleColumnCountChanged(int count);

private slots:
    void on_toolButton_clicked();

    void on_bFindNext_clicked();
    void on_bFindAll_clicked();

    void on_cbSpecificColumn_toggled(bool checked);

private:
    void triggerSearch(bool findAll);

private:
    Ui::SearchBar *ui;
};
