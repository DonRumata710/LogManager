#include "TimelineService.h"
#include "SessionService.h"
#include "../Utils.h"

#include <QMetaType>

#include <chrono>
#include <utility>

TimelineService::TimelineService(SessionService* sessionService, QObject* parent) : QObject(parent), sessionService(sessionService)
{
    qRegisterMetaType<Statistics::Bucket>("Statistics::Bucket");
    qRegisterMetaType<std::vector<Statistics::Bucket>>("std::vector<Statistics::Bucket>");
}

void TimelineService::showTimeline()
{
    QT_SLOT_BEGIN

    emit progressUpdated(QStringLiteral("Preparing timeline..."), 0);

    auto sessionPtr = sessionService->getSession();
    if (!sessionPtr)
        return;

    auto start = sessionPtr->getMinTime();
    auto end = sessionPtr->getMaxTime();
    auto data = Statistics::LogHistogram::calculate(*sessionPtr.get(), start, end, std::chrono::minutes(1));

    emit progressUpdated(QStringLiteral("Timeline ready"), 100);

    emit timelineReady(std::move(data));

    QT_SLOT_END
}

