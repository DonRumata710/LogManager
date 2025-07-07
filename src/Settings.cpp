#include "Settings.h"

#include <QApplication>


Settings::Settings(QObject *parent) : QSettings{ qApp->applicationName() + ".ini", QSettings::Format::IniFormat, parent }
{}
