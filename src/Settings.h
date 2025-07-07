#pragma once

#include <QSettings>

class Settings : public QSettings
{
public:
    explicit Settings(QObject *parent = nullptr);
};
