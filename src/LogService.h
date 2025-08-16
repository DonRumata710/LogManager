#pragma once

#include "services/SessionService.h"
#include "services/SearchService.h"
#include "services/ExportService.h"

class LogService : public QObject
{
    Q_OBJECT
public:
    using DataRequest = SessionService::DataRequest;

    explicit LogService(QObject *parent = nullptr);

    SessionService* getSessionService();
    SearchService* getSearchService();
    ExportService* getExportService();

    const ThreadSafePtr<LogManager>& getLogManager() const { return sessionService.getLogManager(); }
    const ThreadSafePtr<Session>& getSession() const { return sessionService.getSession(); }

    void createSession(const std::unordered_set<QString>& modules,
                       const std::chrono::system_clock::time_point& minTime,
                       const std::chrono::system_clock::time_point& maxTime) { sessionService.createSession(modules, minTime, maxTime); }

    int requestIterator(const std::chrono::system_clock::time_point& startTime,
                        const std::chrono::system_clock::time_point& endTime) { return sessionService.requestIterator(startTime, endTime); }
    std::shared_ptr<LogEntryIterator<>> getIterator(int index) { return sessionService.getIterator(index); }

    int requestReverseIterator(const std::chrono::system_clock::time_point& startTime,
                               const std::chrono::system_clock::time_point& endTime) { return sessionService.requestReverseIterator(startTime, endTime); }
    std::shared_ptr<LogEntryIterator<false>> getReverseIterator(int index) { return sessionService.getReverseIterator(index); }

    template<typename Iterator>
    int requestLogEntries(const std::shared_ptr<Iterator>& iterator, int entryCount) { return sessionService.requestLogEntries(iterator, entryCount); }

    template<typename Iterator>
    int requestLogEntries(const std::shared_ptr<Iterator>& iterator, int entryCount, const LogFilter& filter) { return sessionService.requestLogEntries(iterator, entryCount, filter); }

    template<typename Iterator>
    int requestLogEntries(const std::shared_ptr<Iterator>& iterator, int entryCount, const std::chrono::system_clock::time_point& until) { return sessionService.requestLogEntries(iterator, entryCount, until); }

    template<typename Iterator>
    int requestLogEntries(const std::shared_ptr<Iterator>& iterator, int entryCount, const std::chrono::system_clock::time_point& until, const LogFilter& filter) { return sessionService.requestLogEntries(iterator, entryCount, until, filter); }

    std::vector<LogEntry> getResult(int index) { return sessionService.getResult(index); }

public slots:
    void openFile(const QString& file, const QStringList& formats) { sessionService.openFile(file, formats); }
    void openFolder(const QString& logDirectory, const QStringList& formats) { sessionService.openFolder(logDirectory, formats); }

    void search(const QDateTime& time, const QString& searchTerm, bool lastColumn, bool regexEnabled, bool backward) { searchService.search(time, searchTerm, lastColumn, regexEnabled, backward); }
    void searchWithFilter(const QDateTime& time, const QString& searchTerm, bool lastColumn, bool regexEnabled, bool backward, const LogFilter& filter) { searchService.searchWithFilter(time, searchTerm, lastColumn, regexEnabled, backward, filter); }

    void exportData(const QString& filename, const QDateTime& startTime, const QDateTime& endTime) { exportService.exportData(filename, startTime, endTime); }
    void exportData(const QString& filename, const QDateTime& startTime, const QDateTime& endTime, const QStringList& fields) { exportService.exportData(filename, startTime, endTime, fields); }
    void exportData(const QString& filename, const QDateTime& startTime, const QDateTime& endTime, const QStringList& fields, const LogFilter& filter) { exportService.exportData(filename, startTime, endTime, fields, filter); }
    void exportData(const QString& filename, QTreeView* view) { exportService.exportData(filename, view); }

signals:
    void logManagerCreated(const QString& source);
    void iteratorCreated(int, bool isStraight);
    void dataLoaded(int);
    void progressUpdated(const QString& message, int percent);
    void searchFinished(const QString& searchTerm, const QDateTime& entryTime);
    void handleError(const QString& message);

private:
    SessionService sessionService;
    SearchService searchService;
    ExportService exportService;
};
