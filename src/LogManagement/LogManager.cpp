#include "LogManager.h"

#include "LogUtils.h"

#include <quazip/quazipfile.h>

#include <QFile>
#include <QDebug>
#include <QBuffer>

#include <filesystem>


LogManager::LogManager(const std::vector<QString>& folders, const std::vector<std::shared_ptr<Format>>& formats)
{
    bool foundFiles = false;
    DirectoryScanner scanner;
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
                foundFiles |= scanArchive(scanner, filename, formats);
            }
            else
            {
                auto stem = QString::fromStdString(entry.path().stem().string());
                auto result = scanPlainFile(scanner, filename, stem, extension, formats);
                if (!result)
                    continue;

                foundFiles = true;
            }
        }
    }

    if (!foundFiles)
        throw std::runtime_error("no suitable files found in the specified folders");

    logStorage = std::make_shared<LogStorage>(scanner.scan());
}

LogManager::LogManager(const QString& filename, const std::vector<std::shared_ptr<Format>>& formats)
{
    DirectoryScanner scanner;
    auto extension = QString::fromStdString(std::filesystem::path(filename.toStdString()).extension().string());
    if (extension == ".zip" || extension == ".gz" || extension == ".tar" || extension == ".7z")
    {
        bool foundFiles = scanArchive(scanner, filename, formats);
        if (!foundFiles)
            throw std::runtime_error("no suitable files found in the specified archive: " + filename.toStdString());
    }
    else
    {
        std::filesystem::path path = filename.toStdString();
        auto result = scanPlainFile(scanner, filename, QString::fromStdString(path.stem().string()), QString::fromStdString(path.extension().string()), formats);
        if (!result)
            throw std::runtime_error("no suitable format found for file: " + path.string());
    }

    logStorage = std::make_shared<LogStorage>(scanner.scan());
}

LogManager::LogManager(const QByteArray& data, const QString& filename, const std::vector<std::shared_ptr<Format>>& formats)
{
    DirectoryScanner scanner;
    std::filesystem::path path = filename.toStdString();
    auto extension = QString::fromStdString(path.extension().string());
    auto stem = QString::fromStdString(path.stem().string());

    auto fileCreationFunc = [data](const QString&) {
        std::unique_ptr<QBuffer> buffer = std::make_unique<QBuffer>();
        buffer->setData(data);
        return buffer;
    };

    auto result = addFile(scanner, filename, stem, extension, fileCreationFunc, formats);
    if (!result)
        throw std::runtime_error("no suitable format found for buffer");

    logStorage = std::make_shared<LogStorage>(scanner.scan());
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

bool LogManager::scanPlainFile(DirectoryScanner& scanner, const QString& filename, const QString& stem, const QString& extension, const std::vector<std::shared_ptr<Format>>& formats)
{
    auto fileCreationFunc = [](const QString& filename) {
        return std::make_unique<QFile>(filename);
    };
    return addFile(scanner, filename, stem, extension, fileCreationFunc, formats);
}

bool LogManager::scanArchive(DirectoryScanner& scanner, const QString& filename, const std::vector<std::shared_ptr<Format> >& formats)
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
            auto result = addFile(scanner, innerFilename, module, innerExtension, fileCreationFunc, formats);
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

bool LogManager::addFile(DirectoryScanner& scanner, const QString& filename, const QString& stem, const QString& extension, std::function<std::unique_ptr<QIODevice>(const QString&)> createFileFunc, const std::vector<std::shared_ptr<Format>>& formats)
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

    if (result->format->logFileRegex.isValid() && !result->format->logFileRegex.pattern().isEmpty())
    {
        auto matchIt = regexMatches.find(result->format);
        if (matchIt != regexMatches.end())
            module = matchIt->second;
        else
            module.clear();
    }

    LogMetadata metadata;
    metadata.format = result->format;
    metadata.fileBuilder = [createFileFunc](const QString& filename, const std::shared_ptr<Format>& format) {
        return std::make_shared<Log>(LogManager::createLog(filename, createFileFunc, format));
    };
    metadata.filename = filename;
    scanner.addFile(module, std::move(metadata), result->start, result->end);

    return true;
}

std::optional<LogManager::FileDesc> LogManager::scanLogFile(const QString& filename, std::function<std::unique_ptr<QIODevice>(const QString&)> createFileFunc, const std::vector<std::shared_ptr<Format>>& formats)
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

            auto start = parseTime(parts[format->timeFieldIndex], format);
            auto end = getEndTime(filename, log, format);
            return FileDesc{ format, start, end };
        }
        catch (const std::exception& ex)
        {
            qDebug() << "Failed to scan log file" << filename << "with format" << format->name << ":" << ex.what();
            continue;
        }
    }

    return std::nullopt;
}

std::chrono::system_clock::time_point LogManager::getEndTime(const QString& filename, Log& log, const std::shared_ptr<Format>& format)
{
    log.goToEnd();

    QStringList parts;
    do
    {
        auto line = log.prevLine();
        if (!line)
        {
            qCritical() << "Log file is empty or could not be read:" << filename;
            break;
        }

        try
        {
            parts = splitLine(line.value(), format);
        }
        catch (const std::exception& ex)
        {
            continue;
        }

        if (parts.size() <= format->timeFieldIndex)
            continue;
    }
    while(!checkFormat(parts, format));

    if (parts.empty())
        throw std::runtime_error("Log file is empty or does not contain valid entries: " + filename.toStdString());

    return parseTime(parts[format->timeFieldIndex], format);
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
