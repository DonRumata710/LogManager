#pragma once

#include "LogManagement/Session.h"
#include "LogFilter.h"
#include "ThreadSafePtr.h"

#include <QObject>
#include <QDateTime>

class SessionService;

class SearchService : public QObject
{
    Q_OBJECT
public:
    explicit SearchService(SessionService* sessionService, QObject* parent = nullptr);

public slots:
    void search(const QDateTime& time, const QString& searchTerm, bool lastColumn, bool regexEnabled, bool backward);
    void searchWithFilter(const QDateTime& time, const QString& searchTerm, bool lastColumn, bool regexEnabled, bool backward, const LogFilter& filter);

signals:
    void progressUpdated(const QString& message, int percent);
    void searchFinished(const QString& searchTerm, const QDateTime& entryTime);
    void handleError(const QString& message);

private:
    SessionService* sessionService;
};
