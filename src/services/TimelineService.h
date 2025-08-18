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
    void timelineReady(QWidget* parent, std::vector<Statistics::Bucket> data);

public slots:
    void showTimeline(QWidget* parent);

private:
    SessionService* sessionService;
};

