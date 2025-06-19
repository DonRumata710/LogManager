#pragma once

#include "../LogManagement/LogManager.h"

#include <QAbstractTableModel>
#include <QRegularExpression>

#include <memory>
#include <deque>


class LogModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    explicit LogModel(std::unique_ptr<LogManager>&& logManager, QObject *parent = nullptr);

    void setModules(const std::unordered_set<QString>& modules);

    bool canFetchDownMore() const;
    void fetchDownMore();

    const std::vector<Format::Field>& getFields() const;
    const std::unordered_set<QVariant, VariantHash> availableValues(int section) const;

    QVariant headerData(int section,
                        Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex& index) const override;

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;

    bool hasChildren(const QModelIndex& parent = QModelIndex()) const override;

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

    Qt::ItemFlags flags(const QModelIndex& index) const override;

private:
    void update();

    const Format::Field& getField(int section) const;

    size_t getParentIndex(const QModelIndex& index) const;

private:
    struct LogItem
    {
        LogEntry entry;
        size_t index;
    };

private:
    std::unique_ptr<LogManager> manager;
    std::vector<Format::Field> fields;
    std::unordered_set<QString> modules;
    std::deque<LogItem> logs;

    static const int BatchSize = 1000;
};
