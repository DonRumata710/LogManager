#pragma once

#include "Format.h"

#include <QStringList>
#include <QVariant>


bool checkFormat(const QStringList& parts, const std::shared_ptr<Format>& format);
QStringList splitLine(const QString& line, const std::shared_ptr<Format>& format);
QVariant getValue(const QString& value, const Format::Field& field, const std::shared_ptr<Format>& format);
std::chrono::system_clock::time_point parseTime(const QString& timeStr, const std::shared_ptr<Format>& format);
int getEncodingWidth(QStringConverter::Encoding encoding);
