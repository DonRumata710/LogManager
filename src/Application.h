#pragma once

#include "LogManagement/FormatManager.h"
#include "services/SessionService.h"
#include "services/SearchService.h"
#include "services/ExportService.h"
#include "services/TimelineService.h"

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
    TimelineService* getTimelineService();

private:
    FormatManager formatManager;

    std::unique_ptr<QThread> serviceThread;
    std::unique_ptr<SessionService> sessionService;
    std::unique_ptr<SearchService> searchService;
    std::unique_ptr<ExportService> exportService;
    std::unique_ptr<TimelineService> timelineService;
};
