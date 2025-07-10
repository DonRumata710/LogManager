#include "LogManager.h"

#include "LogUtils.h"

#include <quazip/quazipfile.h>

#include <QFile>
#include <QDebug>

#include <filesystem>


LogManager::LogManager(const std::vector<QString>& folders, const std::vector<std::shared_ptr<Format>>& formats) :
    logStorage(std::make_shared<LogStorage>())
{
    bool foundFiles = false;
    for (const auto& folder : folders)
    {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(folder.toStdString()))
        {
            if (!entry.is_regular_file())
                continue;

            auto filename = QString::fromStdString(entry.path().string());
            auto extension = QString::fromStdString(entry.path().extension().string());

            if (extension == ".zip" || extension == ".gz" || extension == ".tar" || extension == ".7z")
            {
                QuaZip zip(filename);
                if (!zip.open(QuaZip::mdUnzip))
                {
                    qDebug() << "Failed to open archive:" << filename;
                    continue;
                }

                QuaZipFileInfo info;
                for (bool more = zip.goToFirstFile(); more; more = zip.goToNextFile())
                {
                    zip.getCurrentFileInfo(&info);
                    auto innerFilename = info.name;
                    auto module = info.name.mid(0, info.name.lastIndexOf('.'));
                    auto innerExtension = info.name.mid(module.size());

                    auto fileCreationFunc = [filename](const QString& innerFilename) {
                        return std::make_unique<QuaZipFile>(filename, innerFilename);
                    };
                    auto result = addFile(innerFilename, module, innerExtension, fileCreationFunc, formats);
                    if (result)
                        foundFiles = true;
                }
            }
            else
            {
                auto module = QString::fromStdString(entry.path().stem().string());
                auto fileCreationFunc = [](const QString& filename) {
                    return std::make_unique<QFile>(filename);
                };
                auto result = addFile(filename, module, extension, fileCreationFunc, formats);
                if (!result)
                {
                    qDebug() << "No suitable format found for file:" << filename;
                    continue;
                }

                foundFiles = true;
            }
        }
    }

    if (!foundFiles)
    {
        throw std::runtime_error("no suitable files found in the specified folders.");
    }

    logStorage->finalize();
}

LogManager::LogManager(const QString& filename, const std::vector<std::shared_ptr<Format>>& formats) :
    logStorage(std::make_shared<LogStorage>())
{
    std::filesystem::path path = filename.toStdString();
    auto fileCreationFunc = [](const QString& filename) {
        return std::make_unique<QFile>(filename);
    };
    auto result = addFile(filename, QString::fromStdString(path.stem().string()), QString::fromStdString(path.extension().string()), fileCreationFunc, formats);
    if (!result)
    {
        qDebug() << "No suitable format found for file:" << filename;
        throw std::runtime_error("no suitable format found for file: " + path.string() + '.');
    }

    logStorage->finalize();
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

Session LogManager::createSession(const std::unordered_set<QString>& modules, const std::chrono::system_clock::time_point& minTime, const std::chrono::system_clock::time_point& maxTime) const
{
    return Session(std::make_shared<LogStorage>(logStorage->getNarrowedStorage(modules, minTime, maxTime)));
}

bool LogManager::addFile(const QString& filename, const QString& stem, const QString& extension, std::function<std::unique_ptr<QIODevice>(const QString&)> createFileFunc, const std::vector<std::shared_ptr<Format>>& formats)
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

    auto result = scanLogFile(filename, createFileFunc, actualFormats);
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
    metadata.fileBuilder = [this, createFileFunc](const QString& filename, const std::shared_ptr<Format>& format) {
        return std::make_shared<Log>(createLog(filename, createFileFunc, format));
    };
    metadata.filename = filename;
    logStorage->addLog(module, result->second, result->first, std::move(metadata));

    return true;
}

std::optional<std::pair<std::shared_ptr<Format>, std::chrono::system_clock::time_point>> LogManager::scanLogFile(const QString& filename, std::function<std::unique_ptr<QIODevice>(const QString&)> createFileFunc, const std::vector<std::shared_ptr<Format>>& formats)
{
    for (const auto& format : formats)
    {
        Log log(createLog(filename, createFileFunc, format));
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

Log LogManager::createLog(const QString& filename, std::function<std::unique_ptr<QIODevice>(const QString&)> createFileFunc, std::shared_ptr<Format> format)
{
    return Log(createFileFunc(filename), format->encoding, std::shared_ptr<std::vector<Format::Comment>>(format, &format->comments));
}
