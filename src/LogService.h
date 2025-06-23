#pragma once

#include "LogManagement/LogManager.h"
#include "ThreadSafePtr.h"

#include <QObject>


class LogService : public QObject
{
    Q_OBJECT

public:
    explicit LogService(QObject *parent = nullptr);

    const std::shared_ptr<LogManager>& getLogManager() const;

    int requestIterator(const std::chrono::system_clock::time_point& startTime);
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

private slots:
    void handleIteratorRequest();
    void handleDataRequest();

private:
    struct IteratorRequest
    {
        int index;
        std::chrono::system_clock::time_point startTime;

        IteratorRequest(int index, const std::chrono::system_clock::time_point& time);
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

private:
    std::shared_ptr<LogManager> logManager;

    int nextRequestIndex = 0;
    ThreadSafePtr<std::deque<IteratorRequest>> iteratorRequests;
    ThreadSafePtr<std::map<int, std::shared_ptr<LogEntryIterator>>> iterators;

    ThreadSafePtr<std::deque<DataRequest>> dataRequests;
    ThreadSafePtr<std::map<int, std::vector<LogEntry>>> dataRequestResults;
};
