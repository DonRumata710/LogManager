#include "Application.h"

#include <QSettings>
#include <QDir>


Application::Application(int& argc, char** argv) :
    QApplication(argc, argv)
{
    serviceThread = new QThread(this);
    serviceThread->start();

    sessionService = new SessionService();
    searchService = new SearchService(sessionService);
    exportService = new ExportService(sessionService);

    sessionService->moveToThread(serviceThread);
    searchService->moveToThread(serviceThread);
    exportService->moveToThread(serviceThread);

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
        delete serviceThread;
    }

    delete exportService;
    delete searchService;
    delete sessionService;
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
