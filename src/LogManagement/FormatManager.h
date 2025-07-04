#pragma once

#include "Format.h"

#include <map>
#include <memory>


class FormatManager
{
public:
    FormatManager();

    std::map<std::string, std::shared_ptr<Format>> getFormats() const;

    void addFormat(const std::shared_ptr<Format>& format);
    void removeFormat(const QString& name);

private:
    void loadFormats();
    QJsonDocument loadJson(const QString& filePath);

private:
    std::map<std::string, std::shared_ptr<Format>> formats;
};
