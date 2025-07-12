#pragma once

#include <QDebug>

#include <chrono>


#define QT_SLOT_BEGIN \
    qDebug() << "start" << __FUNCTION__; \
    try {

#define QT_SLOT_END \
        qDebug() << "finish" << __FUNCTION__; \
    } catch (const std::exception& e) { \
        qCritical() << "Exception in slot:" << e.what(); \
    } catch (...) { \
        qCritical() << "Unknown exception in slot"; \
    }


class QDateTime;

QDateTime DateTimeFromChronoSystemClock(const std::chrono::system_clock::time_point& tp);
