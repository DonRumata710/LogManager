#pragma once

#include "Format.h"
#include "Log.h"
#include "LogStorage.h"
#include "LogEntryIterator.h"

#include <QDateTime>

#include <unordered_set>


class LogManager
{
public:
    LogManager(const std::vector<QString>& folders, const std::vector<std::shared_ptr<Format>>& formats);
    LogManager(const QString& filename, const std::vector<std::shared_ptr<Format>>& formats);

    const std::unordered_set<std::shared_ptr<Format>>& getFormats() const;
    const std::unordered_set<QString>& getModules() const;

    const std::unordered_set<QVariant, VariantHash>& getEnumList(const QString& field) const;

    std::chrono::system_clock::time_point getMinTime() const;
    std::chrono::system_clock::time_point getMaxTime() const;

    LogEntryIterator getIterator(const std::chrono::system_clock::time_point& startTime = std::chrono::system_clock::time_point(), const std::chrono::system_clock::time_point& endTime = std::chrono::system_clock::time_point::max());

private:
    bool addFile(const QString& filename, const QString& stem, const QString& extension, const std::vector<std::shared_ptr<Format>>& formats);
    std::optional<std::pair<std::shared_ptr<Format>, std::chrono::system_clock::time_point>> scanLogFile(const QString& filename, const std::vector<std::shared_ptr<Format>>& formats);

    Log createLog(const QString& filename, std::shared_ptr<Format> format);

private:
    std::shared_ptr<LogStorage> logStorage;
};
