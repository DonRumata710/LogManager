#pragma once

#include "LogManagement/LogManager.h"
#include "ThreadSafePtr.h"
#include "LogFilter.h"

#include <QObject>
#include <QFile>
#include <QTreeView>


class LogService : public QObject
{
    Q_OBJECT

public:
    explicit LogService(QObject *parent = nullptr);

    const ThreadSafePtr<LogManager>& getLogManager() const;

    int requestIterator(const std::chrono::system_clock::time_point& startTime,
                        const std::chrono::system_clock::time_point& endTime);
    std::shared_ptr<LogEntryIterator> getIterator(int index);

    int requestLogEntries(const std::shared_ptr<LogEntryIterator>& iterator, int entryCount);
    std::vector<LogEntry> getResult(int index);

signals:
    void logManagerCreated();
    void iteratorCreated(int);
    void dataLoaded(int);

public slots:
    void openFile(const QString& file, const QStringList& formats);
    void openFolder(const QString& logDirectory, const QStringList& formats);

    void exportData(const QString& filename, const QDateTime& startTime, const QDateTime& endTime);
    void exportData(const QString& filename, const QDateTime& startTime, const QDateTime& endTime, const QStringList& fields);
    void exportData(const QString& filename, const QDateTime& startTime, const QDateTime& endTime, const QStringList& fields, const LogFilter& filter);

    void exportData(const QString& filename, QTreeView* view);

private slots:
    void handleIteratorRequest();
    void handleDataRequest();

private:
    struct IteratorRequest
    {
        int index;
        std::chrono::system_clock::time_point startTime;
        std::chrono::system_clock::time_point endTime;

        IteratorRequest(int index, const std::chrono::system_clock::time_point& start, const std::chrono::system_clock::time_point& end);
    };

    struct DataRequest
    {
        int index;
        std::shared_ptr<LogEntryIterator> iterator;
        int entryCount;
        bool active = true;

        DataRequest(int index, std::shared_ptr<LogEntryIterator> it, int count);
    };

private:
    std::vector<std::shared_ptr<Format>> getFormats(const QStringList& formats);

    void exportDataToFile(const QString& filename, const QDateTime& startTime, const QDateTime& endTime, const std::function<void (QFile& file, const LogEntry&)>& writeFunction);

private:
    ThreadSafePtr<LogManager> logManager;

    int nextRequestIndex = 0;
    ThreadSafePtr<std::deque<IteratorRequest>> iteratorRequests;
    ThreadSafePtr<std::map<int, std::shared_ptr<LogEntryIterator>>> iterators;

    ThreadSafePtr<std::deque<DataRequest>> dataRequests;
    ThreadSafePtr<std::map<int, std::vector<LogEntry>>> dataRequestResults;
};
