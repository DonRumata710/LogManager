#include "LogUtils.h"

#include <QDateTime>
#include <nlohmann/json.hpp>


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
    else if (format->lineFormat == Format::LineFormat::Json)
    {
        nlohmann::json j;
        try {
            j = nlohmann::json::parse(line.toStdString());
        } catch (const std::exception& ex) {
            throw std::runtime_error(std::string("failed to parse JSON line: ") + ex.what());
        }

        QStringList parts;
        for (const auto& field : format->fields)
        {
            const nlohmann::json* current = &j;
            bool found = true;

            const QStringList tokens = field.name.split('.');
            for (const auto& token : tokens)
            {
                const std::string tokenStr = token.toStdString();
                if (current->contains(tokenStr))
                    current = &(*current)[tokenStr];
                else
                {
                    found = false;
                    break;
                }
            }

            if (found)
            {
                if (current->is_string())
                    parts << QString::fromStdString(current->get<std::string>()).trimmed();
                else
                    parts << QString::fromStdString(current->dump());
            }
            else
            {
                parts << QString();
            }
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
        return QDateTime::fromString(value, format->timeMask);
    default:
        qCritical() << QString("Unsupported field type for field %1:").arg(field.name) << field.type;
        return QVariant();
    }
}

std::chrono::nanoseconds parseFractionToNanos(std::string_view fracPart, int desiredDigits)
{
    if (desiredDigits < 1 || desiredDigits > 9)
        throw std::invalid_argument("Unsupported fractional digit count");

    int64_t value = 0;
    for (char c : fracPart)
    {
        if (c < '0' || c > '9')
            break;
        value = value * 10 + (c - '0');
    }

    int actualDigits = static_cast<int>(fracPart.size());
    while (actualDigits < desiredDigits)
    {
        value *= 10;
        ++actualDigits;
    }
    while (actualDigits > desiredDigits)
    {
        value /= 10;
        --actualDigits;
    }

    int scale = 9 - desiredDigits;
    for (int i = 0; i < scale; ++i)
        value *= 10;

    return std::chrono::nanoseconds(value);
}

std::chrono::system_clock::time_point parseTime(const QString& timeStr, const std::shared_ptr<Format>& format)
{
    if (!format || format->timeMask.isEmpty())
        throw std::invalid_argument("Invalid format");

    std::string input = timeStr.toStdString();

    size_t dotPos = input.find('.');
    std::string_view baseStr{ &*input.begin(), (dotPos != std::string_view::npos ? dotPos : input.size()) };
    std::string_view fracStr = (dotPos != std::string_view::npos ? std::string_view{ input.begin() + dotPos + 1, input.end() } : std::string_view{});

    std::chrono::system_clock::time_point tp;
    {
        std::istringstream ss(std::string{ baseStr });
        ss >> std::chrono::parse(format->timeMask.toStdString(), tp);
        if (!ss)
            throw std::runtime_error("Failed to parse base time part");
    }

    if (!fracStr.empty() && format->timeFractionalDigits > 0)
    {
        auto nanos = parseFractionToNanos(fracStr, format->timeFractionalDigits);
        tp += std::chrono::duration_cast<std::chrono::system_clock::duration>(nanos);
    }

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
