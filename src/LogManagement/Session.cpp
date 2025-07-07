#include "Session.h"

#include "LogUtils.h"

#include <quazip/quazipfile.h>

#include <QFile>
#include <QDebug>

#include <filesystem>


Session::Session(const std::shared_ptr<LogStorage>& storage) : logStorage(storage)
{}

const std::unordered_set<std::shared_ptr<Format>>& Session::getFormats() const
{
    return logStorage->getFormats();
}

const std::unordered_set<QString>& Session::getModules() const
{
    return logStorage->getModules();
}

const std::unordered_set<QVariant, VariantHash>& Session::getEnumList(const QString& field) const
{
    return logStorage->getEnumList(field);
}

std::chrono::system_clock::time_point Session::getMinTime() const
{
    return logStorage->getMinTime();
}

std::chrono::system_clock::time_point Session::getMaxTime() const
{
    return logStorage->getMaxTime();
}
