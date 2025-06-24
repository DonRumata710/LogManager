#pragma once

#include "LogService.h"

#include <QAbstractTableModel>
#include <QRegularExpression>

#include <memory>
#include <deque>


class LogModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    enum class PredefinedColumn
    {
        Module = 1
    };

public:
    explicit LogModel(LogService* logManager, const QDateTime& startTime, const QDateTime& endTime, QObject *parent = nullptr);

    void setModules(const std::unordered_set<QString>& modules);

    bool canFetchDownMore() const;
    void fetchDownMore();

    const std::vector<Format::Field>& getFields() const;
    const std::unordered_set<QVariant, VariantHash> availableValues(int section) const;

    QDateTime getStartTime() const;
    QDateTime getEndTime() const;

    QStringList getFieldsName();

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

public slots:
    void handleIterator(int);
    void handleData(int);

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

    enum class DataRequestType
    {
        Append,
        Prepend,
        Replace
    };

private:
    LogService* service;

    int iteratorIndex;
    std::shared_ptr<LogEntryIterator> iterator;

    std::unordered_map<int, DataRequestType> dataRequests;

    std::vector<Format::Field> fields;
    std::unordered_set<QString> modules;
    std::deque<LogItem> logs;

    static const int BatchSize = 1000;
};
