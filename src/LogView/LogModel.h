#pragma once

#include "LogService.h"

#include <QAbstractTableModel>
#include <QRegularExpression>

#include <memory>
#include <deque>
#include <set>

class LogFilter;


class LogModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    enum class PredefinedColumn
    {
        Module = 1
    };

    enum class MetaData
    {
        Line = Qt::UserRole,
        Message,
        Time
    };

public:
    explicit LogModel(LogService* logService, QObject *parent = nullptr);

    void goToTime(const QDateTime& time);
    void goToTime(const std::chrono::system_clock::time_point& time);

    bool canFetchUpMore() const;
    virtual void fetchUpMore();

    bool canFetchDownMore() const;
    virtual void fetchDownMore();

    bool isFulled() const;

    const std::vector<Format::Field>& getFields() const;
    const std::unordered_set<QVariant, VariantHash> availableValues(int section) const;

    QDateTime getStartTime() const;
    QDateTime getEndTime() const;

    QDateTime getFirstEntryTime() const;
    QDateTime getLastEntryTime() const;

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

    LogService* getService() const;

signals:
    void startPageSwap();
    void endPageSwap(int rows);

    void requestedTimeAvailable(const QModelIndex& index);

    void handleError(const QString& message);

public slots:
    void handleIterator(int, bool);
    void handleData(int);

protected:
    void fetchUpMore(const LogFilter& filter);
    void fetchDownMore(const LogFilter& filter);

private:
    void fetchUpMoreImpl(const std::shared_ptr<LogEntryIterator<false>>& it);
    void fetchDownMoreImpl(const std::shared_ptr<LogEntryIterator<true>>& it);

private:
    struct LogItem
    {
        LogEntry entry;
        size_t index;
    };

    enum class DataRequestType
    {
        None,
        Append,
        Prepend,
        ReplaceForward,
        ReplaceBackward
    };

    struct MergeHeapCacheComparator
    {
        bool operator()(const MergeHeapCache& lhs, const MergeHeapCache& rhs) const
        {
            return lhs.time < rhs.time;
        }
    };

    typedef std::set<MergeHeapCache, MergeHeapCacheComparator> MergeHeapCacheContainer;

private:
    void skipDataRequests();

    const Format::Field& getField(int section) const;

    size_t getParentIndex(const QModelIndex& index) const;

    DataRequestType handleDataRequest(int index);

    enum class Connection
    {
        None,
        Up,
        Down
    };
    Connection detectConnection(const MergeHeapCache& entry) const;
    void reinitIteratorsWithClosestTime(const MergeHeapCache& newCache, Connection connection);

    template<bool straight>
    std::shared_ptr<LogEntryIterator<straight>> createIterator(const MergeHeapCache& cache, const std::chrono::system_clock::time_point& startTime, const std::chrono::system_clock::time_point& endTime)
    {
        return std::make_shared<LogEntryIterator<straight>>(service->getSession()->createIterator<straight>(cache, startTime, endTime));
    }

private:
    static int loadBlockSize();
    static int loadBlockCount();

private:
    LogService* service;

    QDateTime startTime;
    QDateTime endTime;

    int iteratorIndex;
    std::shared_ptr<LogEntryIterator<true>> iterator;
    std::shared_ptr<LogEntryIterator<false>> reverseIterator;

    std::unordered_map<int, DataRequestType> dataRequests;

    std::vector<Format::Field> fields;
    std::unordered_set<QString> modules;
    std::deque<LogItem> logs;

    MergeHeapCacheContainer entryCache;

    QDateTime requestedTime;

    int blockSize = 2000;
    int blockCount = 4;
};
