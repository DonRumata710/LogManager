#include "DirectoryScanner.h"

#include <quazip/quazipfile.h>

#include <QDebug>


void DirectoryScanner::addFile(const QString& moduleName, LogMetadata&& metadata, const std::chrono::system_clock::time_point& start, const std::chrono::system_clock::time_point& end)
{
    auto& module = modules[moduleName];
    QStringList path = getPath(metadata.filename);
    addFile(module, path, moduleName, std::move(metadata), start, end);
}

std::vector<DirectoryScanner::LogFile> DirectoryScanner::scan() const
{
    std::vector<LogFile> result;
    for (const auto& module : modules)
    {
        const auto& branch = module.second;
        auto files = getFiles(branch);
        for (auto& file : files)
        {
            if (!module.first.isEmpty())
            {
                if (file.module.isEmpty())
                    file.module = module.first;
                else
                    file.module += "/" + module.first;
            }
            result.push_back(std::move(file));
            qDebug() << "Found file:" << file.module << file.metadata.filename << "from" << file.start.time_since_epoch().count() << "to" << file.end.time_since_epoch().count();
        }
    }
    return result;
}

void DirectoryScanner::addFile(FileInfoBranch& module, const QStringList& path, const QString& moduleName, LogMetadata&& metadata, const std::chrono::system_clock::time_point& start, const std::chrono::system_clock::time_point& end)
{
    if (module.branch.empty())
    {
        if (!module.files)
        {
            module.path = path;
            module.files.emplace();
            insertFile(module.files.value(), std::move(metadata), start, end);
        }
        else
        {
            if (!mergeFile(module.files.value(), std::move(metadata), start, end))
            {
                FileInfoMap newFiles;
                insertFile(newFiles, std::move(metadata), start, end);

                createBranch(module, module.path, path, std::move(newFiles));
            }
        }
    }
    else
    {
        if (path == module.path)
        {
            if (!module.files)
            {
                module.files.emplace();
                insertFile(module.files.value(), std::move(metadata), start, end);
            }
            else if (!mergeFile(module.files.value(), std::move(metadata), start, end))
            {
                throw std::logic_error("Same path, same module, different metadata. This should not happen. (" + path.join('/').toStdString() + ")");
            }
        }
        else if (path.size() < module.path.size() ||  path.mid(0, module.path.size()) != module.path)
        {
            FileInfoMap newFiles;
            insertFile(newFiles, std::move(metadata), start, end);

            createBranch(module, module.path, path, std::move(newFiles));
        }
        else
        {
            auto it = module.branch.find(path[module.path.size()]);
            if (it == module.branch.end())
            {
                FileInfoMap newFiles;
                insertFile(newFiles, std::move(metadata), start, end);
                module.branch[path[module.path.size()]] = FileInfoBranch{ path.mid(module.path.size() + 1), std::unordered_map<QString, FileInfoBranch>{}, std::move(newFiles) };
            }
            else
            {
                addFile(it->second, path.mid(module.path.size() + 1), moduleName, std::move(metadata), start, end);
            }
        }
    }
}

void DirectoryScanner::insertFile(FileInfoMap& files, LogMetadata&& metadata, const std::chrono::system_clock::time_point& start, const std::chrono::system_clock::time_point& end)
{
    files[start] = std::make_unique<FileInfo>(metadata, start, end);
    files.emplace(end, std::unique_ptr<FileInfo>{});
}

void DirectoryScanner::createBranch(FileInfoBranch& currentBranch, QStringList oldPath, const QStringList& newPath, FileInfoMap&& newFiles)
{
    int i = 0;
    for (; i < oldPath.size(); ++i)
    {
        if (newPath.size() <= i || oldPath[i] != newPath[i])
            break;
    }

    if (i == oldPath.size() && i == newPath.size())
        throw std::logic_error("Same path, same module, different metadata. This should not happen. (" + oldPath.join('/').toStdString() + ")");

    if (i == oldPath.size() && i < newPath.size())
    {
        currentBranch.path = oldPath;
        currentBranch.branch[newPath[i]] = FileInfoBranch{ newPath.mid(i + 1), std::unordered_map<QString, FileInfoBranch>{}, std::move(newFiles) };
    }
    else if (i < oldPath.size() && i == newPath.size())
    {
        currentBranch.path = newPath;
        currentBranch.branch[oldPath[i]] = FileInfoBranch{ oldPath.mid(i + 1), std::unordered_map<QString, FileInfoBranch>{}, std::move(currentBranch.files) };
        currentBranch.files = std::move(newFiles);
    }
    else
    {
        currentBranch.path = oldPath.mid(0, i);
        currentBranch.branch[oldPath[i]] = FileInfoBranch{ oldPath.mid(i + 1), std::unordered_map<QString, FileInfoBranch>{}, std::move(currentBranch.files) };
        currentBranch.branch[newPath[i]] = FileInfoBranch{ newPath.mid(i + 1), std::unordered_map<QString, FileInfoBranch>{}, std::move(newFiles) };
        currentBranch.files.reset();
    }
}

bool DirectoryScanner::mergeFile(FileInfoMap& files, LogMetadata&& metadata, const std::chrono::system_clock::time_point& start, const std::chrono::system_clock::time_point& end)
{
    auto it = files.upper_bound(start);
    if (it == files.begin())
    {
        insertFile(files, std::move(metadata), start, end);
        return true;
    }

    --it;
    if (!it->second)
    {
        insertFile(files, std::move(metadata), start, end);
        return true;
    }

    return false;
}

QStringList DirectoryScanner::getPath(const QString& filename)
{
    QString path = filename;
    path.replace('\\', '/');
    QStringList pathParts = path.split('/', Qt::SplitBehaviorFlags::SkipEmptyParts);
    if (!pathParts.isEmpty())
        pathParts.pop_back();
    return pathParts;
}

std::vector<DirectoryScanner::LogFile> DirectoryScanner::getFiles(const FileInfoBranch& branch) const
{
    std::vector<LogFile> files;

    if (branch.files)
    {
        for (const auto& file : *branch.files)
        {
            if (file.second)
                files.emplace_back(QString{}, file.second->metadata, file.second->start, file.second->end);
        }
    }

    for (const auto& subBranch : branch.branch)
    {
        auto subFiles = getFiles(subBranch.second);
        for (auto& file : subFiles)
        {
            if (!subBranch.first.isEmpty())
            {
                if (file.module.isEmpty())
                    file.module = subBranch.first;
                else
                    file.module = subBranch.first + "/" + file.module;
            }
            files.push_back(std::move(file));
        }
    }

    return files;
}
