#pragma once

#include "Format.h"
#include "Log.h"


struct LogMetadata
{
    std::shared_ptr<Format> format;
    QString filename;

    typedef std::function<std::shared_ptr<Log>(const QString&, const std::shared_ptr<Format>&)> FileBuilder;
    FileBuilder fileBuilder;
};
