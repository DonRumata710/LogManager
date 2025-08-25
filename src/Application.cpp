#include "Application.h"
#include "Utils.h"

#include <QSettings>
#include <QDir>


Application::Application(int& argc, char** argv) :
    QApplication(argc, argv),
    serviceThread(std::make_unique<QThread>(this))
{
    serviceThread->start();

    qRegisterMetaType<std::chrono::system_clock::time_point>("std::chrono::system_clock::time_point");

    sessionService = std::make_unique<SessionService>();
    searchService = std::make_unique<SearchService>(sessionService.get());
    exportService = std::make_unique<ExportService>(sessionService.get());
    timelineService = std::make_unique<TimelineService>(sessionService.get());

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
    return sessionService.get();
}

SearchService* Application::getSearchService()
{
    return searchService.get();
}

ExportService* Application::getExportService()
{
    return exportService.get();
}

TimelineService* Application::getTimelineService()
{
    return timelineService.get();
}
