#include "Application.h"

#include <QSettings>
#include <QDir>


Application::Application(int& argc, char** argv) :
    QApplication(argc, argv),
    serviceThread(std::make_unique<QThread>(this))
{
    serviceThread->start();

    sessionService = new SessionService();
    searchService = new SearchService(sessionService);
    exportService = new ExportService(sessionService);
    timelineService = new TimelineService(sessionService);

    sessionService->moveToThread(serviceThread.get());
    searchService->moveToThread(serviceThread.get());
    exportService->moveToThread(serviceThread.get());
    timelineService->moveToThread(serviceThread.get());

    setApplicationName(APPLICATION_NAME);
    setApplicationVersion(APPLICATION_VERSION);
    setOrganizationName("");

    QSettings::setDefaultFormat(QSettings::Format::IniFormat);
    QSettings::setPath(QSettings::Format::IniFormat, QSettings::Scope::UserScope, QDir::currentPath() + "/" + applicationName() + ".ini");
}

Application::~Application()
{
    if (serviceThread)
    {
        serviceThread->quit();
        serviceThread->wait();
        serviceThread.reset();
    }
}

FormatManager& Application::getFormatManager()
{
    return formatManager;
}

SessionService* Application::getSessionService()
{
    return sessionService;
}

SearchService* Application::getSearchService()
{
    return searchService;
}

ExportService* Application::getExportService()
{
    return exportService;
}

TimelineService* Application::getTimelineService()
{
    return timelineService;
}
