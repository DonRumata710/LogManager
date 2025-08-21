#pragma once

#include <QObject>
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
    void showTimeline();

private:
    SessionService* sessionService;
};

