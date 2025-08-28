#pragma once

#include <QDockWidget>
#include <QStringList>

class QListWidget;

class SearchResultsWidget : public QDockWidget
{
    Q_OBJECT

public:
    explicit SearchResultsWidget(QWidget* parent = nullptr);
    ~SearchResultsWidget() override = default;

signals:
    void handleError(const QString& msg);

public slots:
    void showResults(const QStringList& results);

private:
    QListWidget* mResults = nullptr;
};
