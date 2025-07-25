#pragma once

#include "LogMetadata.h"

#include <QString>

#include <chrono>


class DirectoryScanner
{
public:
    void addFile(const QString& module, LogMetadata&& metadata, const std::chrono::system_clock::time_point& start, const std::chrono::system_clock::time_point& end);

    struct LogFile
    {
        QString module;
        LogMetadata metadata;
        std::chrono::system_clock::time_point start;
        std::chrono::system_clock::time_point end;
    };
    std::vector<LogFile> scan() const;

private:
    struct FileInfo
    {
        LogMetadata metadata;
        std::chrono::system_clock::time_point start;
        std::chrono::system_clock::time_point end;
    };

    typedef std::map<std::chrono::system_clock::time_point, std::unique_ptr<FileInfo>> FileInfoMap;

    struct FileInfoBranch
    {
        QStringList path;
        std::unordered_map<QString, FileInfoBranch> branch;
        std::optional<FileInfoMap> files;
    };

private:
    void addFile(FileInfoBranch& branch, const QStringList& path, const QString& module, LogMetadata&& metadata, const std::chrono::system_clock::time_point& start, const std::chrono::system_clock::time_point& end);

    void insertFile(FileInfoMap& files, LogMetadata&& metadata, const std::chrono::system_clock::time_point& start, const std::chrono::system_clock::time_point& end);
    void createBranch(FileInfoBranch& currentBranch, QStringList oldPath, const QStringList& newPath, FileInfoMap&& newFiles);
    bool mergeFile(FileInfoMap& files, LogMetadata&& metadata, const std::chrono::system_clock::time_point& end, const std::chrono::system_clock::time_point& start);

    QStringList getPath(const QString& filename);

    std::vector<LogFile> getFiles(const FileInfoBranch& branch) const;

private:
    std::unordered_map<QString, FileInfoBranch> modules;
};
