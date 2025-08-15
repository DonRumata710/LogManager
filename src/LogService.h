#pragma once

#include "LogManagement/LogManager.h"
#include "LogManagement/Session.h"
#include "LogManagement/FilteredLogIterator.h"
#include "ThreadSafePtr.h"
#include "LogFilter.h"

#include <QObject>
#include <QFile>
#include <QTreeView>
#include <QString>

#include <map>
#include <vector>
#include <memory>
#include <variant>
#include <optional>


class LogService : public QObject
{
    Q_OBJECT

public:
    struct DataRequest
    {
        int index;
        std::variant<
            std::shared_ptr<LogEntryIterator<true>>,
            std::shared_ptr<LogEntryIterator<false>>,
            std::shared_ptr<FilteredLogIterator<true>>,
            std::shared_ptr<FilteredLogIterator<false>>
            > iterator;
        int entriesCount;
        std::optional<std::chrono::system_clock::time_point> until;

        template<typename Iterator>
        DataRequest(int idx, const std::shared_ptr<Iterator>& it, int count, const std::optional<std::chrono::system_clock::time_point>& until = std::nullopt)
            : index(idx), iterator(it), entriesCount(count), until(until)
        {}
    };

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

    template<typename Iterator>
    int requestLogEntries(const std::shared_ptr<Iterator>& iterator, int entryCount)
    {
        if (!logManager || !iterator || entryCount <= 0 || !iterator->hasLogs())
            throw std::runtime_error("Invalid log entry request parameters.");

        int index = nextRequestIndex++;
        emit logEntriesRequested(DataRequest(index, iterator, entryCount));
        return index;
    }

    template<typename Iterator>
    int requestLogEntries(const std::shared_ptr<Iterator>& iterator, int entryCount, const LogFilter& filter)
    {
        if (!logManager || !iterator || entryCount <= 0 || !iterator->hasLogs())
            throw std::runtime_error("Invalid log entry request parameters.");

        int index = nextRequestIndex++;
        emit logEntriesRequested(DataRequest(index, std::make_shared<FilteredLogIterator<Iterator::IsStraight>>(iterator, filter), entryCount));
        return index;
    }

    template<typename Iterator>
    int requestLogEntries(const std::shared_ptr<Iterator>& iterator, int entryCount, const std::chrono::system_clock::time_point& until)
    {
        if (!logManager || !iterator || entryCount <= 0 || !iterator->hasLogs())
            throw std::runtime_error("Invalid log entry request parameters.");

        int index = nextRequestIndex++;
        emit logEntriesRequested(DataRequest(index, iterator, entryCount, until));
        return index;
    }

    template<typename Iterator>
    int requestLogEntries(const std::shared_ptr<Iterator>& iterator, int entryCount, const std::chrono::system_clock::time_point& until, const LogFilter& filter)
    {
        if (!logManager || !iterator || entryCount <= 0 || !iterator->hasLogs())
            throw std::runtime_error("Invalid log entry request parameters.");

        int index = nextRequestIndex++;
        emit logEntriesRequested(DataRequest(index, std::make_shared<FilteredLogIterator<Iterator::IsStraight>>(iterator, filter), entryCount, until));
        return index;
    }

    std::vector<LogEntry> getResult(int index);

signals:
    void logManagerCreated(const QString& source);
    void iteratorCreated(int, bool isStraight);
    void dataLoaded(int);

    void progressUpdated(const QString& message, int percent);

    void searchFinished(const QString& searchTerm, const QDateTime& entryTime);

    void handleError(const QString& message);

    void iteratorRequested(int index, const std::chrono::system_clock::time_point& startTime, const std::chrono::system_clock::time_point& endTime);
    void reverseIteratorRequested(int index, const std::chrono::system_clock::time_point& startTime, const std::chrono::system_clock::time_point& endTime);
    void logEntriesRequested(const DataRequest& request);

public slots:
    void openFile(const QString& file, const QStringList& formats);
    void openFolder(const QString& logDirectory, const QStringList& formats);

    void search(const QDateTime& time, const QString& searchTerm, bool lastColumn, bool regexEnabled, bool backward);
    void searchWithFilter(const QDateTime& time, const QString& searchTerm, bool lastColumn, bool regexEnabled, bool backward, const LogFilter& filter);

    void exportData(const QString& filename, const QDateTime& startTime, const QDateTime& endTime);
    void exportData(const QString& filename, const QDateTime& startTime, const QDateTime& endTime, const QStringList& fields);
    void exportData(const QString& filename, const QDateTime& startTime, const QDateTime& endTime, const QStringList& fields, const LogFilter& filter);

    void exportData(const QString& filename, QTreeView* view);

private slots:
    void handleIteratorRequest(int index, const std::chrono::system_clock::time_point& startTime, const std::chrono::system_clock::time_point& endTime);
    void handleReverseIteratorRequest(int index, const std::chrono::system_clock::time_point& startTime, const std::chrono::system_clock::time_point& endTime);
    void handleDataRequest(const DataRequest& request);

private:
    std::vector<std::shared_ptr<Format>> getFormats(const QStringList& formats);

    void exportDataToFile(const QString& filename, const QDateTime& startTime, const QDateTime& endTime, const std::function<void (QFile& file, const LogEntry&)>& writeFunction, const std::function<void (QFile& file)>& prefix = std::function<void (QFile& file)>{});
    void exportDataToFile(const QString& filename, const QDateTime& startTime, const QDateTime& endTime, const std::function<void (QFile& file, const LogEntry&)>& writeFunction, const QStringList& fields);

private:
    ThreadSafePtr<LogManager> logManager;
    ThreadSafePtr<Session> session;

    int nextRequestIndex = 0;
    ThreadSafePtr<std::map<int, std::shared_ptr<LogEntryIterator<true>>>> iterators;
    ThreadSafePtr<std::map<int, std::shared_ptr<LogEntryIterator<false>>>> reverseIterators;

    ThreadSafePtr<std::map<int, std::vector<LogEntry>>> dataRequestResults;
};

Q_DECLARE_METATYPE(LogService::DataRequest)

