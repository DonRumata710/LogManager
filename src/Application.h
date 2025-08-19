#pragma once

#include "LogManagement/FormatManager.h"
#include "services/SessionService.h"
#include "services/SearchService.h"
#include "services/ExportService.h"

#include <QApplication>
#include <QThread>


class Application : public QApplication
{
    Q_OBJECT

public:
    Application(int& argc, char** argv);
    ~Application();

    FormatManager& getFormatManager();
    SessionService* getSessionService();
    SearchService* getSearchService();
    ExportService* getExportService();

private:
    FormatManager formatManager;

    QThread* serviceThread;
    SessionService* sessionService;
    SearchService* searchService;
    ExportService* exportService;
};
