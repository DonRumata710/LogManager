#pragma once

#include "LogManagement/LogManager.h"
#include "LogManagement/Session.h"
#include "ThreadSafePtr.h"
#include "LogFilter.h"

#include <QObject>
#include <QFile>
#include <QTreeView>

#include <map>
#include <deque>
#include <vector>
#include <memory>


class LogService : public QObject
{
    Q_OBJECT

public:
    explicit LogService(QObject *parent = nullptr);

    const ThreadSafePtr<LogManager>& getLogManager() const;

    void createSession(const std::unordered_set<QString>& modules,
                        const std::chrono::system_clock::time_point& minTime,
                        const std::chrono::system_clock::time_point& maxTime);
    const ThreadSafePtr<Session>& getSession() const;

    int requestIterator(const std::chrono::system_clock::time_point& startTime,
                        const std::chrono::system_clock::time_point& endTime);
    std::shared_ptr<LogEntryIterator<>> getIterator(int index);

    int requestReverseIterator(const std::chrono::system_clock::time_point& startTime,
                               const std::chrono::system_clock::time_point& endTime);
    std::shared_ptr<LogEntryIterator<false>> getReverseIterator(int index);

    int requestLogEntries(const std::shared_ptr<LogEntryIterator<true>>& iterator, int entryCount);
    int requestLogEntries(const std::shared_ptr<LogEntryIterator<false>>& iterator, int entryCount);
    std::vector<LogEntry> getResult(int index);

signals:
    void logManagerCreated();
    void iteratorCreated(int, bool isStraight);
    void dataLoaded(int);

public slots:
    void openFile(const QString& file, const QStringList& formats);
    void openFolder(const QString& logDirectory, const QStringList& formats);

    void search(const QString& searchTerm, bool lastColumn, bool regexEnabled, bool backward);

    void exportData(const QString& filename, const QDateTime& startTime, const QDateTime& endTime);
    void exportData(const QString& filename, const QDateTime& startTime, const QDateTime& endTime, const QStringList& fields);
    void exportData(const QString& filename, const QDateTime& startTime, const QDateTime& endTime, const QStringList& fields, const LogFilter& filter);

    void exportData(const QString& filename, QTreeView* view);

private slots:
    void handleIteratorRequest();
    void handleDataRequest();

    void handleReverseIteratorRequest();
    void handleDataRequestReverse();

private:
    struct IteratorRequest
    {
        int index;
        std::chrono::system_clock::time_point startTime;
        std::chrono::system_clock::time_point endTime;

        IteratorRequest(int index, const std::chrono::system_clock::time_point& start, const std::chrono::system_clock::time_point& end);
    };

    struct ReverseIteratorRequest
    {
        int index;
        std::chrono::system_clock::time_point startTime;
        std::chrono::system_clock::time_point endTime;

        ReverseIteratorRequest(int index, const std::chrono::system_clock::time_point& start, const std::chrono::system_clock::time_point& end);
    };

    struct DataRequest
    {
        int index;
        std::shared_ptr<LogEntryIterator<>> iterator;
        int entryCount;
        bool active = true;

        DataRequest(int index, const std::shared_ptr<LogEntryIterator<>>& it, int count);
    };

    struct DataRequestReverse
    {
        int index;
        std::shared_ptr<LogEntryIterator<false>> iterator;
        int entryCount;
        bool active = true;

        DataRequestReverse(int index, const std::shared_ptr<LogEntryIterator<false>>& it, int count);
    };

private:
    std::vector<std::shared_ptr<Format>> getFormats(const QStringList& formats);

    void exportDataToFile(const QString& filename, const QDateTime& startTime, const QDateTime& endTime, const std::function<void (QFile& file, const LogEntry&)>& writeFunction);

private:
    ThreadSafePtr<LogManager> logManager;
    ThreadSafePtr<Session> session;

    int nextRequestIndex = 0;
    ThreadSafePtr<std::deque<IteratorRequest>> iteratorRequests;
    ThreadSafePtr<std::map<int, std::shared_ptr<LogEntryIterator<true>>>> iterators;

    ThreadSafePtr<std::deque<ReverseIteratorRequest>> reverseIteratorRequests;
    ThreadSafePtr<std::map<int, std::shared_ptr<LogEntryIterator<false>>>> reverseIterators;

    ThreadSafePtr<std::deque<DataRequest>> dataRequests;
    ThreadSafePtr<std::deque<DataRequestReverse>> dataRequestsReverse;
    ThreadSafePtr<std::map<int, std::vector<LogEntry>>> dataRequestResults;
};
