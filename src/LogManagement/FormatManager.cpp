#include "FormatManager.h"

#include <QDir>
#include <QSettings>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMetaType>


FormatManager::FormatManager()
{
    loadFormats();
}

void FormatManager::updateFormats()
{
    formats.clear();
    loadFormats();
}

std::map<std::string, std::shared_ptr<Format>> FormatManager::getFormats() const
{
    return formats;
}

void FormatManager::addFormat(const std::shared_ptr<Format>& format)
{
    if (!format)
        throw std::logic_error("format cannot be null");

    if (format->name.isEmpty())
        throw std::logic_error("format name cannot be empty");

    QJsonDocument doc;
    QJsonObject formatObj;

    QJsonArray moduleArray;
    for (const auto& module : format->modules)
        moduleArray.append(module);
    if (!moduleArray.empty())
        formatObj.insert("modules", moduleArray);

    if (format->logFileRegex.isValid() && !format->logFileRegex.pattern().isEmpty())
        formatObj.insert("logFileRegex", format->logFileRegex.pattern());
    formatObj.insert("extension", format->extension);
    if (format->encoding)
        formatObj.insert("encoding", QStringConverter::nameForEncoding(format->encoding.value()));

    QJsonArray commentArray;
    for (const auto& comment : format->comments)
    {
        QJsonObject commentObj;
        commentObj.insert("start", comment.start);
        if (comment.finish && !comment.finish->isEmpty())
            commentObj.insert("finish", comment.finish.value());
        commentArray.append(commentObj);
    }
    if (!commentArray.empty())
        formatObj.insert("comments", commentArray);

    if (!format->separator.isEmpty())
        formatObj.insert("separator", format->separator);
    if (format->lineRegex.isValid() && !format->lineRegex.pattern().isEmpty())
        formatObj.insert("lineRegex", format->lineRegex.pattern());
    if (format->lineFormat == Format::LineFormat::Json)
        formatObj.insert("lineFormat", "json");

    formatObj.insert("timeFieldIndex", format->timeFieldIndex);
    formatObj.insert("timeMask", format->timeMask);
    formatObj.insert("timeFractionalDigits", format->timeFractionalDigits);

    QJsonArray fieldArray;
    for (const auto& field : format->fields)
    {
        QJsonObject fieldObj;

        fieldObj.insert("name", field.name);
        fieldObj.insert("regex", field.regex.pattern());
        fieldObj.insert("type", QMetaType(field.type).name());
        fieldObj.insert("optional", field.isOptional);
        fieldObj.insert("enum", field.isEnum);

        if (!field.values.empty())
        {
            QJsonArray valuesArray;
            for (const auto& value : field.values)
                valuesArray.append(value.toJsonValue());
            fieldObj.insert("values", valuesArray);
        }

        fieldArray.append(fieldObj);
    }
    formatObj.insert("fields", fieldArray);

    doc.setObject(formatObj);

    QString filePath = format->name + ".json";
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly))
    {
        qWarning() << "Failed to open file for writing:" << filePath;
        return;
    }

    if (file.write(doc.toJson()) == -1)
    {
        qWarning() << "Failed to write to file:" << filePath;
        return;
    }

    file.close();

    formats[format->name.toStdString()] = format;

    qDebug() << "Format created:" << format->name;
}

void FormatManager::removeFormat(const QString& name)
{
    auto it = formats.find(name.toStdString());
    if (it != formats.end())
    {
        formats.erase(it);
        QString filePath = name + ".json";
        if (QFile::exists(filePath))
        {
            if (!QFile::remove(filePath))
                qWarning() << "Failed to remove file:" << filePath;
            else
                qDebug() << "Format removed:" << name;
        }
    }
    else
    {
        qWarning() << "Format not found:" << name;
    }
}

void FormatManager::loadFormats()
{
    auto entryList = QDir::current().entryList(QStringList() << "*.json", QDir::Files);
    for (const auto& file : std::as_const(entryList))
    {
        auto doc = loadJson(file);
        if (doc.isObject())
        {
            QJsonObject formatObj = doc.object();
            std::shared_ptr<Format> format = std::make_shared<Format>();
            format->name = file.left(file.lastIndexOf('.'));

            if (formatObj.contains("modules"))
            {
                auto moduleArray = formatObj.value("modules").toArray();
                for (const auto& module : std::as_const(moduleArray))
                    format->modules.insert(module.toString());
            }

            if (formatObj.contains("logFileRegex"))
                format->logFileRegex = QRegularExpression(formatObj.value("logFileRegex").toString());
            if (formatObj.contains("extension"))
                format->extension = formatObj.value("extension").toString();
            if (formatObj.contains("encoding"))
                format->encoding = QStringConverter::encodingForName(formatObj.value("encoding").toString().toStdString().c_str());

            if (formatObj.contains("comments"))
            {
                auto commentArray = formatObj.value("comments").toArray();
                for (const auto& comment : std::as_const(commentArray))
                {
                    Format::Comment& c = format->comments.emplace_back();
                    QJsonObject commentObj = comment.toObject();
                    c.start = commentObj.value("start").toString();
                    if (commentObj.contains("finish"))
                        c.finish = commentObj.value("finish").toString();
                }
            }

            if (formatObj.contains("separator"))
                format->separator = formatObj.value("separator").toString();
            if (formatObj.contains("lineRegex"))
                format->lineRegex = QRegularExpression(formatObj.value("lineRegex").toString());
            if (formatObj.contains("lineFormat") && formatObj.value("lineFormat").toString() == "json")
                format->lineFormat = Format::LineFormat::Json;
            else
                format->lineFormat = Format::LineFormat::None;

            format->timeFieldIndex = formatObj.value("timeFieldIndex").toInt(-1);
            format->timeMask = formatObj.value("timeMask").toString();
            format->timeFractionalDigits = formatObj.value("timeFractionalDigits").toInt(0);

            auto fieldArray = formatObj.value("fields").toArray();
            for (const auto& field : std::as_const(fieldArray))
            {
                Format::Field& f = format->fields.emplace_back();
                QJsonObject fieldObj = field.toObject();

                f.name = fieldObj.value("name").toString();
                f.regex = QRegularExpression(fieldObj.value("regex").toString());
                f.type = static_cast<QMetaType::Type>(QMetaType::fromName(fieldObj.value("type").toString().toLatin1().constData()).id());

                if (fieldObj.contains("optional"))
                    f.isOptional = fieldObj.value("optional").toBool();

                if (fieldObj.contains("enum"))
                    f.isEnum = fieldObj.value("enum").toBool();
                if (fieldObj.contains("values"))
                {
                    auto valuesArray = fieldObj.value("values").toArray();
                    for (const auto& value : std::as_const(valuesArray))
                        f.values.emplace(value.toVariant());
                }
            }

            formats[std::filesystem::path(file.toStdString()).stem().string()] = std::move(format);
        }
    }

    qDebug() << "Formats loaded:" << formats.size();
}

QJsonDocument FormatManager::loadJson(const QString& filePath)
{
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly))
    {
        qWarning() << "Failed to open file for reading:" << filePath;
        return {};
    }

    QByteArray raw = f.readAll();
    f.close();

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(raw, &err);
    if (err.error != QJsonParseError::NoError)
    {
        qWarning() << "Parsing error JSON:" << err.errorString();
        return {};
    }
    return doc;
}
