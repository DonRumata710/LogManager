#include "Log.h"


Log::Log(std::unique_ptr<QIODevice>&& _file, const std::optional<QStringConverter::Encoding>& encoding, const std::shared_ptr<std::vector<Format::Comment>>& _comments) :
    file(std::move(_file)),
    stream(std::make_unique<QTextStream>(file.get())),
    comments(_comments)
{
    if (!file->isOpen())
    {
        if (!file->open(QIODevice::ReadOnly | QIODevice::Text))
            throw std::runtime_error("Cannot open log file: " + file->errorString().toStdString());
    }

    if (encoding)
    {
        stream->setEncoding(encoding.value());
    }
    else
    {
        QByteArray head = file->read(4);
        file->seek(0);

        if (head.startsWith("\xEF\xBB\xBF"))
        {
            stream->setEncoding(QStringDecoder::Utf8);
        }
        else if (head.startsWith("\xFF\xFE"))
        {
            stream->setEncoding(QStringDecoder::Utf16LE);
        }
        else if (head.startsWith("\xFE\xFF"))
        {
            stream->setEncoding(QStringDecoder::Utf16BE);
        }
        else if (head.startsWith("\xFF\xFE\x00\x00"))
        {
            stream->setEncoding(QStringDecoder::Utf32LE);
        }
        else if (head.startsWith("\x00\x00\xFE\xFF"))
        {
            stream->setEncoding(QStringDecoder::Utf32BE);
        }
        else
        {
            stream->setEncoding(QStringDecoder::Utf8);
        }
    }
}

std::optional<QString> Log::nextLine()
{
    QString line;
    std::optional<Format::Comment> comment;
    while (!(line = stream->readLine()).isEmpty())
    {
        if (comment)
        {
            if (isCommentEnd(comment.value(), line))
                comment.reset();
            continue;
        }

        if (comments)
        {
            bool flag = false;

            for (const auto& c : *comments)
            {
                if (line.startsWith(c.start))
                {
                    if (c.finish && !line.endsWith(c.finish.value()))
                        comment = c;

                    flag = true;
                    break;
                }
            }

            if (flag)
                continue;
        }

        return line;
    }
    return std::nullopt;
}

bool Log::isCommentEnd(const Format::Comment& comment, const QString& line) const
{
    return comment.finish && line.endsWith(comment.finish.value());
}
