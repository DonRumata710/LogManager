#pragma once

#include "Format.h"

#include <QIODevice>

#include <optional>


class Log
{
public:
    Log(std::unique_ptr<QIODevice>&& _file, const std::optional<QStringConverter::Encoding>& encoding, const std::shared_ptr<std::vector<Format::Comment>>& _comment);

    std::optional<QString> nextLine();

private:
    bool isCommentEnd(const Format::Comment& comment, const QString& line) const;

private:
    std::unique_ptr<QIODevice> file;
    std::unique_ptr<QTextStream> stream;
    std::shared_ptr<std::vector<Format::Comment>> comments;
    QByteArray buffer;
};
