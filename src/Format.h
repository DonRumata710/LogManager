#pragma once

#include <QRegularExpression>
#include <QVariant>

#include <unordered_set>
#include <vector>


struct Format
{    
    std::unordered_set<QString> modules;

    std::optional<QStringConverter::Encoding> encoding;

    struct Comment
    {
        QString start;
        std::optional<QString> finish;
    };
    std::vector<Comment> comments;

    QString separator;
    int timeFieldIndex = -1;
    QString timeRegex;

    struct Fields
    {
        QString name;
        QRegularExpression regex;
        QMetaType::Type type;
    };
    std::vector<Fields> fields;
};
