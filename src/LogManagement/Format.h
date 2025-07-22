#pragma once

#include <QRegularExpression>
#include <QVariant>

#include <unordered_set>
#include <vector>
#include <optional>


struct VariantHash
{
    size_t operator()(QVariant const& v) const noexcept
    {
        if (v.isNull())
            return 0;

        switch (v.metaType().id())
        {
        case QMetaType::Bool:
            return std::hash<bool>()(v.toBool());
        case QMetaType::Int:
            return std::hash<int>()(v.toInt());
        case QMetaType::UInt:
            return std::hash<unsigned>()(v.toUInt());
        case QMetaType::LongLong:
            return std::hash<long long>()(v.toLongLong());
        case QMetaType::ULongLong:
            return std::hash<unsigned long long>()(v.toULongLong());
        case QMetaType::Double:
            return std::hash<double>()(v.toDouble());
        case QMetaType::QString:
            return std::hash<QString>()(v.toString());
        case QMetaType::QByteArray:
            return std::hash<QByteArray>()(v.toByteArray());
        case QMetaType::QVariantMap:
        {
            size_t hash = 0;
            for (const auto& pair : v.toMap().toStdMap())
            {
                hash ^= VariantHash()(pair.first) ^ VariantHash()(pair.second);
            }
            return hash;
        }
        case QMetaType::QVariantList:
        {
            size_t hash = 0;
            for (const auto& item : v.toList())
            {
                hash ^= VariantHash()(item);
            }
            return hash;
        }
        case QMetaType::QVariantHash:
        {
            size_t hash = 0;
            auto val = v.toHash();
            for (const auto& key : val.keys())
            {
                hash ^= VariantHash()(key) ^ VariantHash()(val.value(key));
            }
            return hash;
        }
        default:
            return 0;
        }
    }
};


struct Format
{
    QString name;

    std::unordered_set<QString> modules;
    QRegularExpression logFileRegex;

    QString extension;
    std::optional<QStringConverter::Encoding> encoding;

    struct Comment
    {
        QString start;
        std::optional<QString> finish;
    };
    std::vector<Comment> comments;

    QString separator;
    QRegularExpression lineRegex;

    enum class LineFormat { None, Json };
    LineFormat lineFormat = LineFormat::None;

    int timeFieldIndex = -1;
    QString timeMask;
    int timeFractionalDigits = 0;

    struct Field
    {
        QString name;
        QRegularExpression regex;
        QMetaType::Type type;
        bool isOptional = false;

        bool isEnum = false;
        std::unordered_set<QVariant, VariantHash> values;
    };
    std::vector<Field> fields;
};
