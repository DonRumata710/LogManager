#include "Application.h"

#include <QSettings>
#include <QDir>


Application::Application(int& argc, char** argv) :
    QApplication(argc, argv)
{
    serviceThread = new QThread(this);
    serviceThread->start();

    logService = new LogService();
    logService->moveToThread(serviceThread);

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

    delete logService;
}

FormatManager& Application::getFormatManager()
{
    return formatManager;
}

LogService* Application::getLogService()
{
    return logService;
}
