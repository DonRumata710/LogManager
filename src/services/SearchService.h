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
    void search(const std::chrono::system_clock::time_point& time, const QString& searchTerm, bool regexEnabled, bool backward, bool findAll, int column, const QStringList& fields);
    void searchWithFilter(const std::chrono::system_clock::time_point& time, const QString& searchTerm, bool regexEnabled, bool backward, bool findAll, int column, const QStringList& fields, const LogFilter& filter);

signals:
    void progressUpdated(const QString& message, int percent);
    void searchFinished(const QString& searchTerm, const std::chrono::system_clock::time_point& entryTime);
    void searchResults(const QStringList& results);
    void handleError(const QString& message);

private:
    SessionService* sessionService;
};
