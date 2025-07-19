#include "LogUtils.h"

#include <QDateTime>


bool checkFormat(const QStringList& parts, const std::shared_ptr<Format>& format)
{
    int index = 0;
    for (const auto& field : format->fields)
    {
        if (parts.size() <= index)
        {
            if (!field.isOptional)
                return false;

            ++index;
            continue;
        }

        QRegularExpressionMatch match;
        bool hasMatch = true;
        if (field.regex.isValid() && !field.regex.pattern().isEmpty())
        {
            match = field.regex.match(parts[index]);
            hasMatch = match.hasMatch();
        }
        else
        {
            match = QRegularExpressionMatch();
        }

        bool isOutOfList = field.isEnum && !field.values.empty() && !field.values.contains(getValue(hasMatch ? match.captured(0) : parts[index], field, format));
        if ((!hasMatch || isOutOfList) && !field.isOptional)
            return false;

        if (field.isOptional && !hasMatch && !parts[index].isEmpty())
            continue;

        ++index;
    }
    return true;
}

QStringList splitLine(const QString& line, const std::shared_ptr<Format>& format)
{
    if (!format->separator.isEmpty())
    {
        auto parts = line.split(format->separator);
        for (auto& part : parts)
            part = part.trimmed();
        return parts;
    }
    else if (format->lineRegex.isValid() && !format->lineRegex.pattern().isEmpty())
    {
        QRegularExpressionMatch match = format->lineRegex.match(line);
        if (!match.hasMatch())
            throw std::runtime_error("line does not match the regex: " + format->lineRegex.pattern().toStdString());

        QStringList parts;
        for (const auto& field : format->fields)
        {
            QString value;
            if (match.hasCaptured(field.name))
                value = match.captured(field.name);
            else if (match.lastCapturedIndex() >= parts.size() + 1)
                value = match.captured(parts.size() + 1);
            parts << value.trimmed();
        }
        return parts;
    }
    else
    {
        throw std::logic_error("format does not have a valid separator or line regex");
    }
}

QVariant getValue(const QString& value, const Format::Field& field, const std::shared_ptr<Format>& format)
{
    switch (field.type)
    {
    case QMetaType::Bool:
    {
        static const std::unordered_set<QString> trueValues = {
            "true", "t", "1", "yes", "y", "on", "enabled"
        };
        return trueValues.contains(value.toLower());
    }
    case QMetaType::Int:
        return value;
    case QMetaType::UInt:
        return value.toUInt();
    case QMetaType::Double:
        return value.toDouble();
    case QMetaType::QString:
        return value;
    case QMetaType::QDateTime:
        return QDateTime::fromString(value, format->timeRegex);
    default:
        qCritical() << QString("Unsupported field type for field %1:").arg(field.name) << field.type;
        return QVariant();
    }
}

std::chrono::system_clock::time_point parseTime(const QString& timeStr, const std::shared_ptr<Format>& format)
{
    std::istringstream ss(timeStr.toStdString());
    std::chrono::system_clock::time_point tp;
    ss >> std::chrono::parse(format->timeRegex.toStdString(), tp);
    return tp;
}

int getEncodingWidth(QStringConverter::Encoding encoding)
{
    switch (encoding)
    {
    case QStringConverter::Utf8:
    case QStringConverter::Latin1:
        return 1;
    case QStringConverter::Utf16LE:
    case QStringConverter::Utf16BE:
        return 2;
    case QStringConverter::Utf32LE:
    case QStringConverter::Utf32BE:
        return 4;
    default:
        return 1;
    }
}
