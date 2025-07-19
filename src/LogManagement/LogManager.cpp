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
                foundFiles |= scanArchive(filename, formats);
            }
            else
            {
                auto module = QString::fromStdString(entry.path().stem().string());
                auto fileCreationFunc = [](const QString& filename) {
                    return std::make_unique<QFile>(filename);
                };
                auto result = addFile(filename, module, extension, fileCreationFunc, formats);
                if (!result)
                    continue;

                foundFiles = true;
            }
        }
    }

    if (!foundFiles)
        throw std::runtime_error("no suitable files found in the specified folders");

    logStorage->finalize();
}

LogManager::LogManager(const QString& filename, const std::vector<std::shared_ptr<Format>>& formats) :
    logStorage(std::make_shared<LogStorage>())
{
    auto extension = QString::fromStdString(std::filesystem::path(filename.toStdString()).extension().string());
    if (extension == ".zip" || extension == ".gz" || extension == ".tar" || extension == ".7z")
    {
        bool foundFiles = scanArchive(filename, formats);
        if (!foundFiles)
            throw std::runtime_error("no suitable files found in the specified archive: " + filename.toStdString());
    }
    else
    {
        std::filesystem::path path = filename.toStdString();
        auto fileCreationFunc = [](const QString& filename) {
            return std::make_unique<QFile>(filename);
        };
        auto result = addFile(filename, QString::fromStdString(path.stem().string()), QString::fromStdString(path.extension().string()), fileCreationFunc, formats);
        if (!result)
            throw std::runtime_error("no suitable format found for file: " + path.string());
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

bool LogManager::scanArchive(const QString& filename, const std::vector<std::shared_ptr<Format> >& formats)
{
    QuaZip zip(filename);
    if (!zip.open(QuaZip::mdUnzip))
    {
        qDebug() << "Failed to open archive:" << filename;
        return false;
    }

    QuaZipFileInfo info;
    bool foundFiles = false;
    for (bool more = zip.goToFirstFile(); more; more = zip.goToNextFile())
    {
        try
        {
            zip.getCurrentFileInfo(&info);
            auto innerFilename = info.name;

            int slashPos = innerFilename.lastIndexOf('/');
            if (slashPos == -1)
                slashPos = innerFilename.lastIndexOf('\\');
            ++slashPos;

            int dotPos = innerFilename.lastIndexOf('.');
            if (dotPos < slashPos)
                dotPos = -1;

            auto module = innerFilename.mid(slashPos, dotPos - slashPos);
            auto innerExtension = info.name.mid(dotPos);

            auto fileCreationFunc = [filename](const QString& innerFilename) {
                QuaZipFile zipFile(filename, innerFilename);
                std::unique_ptr<QBuffer> buffer = std::make_unique<QBuffer>();
                LogManager::readIntoBuffer(zipFile, *buffer);
                return buffer;
            };
            auto result = addFile(innerFilename, module, innerExtension, fileCreationFunc, formats);
            if (result)
                foundFiles = true;
        }
        catch (const std::exception& ex)
        {
            qDebug() << "Failed to process file in archive" << info.name << "because of error:" << ex.what();
            continue;
        }
    }
    return foundFiles;
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
        try
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
        catch (const std::exception& ex)
        {
            qDebug() << "Failed to scan log file" << filename << "with format" << format->name << ":" << ex.what();
            continue;
        }
    }

    return std::nullopt;
}

Log LogManager::createLog(const QString& filename, std::function<std::unique_ptr<QIODevice>(const QString&)> createFileFunc, std::shared_ptr<Format> format)
{
    return Log(createFileFunc(filename), format->encoding, std::shared_ptr<std::vector<Format::Comment>>(format, &format->comments));
}

void LogManager::readIntoBuffer(QIODevice& source, QBuffer& targetBuffer)
{
    if (!source.isOpen() && !source.open(QIODevice::ReadOnly))
        throw std::runtime_error("cannot open log file: " + source.errorString().toStdString());

    QByteArray data = source.readAll();
    targetBuffer.close();
    targetBuffer.setData(data);
}
