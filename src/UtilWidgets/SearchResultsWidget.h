#pragma once

#include "EntryLinkView.h"

#include <QDockWidget>
#include <QStringList>


class QListView;
class EntryLinkModel;

class SearchResultsWidget : public QDockWidget
{
    Q_OBJECT

public:
    explicit SearchResultsWidget(QWidget* parent = nullptr);
    ~SearchResultsWidget() override = default;

    void clearResults();

signals:
    void selectedResult(const std::chrono::system_clock::time_point& result);
    void handleError(const QString& msg);

public slots:
    void showResults(const QMap<std::chrono::system_clock::time_point, QString>& results);

private:
    void hideEvent(QHideEvent* event) override;

    void resetModel(EntryLinkModel* newModel);

private:
    EntryLinkView* mResults = nullptr;
};
