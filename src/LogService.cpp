#include "LogService.h"


LogService::LogService(QObject *parent) :
    QObject(parent),
    sessionService(this),
    searchService(&sessionService, this),
    exportService(&sessionService, this)
{
    connect(&sessionService, &SessionService::logManagerCreated, this, &LogService::logManagerCreated);
    connect(&sessionService, &SessionService::iteratorCreated, this, &LogService::iteratorCreated);
    connect(&sessionService, &SessionService::dataLoaded, this, &LogService::dataLoaded);
    connect(&sessionService, &SessionService::progressUpdated, this, &LogService::progressUpdated);
    connect(&sessionService, &SessionService::handleError, this, &LogService::handleError);

    connect(&searchService, &SearchService::progressUpdated, this, &LogService::progressUpdated);
    connect(&searchService, &SearchService::searchFinished, this, &LogService::searchFinished);
    connect(&searchService, &SearchService::handleError, this, &LogService::handleError);

    connect(&exportService, &ExportService::progressUpdated, this, &LogService::progressUpdated);
    connect(&exportService, &ExportService::handleError, this, &LogService::handleError);
}

SessionService* LogService::getSessionService()
{
    return &sessionService;
}

SearchService* LogService::getSearchService()
{
    return &searchService;
}

ExportService* LogService::getExportService()
{
    return &exportService;
}

void LogService::createSession(const std::unordered_set<QString>& modules, const std::chrono::system_clock::time_point& minTime, const std::chrono::system_clock::time_point& maxTime)
{
    sessionService.createSession(modules, minTime, maxTime);
}

int LogService::requestIterator(const std::chrono::system_clock::time_point& startTime, const std::chrono::system_clock::time_point& endTime)
{
    return sessionService.requestIterator(startTime, endTime);
}

std::shared_ptr<LogEntryIterator<> > LogService::getIterator(int index)
{
    return sessionService.getIterator(index);
}

int LogService::requestReverseIterator(const std::chrono::system_clock::time_point& startTime, const std::chrono::system_clock::time_point& endTime)
{
    return sessionService.requestReverseIterator(startTime, endTime);
}

std::shared_ptr<LogEntryIterator<false> > LogService::getReverseIterator(int index)
{
    return sessionService.getReverseIterator(index);
}
