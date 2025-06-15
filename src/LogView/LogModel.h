#pragma once

#include "../LogManagement/LogManager.h"

#include <QAbstractTableModel>
#include <QRegularExpression>

#include <memory>
#include <deque>


class LogModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit LogModel(std::unique_ptr<LogManager>&& logManager, QObject *parent = nullptr);

    void setModules(const std::unordered_set<QString>& modules);

    bool canFetchDownMore() const;
    void fetchDownMore();

    const std::vector<Format::Field>& getFields() const;

    // Header:
    QVariant headerData(int section,
                        Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    // Basic functionality:
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;

    // Fetch data dynamically:
    bool hasChildren(const QModelIndex& parent = QModelIndex()) const override;

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

private:
    void update();

private:
    std::unique_ptr<LogManager> manager;
    std::vector<Format::Field> fields;
    std::unordered_set<QString> modules;
    std::deque<LogEntry> logs;

    static const int BatchSize = 1000;
};
