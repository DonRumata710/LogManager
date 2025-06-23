#include "Application.h"


Application::Application(int& argc, char** argv) :
    QApplication(argc, argv)
{
    serviceThread = new QThread(this);
    serviceThread->start();

    logService = new LogService();
    logService->moveToThread(serviceThread);
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
