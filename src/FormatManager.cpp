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

std::map<std::string, std::shared_ptr<Format>> FormatManager::getFormats() const
{
    return formats;
}

void FormatManager::addFormat(const QString& name, const std::shared_ptr<Format>& format)
{
    if (format && !name.isEmpty())
    {
        QJsonDocument doc;
        QJsonObject formatObj;

        QJsonArray moduleArray;
        for (const auto& module : format->modules)
            moduleArray.append(module);
        formatObj.insert("modules", moduleArray);

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
        formatObj.insert("comments", commentArray);

        formatObj.insert("separator", format->separator);

        formatObj.insert("timeFieldIndex", format->timeFieldIndex);

        formatObj.insert("timeRegex", format->timeRegex);

        QJsonArray fieldArray;
        for (const auto& field : format->fields)
        {
            QJsonObject fieldObj;
            fieldObj.insert("name", field.name);
            fieldObj.insert("regex", field.regex.pattern());
            fieldObj.insert("type", QMetaType::typeName(field.type));
            fieldArray.append(fieldObj);
        }
        formatObj.insert("fields", fieldArray);

        doc.setObject(formatObj);

        QString filePath = name + ".json";
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

        formats[name.toStdString()] = format;

        qDebug() << "Format created: " << name;
    }
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
                qDebug() << "Format removed: " << name;
        }
    }
    else
    {
        qWarning() << "Format not found: " << name;
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

            auto moduleArray = formatObj.value("modules").toArray();
            for (const auto& module : std::as_const(moduleArray))
                format->modules.insert(module.toString());

            if (formatObj.contains("extension"))
                format->extension = formatObj.value("extension").toString();
            if (formatObj.contains("encoding"))
                format->encoding = QStringConverter::encodingForName(formatObj.value("encoding").toString().toStdString().c_str());

            auto commentArray = formatObj.value("comments").toArray();
            for (const auto& comment : std::as_const(commentArray))
            {
                Format::Comment& c = format->comments.emplace_back();
                QJsonObject commentObj = comment.toObject();
                c.start = commentObj.value("start").toString();
                if (commentObj.contains("finish"))
                    c.finish = commentObj.value("finish").toString();
            }

            format->separator = formatObj.value("separator").toString();

            format->timeFieldIndex = formatObj.value("timeFieldIndex").toInt(-1);

            format->timeRegex = formatObj.value("timeRegex").toString();

            auto fieldArray = formatObj.value("fields").toArray();
            for (const auto& field : std::as_const(fieldArray))
            {
                Format::Fields& f = format->fields.emplace_back();
                QJsonObject fieldObj = field.toObject();
                f.name = fieldObj.value("name").toString();
                f.regex = QRegularExpression(fieldObj.value("regex").toString());
                f.type = static_cast<QMetaType::Type>(QMetaType::type(fieldObj.value("type").toString().toLatin1().constData()));
            }

            formats[std::filesystem::path(file.toStdString()).stem().string()] = std::move(format);
        }
    }
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
