#pragma once

#include <QAbstractItemModel>
#include <QTreeView>


class LogView : public QTreeView
{
    Q_OBJECT

public:
    LogView(QWidget* parent = nullptr);

    void setLogModel(QAbstractItemModel* model);

private slots:
    void checkFetchNeeded();
    void handleFirstDataLoaded();
    void handleReset();

    void handleFirstLineChangeStart();
    void handleFirstLineRemoving(const QModelIndex& parent, int first, int last);
    void handleFirstLineAddition(const QModelIndex& parent, int first, int last);

private:
    std::optional<QModelIndex> lastScrollPosition;
};
