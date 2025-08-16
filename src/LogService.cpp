#include "LogService.h"

LogService::LogService(QObject *parent)
    : QObject(parent),
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
