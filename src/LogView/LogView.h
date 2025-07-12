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
    void handleReset();
};
