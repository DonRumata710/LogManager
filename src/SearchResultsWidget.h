#pragma once

#include <QWidget>
#include <QStringList>

namespace Ui {
class SearchResultsWidget;
}

class SearchResultsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SearchResultsWidget(QWidget* parent = nullptr);
    ~SearchResultsWidget();

public slots:
    void showResults(const QStringList& results);

private slots:
    void on_bHideResults_clicked();

private:
    Ui::SearchResultsWidget* ui;
};

