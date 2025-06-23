#pragma once

#include "LogManagement/FormatManager.h"
#include "LogService.h"

#include <QApplication>
#include <QThread>


class Application : public QApplication
{
    Q_OBJECT

public:
    Application(int& argc, char** argv);
    ~Application();

    FormatManager& getFormatManager();
    LogService* getLogService();

private:
    FormatManager formatManager;

    QThread* serviceThread;
    LogService* logService;
};
