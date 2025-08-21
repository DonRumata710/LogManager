#pragma once

#include <QObject>
#include <QDateTime>
#include <vector>
#include "Statistics/LogHistogram.h"

class SessionService;
class QWidget;

class TimelineService : public QObject
{
    Q_OBJECT

public:
    explicit TimelineService(SessionService* sessionService, QObject* parent = nullptr);

signals:
    void timelineReady(std::vector<Statistics::Bucket> data);
    void progressUpdated(const QString& message, int progress);
    void handleError(const QString& message);

public slots:
    void showTimeline(const QDateTime& start, const QDateTime& end);

private:
    SessionService* sessionService;
};

