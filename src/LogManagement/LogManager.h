#pragma once

#include "Format.h"
#include "Log.h"
#include "LogStorage.h"
#include "Session.h"
#include "DirectoryScanner.h"

#include <QDateTime>
#include <QBuffer>
#include <QByteArray>

#include <unordered_set>
#include <memory>


class LogManager
{
public:
    LogManager(const std::vector<QString>& folders, const std::vector<std::shared_ptr<Format>>& formats);
    LogManager(const QString& filename, const std::vector<std::shared_ptr<Format>>& formats);
    LogManager(const QByteArray& data, const QString& filename, const std::vector<std::shared_ptr<Format>>& formats);

    const std::unordered_set<std::shared_ptr<Format>>& getFormats() const;
    const std::unordered_set<QString>& getModules() const;

    const std::unordered_set<QVariant, VariantHash>& getEnumList(const QString& field) const;

    std::chrono::system_clock::time_point getMinTime() const;
    std::chrono::system_clock::time_point getMaxTime() const;

    Session createSession(const std::unordered_set<QString>& modules, const std::chrono::system_clock::time_point& minTime, const std::chrono::system_clock::time_point& maxTime) const;

private:
    struct FileDesc
    {
        std::shared_ptr<Format> format;
        std::chrono::system_clock::time_point start;
        std::chrono::system_clock::time_point end;
    };

private:
    bool scanPlainFile(DirectoryScanner& scanner, const QString& filename, const QString& stem, const QString& extension, const std::vector<std::shared_ptr<Format>>& formats);
    bool scanArchive(DirectoryScanner& scanner, const QString& filename, const std::vector<std::shared_ptr<Format>>& formats);

    bool addFile(DirectoryScanner& scanner, const QString& filename, const QString& stem, const QString& extension, std::function<std::unique_ptr<QIODevice>(const QString&)> createFileFunc, const std::vector<std::shared_ptr<Format>>& formats);
    std::optional<FileDesc> scanLogFile(const QString& filename, std::function<std::unique_ptr<QIODevice>(const QString&)> createFileFunc, const std::vector<std::shared_ptr<Format>>& formats);
    std::chrono::system_clock::time_point getEndTime(const QString& filename, Log& log, const std::shared_ptr<Format>& format);

    Log createLog(const QString& filename, std::function<std::unique_ptr<QIODevice>(const QString&)> createFileFunc, std::shared_ptr<Format> format);

    static void readIntoBuffer(QIODevice& source, QBuffer& targetBuffer);

private:
    std::shared_ptr<LogStorage> logStorage;
};
