#pragma once

#include "Format.h"

#include <QIODevice>

#include <optional>
#include <memory>
#include <vector>


class Log
{
public:
    Log(std::unique_ptr<QIODevice>&& _file, const std::optional<QStringConverter::Encoding>& encoding, const std::shared_ptr<std::vector<Format::Comment>>& _comment);
    Log(std::unique_ptr<QIODevice>&& _file, qint64 pos, const std::optional<QStringConverter::Encoding>& encoding, const std::shared_ptr<std::vector<Format::Comment>>& _comment);

    std::optional<QString> prevLine();
    std::optional<QString> nextLine();

    void seek(qint64 pos);
    void goToEnd();
    qint64 getFilePosition() const;

private:
    QStringConverter::Encoding checkFileForBom();

    bool getToNextLine(qint64& pos, QString& line);

    bool isCommentEnd(const Format::Comment& comment, const QString& line) const;

private:
    std::unique_ptr<QIODevice> file;
    QStringDecoder decoder;
    std::shared_ptr<std::vector<Format::Comment>> comments;
    qint64 fileStart = 0;
    QByteArray buffer;
    qsizetype encodingWidth = 1;
};
