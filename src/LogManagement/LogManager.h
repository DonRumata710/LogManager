#pragma once

#include "Format.h"
#include "Log.h"
#include "LogStorage.h"
#include "Session.h"

#include <QDateTime>
#include <QBuffer>

#include <unordered_set>
#include <memory>


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

    Session createSession(const std::unordered_set<QString>& modules, const std::chrono::system_clock::time_point& minTime, const std::chrono::system_clock::time_point& maxTime) const;

private:
    bool scanArchive(const QString& filename, const std::vector<std::shared_ptr<Format>>& formats);

    bool addFile(const QString& filename, const QString& stem, const QString& extension, std::function<std::unique_ptr<QIODevice>(const QString&)> createFileFunc, const std::vector<std::shared_ptr<Format>>& formats);
    std::optional<std::pair<std::shared_ptr<Format>, std::chrono::system_clock::time_point>> scanLogFile(const QString& filename, std::function<std::unique_ptr<QIODevice>(const QString&)> createFileFunc, const std::vector<std::shared_ptr<Format>>& formats);

    Log createLog(const QString& filename, std::function<std::unique_ptr<QIODevice>(const QString&)> createFileFunc, std::shared_ptr<Format> format);

    static void readIntoBuffer(QIODevice& source, QBuffer& targetBuffer);

private:
    std::shared_ptr<LogStorage> logStorage;
};
