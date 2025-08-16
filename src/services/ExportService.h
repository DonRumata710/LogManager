#pragma once

#include "LogManagement/Session.h"
#include "LogFilter.h"
#include <QObject>
#include <QFile>
#include <QTreeView>

class SessionService;

class ExportService : public QObject
{
    Q_OBJECT
public:
    explicit ExportService(SessionService* sessionService, QObject* parent = nullptr);

public slots:
    void exportData(const QString& filename, const QDateTime& startTime, const QDateTime& endTime);
    void exportData(const QString& filename, const QDateTime& startTime, const QDateTime& endTime, const QStringList& fields);
    void exportData(const QString& filename, const QDateTime& startTime, const QDateTime& endTime, const QStringList& fields, const LogFilter& filter);
    void exportData(const QString& filename, QTreeView* view);

signals:
    void progressUpdated(const QString& message, int percent);
    void handleError(const QString& message);

private:
    void exportDataToFile(const QString& filename, const QDateTime& startTime, const QDateTime& endTime,
                          const std::function<void (QFile&, const LogEntry&)>& writeFunction,
                          const std::function<void (QFile& file)>& prefix = std::function<void (QFile& file)>{});
    void exportDataToFile(const QString& filename, const QDateTime& startTime, const QDateTime& endTime,
                          const std::function<void (QFile&, const LogEntry&)>& writeFunction, const QStringList& fields);

private:
    SessionService* sessionService;
};
