#include "Log.h"


Log::Log(std::unique_ptr<QIODevice>&& _file, const std::optional<QStringConverter::Encoding>& _encoding, const std::shared_ptr<std::vector<Format::Comment>>& _comments) :
    file(std::move(_file)),
    comments(_comments)
{
    if (!file->isOpen())
    {
        if (!file->open(QIODevice::ReadOnly | QIODevice::Text))
            throw std::runtime_error("cannot open log file: " + file->errorString().toStdString());
    }

    QStringConverter::Encoding encoding = checkFileForBom();
    if (_encoding)
        encoding = _encoding.value();
    decoder = QStringDecoder(encoding);
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
    while (file->pos() != 0)
    {
        QByteArray lineData;

        char ch;
        int prevPos = file->pos();
        while (prevPos > 0)
        {
            prevPos--;
            file->seek(prevPos);
            if (!file->getChar(&ch))
                return std::nullopt;

            if (ch != '\n' && ch != '\r')
            {
                lineData.prepend(ch);
                break;
            }
        }

        while (prevPos > 0)
        {
            file->seek(prevPos - 1);
            if (!file->getChar(&ch))
                return std::nullopt;
            if (ch == '\n' || ch == '\r')
                break;
            prevPos--;
            lineData.prepend(ch);
        }
        file->seek(prevPos);
        line = decoder(QByteArrayView(lineData));

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

        qDebug() << "Found line:" << line;
        return line;
    }
    return std::nullopt;
}

std::optional<QString> Log::nextLine()
{
    QString line;
    std::optional<Format::Comment> comment;
    while (!buffer.isEmpty() || !file->atEnd())
    {
        qsizetype pos = buffer.indexOf('\n');
        while(pos == -1 && !file->atEnd())
        {
            buffer.append(decoder(file->read(512)));
            pos = buffer.indexOf('\n');
        }

        line = buffer.left(pos);
        buffer.remove(0, line.size() + 1);

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
    return static_cast<int>(file->pos()) - buffer.size();
}

QStringConverter::Encoding Log::checkFileForBom()
{
    QStringConverter::Encoding encoding;

    QByteArray head = file->read(4);
    file->seek(0);

    if (head.startsWith("\xEF\xBB\xBF"))
    {
        encoding = QStringConverter::Utf8;
        file->seek(3);
    }
    else if (head.startsWith("\xFF\xFE"))
    {
        encoding = QStringDecoder::Utf16LE;
        file->seek(2);
    }
    else if (head.startsWith("\xFE\xFF"))
    {
        encoding = QStringDecoder::Utf16BE;
        file->seek(2);
    }
    else if (head.startsWith("\xFF\xFE\x00\x00"))
    {
        encoding = QStringDecoder::Utf32LE;
    }
    else if (head.startsWith("\x00\x00\xFE\xFF"))
    {
        encoding = QStringDecoder::Utf32BE;
    }
    else
    {
        encoding = QStringDecoder::Utf8;
    }

    return encoding;
}

bool Log::isCommentEnd(const Format::Comment& comment, const QString& line) const
{
    return comment.finish && line.endsWith(comment.finish.value());
}
