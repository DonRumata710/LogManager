#pragma once

#include <QDebug>

#include <chrono>


class Tracer
{
public:
    Tracer(const char* functionName);
    ~Tracer();

private:
    const char* functionName;
};


#define QT_SLOT_BEGIN \
    Tracer{ __FUNCTION__ }; \
    try {

#define QT_SLOT_END \
    } \
    catch (const std::exception& e) { \
        handleError(QString{ "Exception in slot: " } + e.what()); \
    } \
    catch (...) \
    { \
        handleError(QString{ "Unknown exception in slot" }); \
    }


class QDateTime;

QDateTime DateTimeFromChronoSystemClock(const std::chrono::system_clock::time_point& tp);
