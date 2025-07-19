#include "Log.h"

#include "LogUtils.h"


Log::Log(std::unique_ptr<QIODevice>&& _file, const std::optional<QStringConverter::Encoding>& _encoding, const std::shared_ptr<std::vector<Format::Comment>>& _comments) :
    file(std::move(_file)),
    comments(_comments)
{
    if (!file->isOpen())
    {
        if (!file->open(QIODevice::ReadOnly))
            throw std::runtime_error("cannot open log file: " + file->errorString().toStdString());
    }

    QStringConverter::Encoding encoding = checkFileForBom();
    if (_encoding)
        encoding = _encoding.value();

    encodingWidth = getEncodingWidth(encoding);

    decoder = QStringDecoder(encoding);
    fileStart = file->pos();
}

Log::Log(std::unique_ptr<QIODevice>&& _file, qint64 pos, const std::optional<QStringConverter::Encoding>& encoding, const std::shared_ptr<std::vector<Format::Comment>>& _comment) :
    Log(std::move(_file), encoding, _comment)
{
    seek(pos);
}

std::optional<QString> Log::prevLine()
{
    QString line;
    std::optional<Format::Comment> comment;
    decoder.resetState();
    while (file->pos() != fileStart)
    {
        QByteArray charData(encodingWidth, '\0');

        int prevPos = file->pos();
        while (prevPos > fileStart)
        {
            prevPos -= encodingWidth;
            file->seek(prevPos);
            if (file->read(charData.data(), encodingWidth) != encodingWidth)
                return std::nullopt;

            QString ch = decoder.decode(QByteArrayView(charData));
            decoder.resetState();
            if (ch != "\n" && ch != "\r")
            {
                line.prepend(ch);
            }
            else if (!line.isEmpty())
            {
                break;
            }
        }

        file->seek(prevPos);

        if (line.isEmpty())
            continue;

        if (comment)
        {
            if (line.startsWith(comment.value().start))
                comment.reset();
            continue;
        }

        if (comments)
        {
            bool flag = false;

            for (const auto& c : *comments)
            {
                if (c.finish)
                {
                    if (line.endsWith(c.finish.value()))
                    {
                        if (!line.startsWith(c.start))
                            comment = c;

                        flag = true;
                        break;
                    }
                }

                if (line.startsWith(c.start))
                {
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

std::optional<QString> Log::nextLine()
{
    std::optional<Format::Comment> comment;
    while (!buffer.isEmpty() || !file->atEnd())
    {
        QString line;

        qsizetype pos = buffer.indexOf('\n');
        while(true)
        {
            if (getToNextLine(pos, line))
                break;

            auto oldSize = buffer.size();
            buffer.append(file->read(512));
            if (oldSize == buffer.size())
            {
                pos = buffer.size();
                continue;
            }

            pos = buffer.indexOf('\n', oldSize);
        }

        if (line.isEmpty())
            continue;

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

void Log::seek(qint64 pos)
{
    if (pos > 0)
    {
        if (!file->seek(pos))
            throw std::runtime_error("cannot seek to position " + std::to_string(pos) + " in log file: " + file->errorString().toStdString());
    }
    else if (pos < 0)
    {
        if (!file->seek(file->size() + pos))
            throw std::runtime_error("cannot seek to position " + std::to_string(pos) + " in log file: " + file->errorString().toStdString());
    }
    else
    {
        file->seek(0);
    }
    buffer.clear();
}

void Log::goToEnd()
{
    seek(file->size());
}

qint64 Log::getFilePosition() const
{
    return file->pos() - buffer.size();
}

QStringConverter::Encoding Log::checkFileForBom()
{
    QStringConverter::Encoding encoding;

    QByteArray head = file->read(4);
    file->seek(0);

    if (head.startsWith(QByteArray{ "\xEF\xBB\xBF", 3 }))
    {
        encoding = QStringConverter::Utf8;
        file->seek(3);
    }
    else if (head.startsWith(QByteArray{ "\xFF\xFE", 2 }))
    {
        encoding = QStringDecoder::Utf16LE;
        file->seek(2);
    }
    else if (head.startsWith(QByteArray{ "\xFE\xFF", 2 }))
    {
        encoding = QStringDecoder::Utf16BE;
        file->seek(2);
    }
    else if (head.startsWith(QByteArray{ "\xFF\xFE\x00\x00", 4 }))
    {
        encoding = QStringDecoder::Utf32LE;
    }
    else if (head.startsWith(QByteArray{ "\x00\x00\xFE\xFF", 4 }))
    {
        encoding = QStringDecoder::Utf32BE;
    }
    else
    {
        encoding = QStringDecoder::Utf8;
    }

    return encoding;
}

bool Log::getToNextLine(qint64& pos, QString& line)
{
    while (pos != -1)
    {
        pos = ((pos + encodingWidth - 1) & ~(encodingWidth - 1)) + encodingWidth;
        auto str = decoder.decode(QByteArrayView{ buffer.constData(), pos });
        line.append(str);
        buffer = buffer.mid(str.data.size());

        if (line.endsWith('\n'))
        {
            while (!line.isEmpty() && (line.endsWith('\n') || line.endsWith('\r')))
                line.chop(1);
            return true;
        }

        pos = buffer.indexOf('\n', pos);
    }
    return false;
}

bool Log::isCommentEnd(const Format::Comment& comment, const QString& line) const
{
    return comment.finish && line.endsWith(comment.finish.value());
}
