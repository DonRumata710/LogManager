#include "Utils.h"

#include <QDateTime>


QDateTime DateTimeFromChronoSystemClock(const std::chrono::system_clock::time_point& tp)
{
    auto minMs = std::chrono::duration_cast<std::chrono::milliseconds>(tp.time_since_epoch()).count();
    return QDateTime::fromMSecsSinceEpoch(minMs, Qt::LocalTime);
}

std::chrono::system_clock::time_point ChronoSystemClockFromDateTime(const QDateTime& dt)
{
    return std::chrono::system_clock::time_point{ std::chrono::milliseconds{ dt.toMSecsSinceEpoch() } };
}

Tracer::Tracer(const char* functionName) : functionName(functionName)
{
    qDebug() << "start" << functionName;
}

Tracer::~Tracer()
{
    qDebug() << "finish" << functionName;
}
