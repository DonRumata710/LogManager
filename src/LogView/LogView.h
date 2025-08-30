#pragma once

#include <QAbstractItemModel>
#include <QTreeView>
#include <chrono>

class QContextMenuEvent;

class LogModel;
class LogFilterModel;


class LogView : public QTreeView
{
    Q_OBJECT

public:
    LogView(QWidget* parent = nullptr);

    void setLogModel(QAbstractItemModel* model);

public slots:
    void bookmarkActivated(const std::chrono::system_clock::time_point& time);
    void requestedItemHandle(const QModelIndex& index);

signals:
    void handleError(const QString& message);

private slots:
    void checkFetchNeeded();
    void handleFirstDataLoaded();
    void handleReset();

    void handleFirstLineChangeStart();
    void handleFirstLineRemoving(const QModelIndex& parent, int first, int last);
    void handleFirstLineAddition(const QModelIndex& parent, int first, int last);

private:
    virtual void scrollContentsBy(int dx, int dy) override;
    void contextMenuEvent(QContextMenuEvent* event) override;

private:
    std::optional<QModelIndex> lastScrollPosition;
    LogModel* currentLogModel = nullptr;
    LogFilterModel* currentProxyModel = nullptr;
};
