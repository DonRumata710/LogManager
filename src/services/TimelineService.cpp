#include "TimelineService.h"
#include "SessionService.h"

#include <QMetaType>
#include <chrono>
#include <utility>

TimelineService::TimelineService(SessionService* sessionService, QObject* parent)
    : QObject(parent), sessionService(sessionService)
{
    qRegisterMetaType<Statistics::Bucket>("Statistics::Bucket");
    qRegisterMetaType<std::vector<Statistics::Bucket>>("std::vector<Statistics::Bucket>");
}

void TimelineService::showTimeline(QWidget* parent)
{
    auto sessionPtr = sessionService->getSession();
    if (!sessionPtr)
        return;

    auto start = sessionPtr->getMinTime();
    auto end = sessionPtr->getMaxTime();
    auto data = Statistics::LogHistogram::calculate(*sessionPtr.get(), start, end, std::chrono::minutes(1));

    emit timelineReady(parent, std::move(data));
}

