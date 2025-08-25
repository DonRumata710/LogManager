#pragma once

#include "LogManagement/Session.h"
#include "LogFilter.h"
#include "ThreadSafePtr.h"

#include <QObject>
#include <chrono>

class SessionService;

class SearchService : public QObject
{
    Q_OBJECT
public:
    explicit SearchService(SessionService* sessionService, QObject* parent = nullptr);

public slots:
    void search(const std::chrono::system_clock::time_point& time, const QString& searchTerm, bool lastColumn, bool regexEnabled, bool backward);
    void searchWithFilter(const std::chrono::system_clock::time_point& time, const QString& searchTerm, bool lastColumn, bool regexEnabled, bool backward, const LogFilter& filter);

signals:
    void progressUpdated(const QString& message, int percent);
    void searchFinished(const QString& searchTerm, const std::chrono::system_clock::time_point& entryTime);
    void handleError(const QString& message);

private:
    SessionService* sessionService;
};
