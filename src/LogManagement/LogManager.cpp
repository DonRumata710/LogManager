#include "LogManager.h"

#include "LogUtils.h"

#include <QFile>
#include <QDebug>

#include <filesystem>


LogManager::LogManager(const std::vector<QString>& folders, const std::vector<std::shared_ptr<Format>>& formats)
{
    logStorage = std::make_shared<LogStorage>();

    bool foundFiles = false;
    for (const auto& folder : folders)
    {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(folder.toStdString()))
        {
            if (!entry.is_regular_file())
                continue;

            auto filename = QString::fromStdString(entry.path().string());
            auto stem = QString::fromStdString(entry.path().stem().string());
            auto extension = QString::fromStdString(entry.path().extension().string());
            auto result = addFile(filename, stem, extension, formats);
            if (!result)
            {
                qDebug() << "No suitable format found for file:" << filename;
                continue;
            }

            foundFiles = true;
        }
    }

    if (!foundFiles)
    {
        throw std::runtime_error("No suitable files found in the specified folders.");
    }
}

LogManager::LogManager(const QString& filename, const std::vector<std::shared_ptr<Format>> &formats)
{
    logStorage = std::make_shared<LogStorage>();

    std::filesystem::path path = filename.toStdString();
    auto result = addFile(QString::fromStdString(path.string()), QString::fromStdString(path.stem().string()), QString::fromStdString(path.extension().string()), formats);
    if (!result)
    {
        qDebug() << "No suitable format found for file:" << filename;
        throw std::runtime_error("No suitable format found for file: " + path.string() + '.');
    }
}

const std::unordered_set<std::shared_ptr<Format>>& LogManager::getFormats() const
{
    return logStorage->getFormats();
}

const std::unordered_set<QString>& LogManager::getModules() const
{
    return logStorage->getModules();
}

const std::unordered_set<QVariant, VariantHash>& LogManager::getEnumList(const QString& field) const
{
    return logStorage->getEnumList(field);
}

std::chrono::system_clock::time_point LogManager::getMinTime() const
{
    return logStorage->getMinTime();
}

std::chrono::system_clock::time_point LogManager::getMaxTime() const
{
    return logStorage->getMaxTime();
}

LogEntryIterator LogManager::getIterator(const std::chrono::system_clock::time_point& startTime, const std::chrono::system_clock::time_point& endTime)
{
    return LogEntryIterator(logStorage, startTime, endTime);
}

bool LogManager::addFile(const QString& filename, const QString& stem, const QString& extension, const std::vector<std::shared_ptr<Format>>& formats)
{
    QString module = stem;

    std::vector<std::shared_ptr<Format>> actualFormats;
    std::unordered_map<std::shared_ptr<Format>, QString> regexMatches;
    for (const auto& format : formats)
    {
        QString formatModule = module;
        if (format->logFileRegex.isValid() && !format->logFileRegex.pattern().isEmpty())
        {
            QRegularExpressionMatch match = format->logFileRegex.match(stem);
            if (!match.hasMatch())
                continue;

            if (match.hasCaptured("module"))
                regexMatches[format] = match.captured("module");
        }

        if ((format->modules.empty() || format->modules.contains(formatModule)) && format->extension == extension)
            actualFormats.push_back(format);
    }

    if (actualFormats.empty())
        return false;

    auto result = scanLogFile(filename, actualFormats);
    if (!result)
        return false;

    qDebug() << "File discovered:" << filename;

    if (result->first->logFileRegex.isValid() && !result->first->logFileRegex.pattern().isEmpty())
    {
        auto matchIt = regexMatches.find(result->first);
        if (matchIt != regexMatches.end())
            module = matchIt->second;
        else
            module = result->first->name;
    }

    LogStorage::LogMetadata metadata;
    metadata.format = result->first;
    metadata.fileBuilder = [this](const QString& filename, const std::shared_ptr<Format>& format) {
        return std::make_shared<Log>(createLog(filename, format));
    };
    metadata.filename = filename;
    logStorage->addLog(module, result->second, result->first, std::move(metadata));

    return true;
}

std::optional<std::pair<std::shared_ptr<Format>, std::chrono::system_clock::time_point>> LogManager::scanLogFile(const QString& filename, const std::vector<std::shared_ptr<Format>>& formats)
{
    for (const auto& format : formats)
    {
        Log log(createLog(filename, format));
        auto line = log.nextLine();
        if (!line)
            continue;

        auto parts = splitLine(line.value(), format);
        if (parts.size() <= format->timeFieldIndex)
            continue;

        if (!checkFormat(parts, format))
            continue;

        auto time = parseTime(parts[format->timeFieldIndex], format);
        return std::make_pair(format, time);
    }

    return std::nullopt;
}

Log LogManager::createLog(const QString& path, std::shared_ptr<Format> format)
{
    return Log(std::make_unique<QFile>(path), format->encoding, std::shared_ptr<std::vector<Format::Comment>>(format, &format->comments));
}
