#include "ExportService.h"
#include "SessionService.h"
#include "Utils.h"
#include "LogView/LogModel.h"


ExportService::ExportService(SessionService* sessionService, QObject* parent) :
    QObject(parent),
    sessionService(sessionService)
{}

void ExportService::exportData(const QString& filename, const std::chrono::system_clock::time_point& startTime, const std::chrono::system_clock::time_point& endTime)
{
    QT_SLOT_BEGIN

    emit progressUpdated(QStringLiteral("Exporting data to %1 ...").arg(filename), 0);

    exportDataToFile(filename, startTime, endTime, [](QFile& file, const LogEntry& entry) {
        file.write(entry.line.toUtf8());
        file.write("\n");
    });

    emit progressUpdated(QStringLiteral("Export finished"), 100);

    QT_SLOT_END
}

void ExportService::exportData(const QString& filename, const std::chrono::system_clock::time_point& startTime, const std::chrono::system_clock::time_point& endTime, const QStringList& fields, const LogFilter& filter)
{
    QT_SLOT_BEGIN

    emit progressUpdated(QStringLiteral("Exporting data to %1 ...").arg(filename), 0);

    exportDataToFile(filename, startTime, endTime, [&fields, &filter](QFile& file, const LogEntry& entry) {
        if (!filter.check(entry))
            return;

        file.write(entry.line.toUtf8());
        file.write("\n");
    });

    emit progressUpdated(QStringLiteral("Export finished"), 100);

    QT_SLOT_END
}

void ExportService::exportDataToTable(const QString& filename, const std::chrono::system_clock::time_point& startTime, const std::chrono::system_clock::time_point& endTime, const QStringList& fields)
{
    QT_SLOT_BEGIN

    emit progressUpdated(QStringLiteral("Exporting data to %1 ...").arg(filename), 0);

    exportDataToFile(filename, startTime, endTime, [&fields](QFile& file, const LogEntry& entry) {
        for (int i = 0; i < fields.size(); ++i)
        {
            const auto& field = fields[i];
            auto value = entry.values.find(field);
            if (value != entry.values.end())
                file.write(value->second.toString().toUtf8());
            else if (i == static_cast<int>(LogModel::PredefinedColumn::Module))
                file.write(entry.module.toUtf8());
            file.write(";");
        }

        if (!entry.additionalLines.isEmpty())
        {
            file.write("\n");
            file.write(entry.additionalLines.toUtf8());
        }

        file.write("\n");
    }, fields);

    emit progressUpdated(QStringLiteral("Export finished"), 100);

    QT_SLOT_END
}

void ExportService::exportDataToTable(const QString& filename, const std::chrono::system_clock::time_point& startTime, const std::chrono::system_clock::time_point& endTime, const QStringList& fields, const LogFilter& filter)
{
    QT_SLOT_BEGIN

    emit progressUpdated(QStringLiteral("Exporting data to %1 ...").arg(filename), 0);

    exportDataToFile(filename, startTime, endTime, [&fields, &filter](QFile& file, const LogEntry& entry) {
        if (!filter.check(entry))
            return;

        for (int i = 0; i < fields.size(); ++i)
        {
            const auto& field = fields[i];
            auto value = entry.values.find(field);
            if (value != entry.values.end())
                file.write(value->second.toString().toUtf8());
            else if (i == static_cast<int>(LogModel::PredefinedColumn::Module))
                file.write(entry.module.toUtf8());
            file.write(";");
        }

        if (!entry.additionalLines.isEmpty())
        {
            file.write("\n");
            file.write(entry.additionalLines.toUtf8());
        }

        file.write("\n");
    }, fields);

    emit progressUpdated(QStringLiteral("Export finished"), 100);

    QT_SLOT_END
}

void ExportService::exportData(const QString& filename, QTreeView* view)
{
    QT_SLOT_BEGIN

    emit progressUpdated(QStringLiteral("Exporting data to %1 ...").arg(filename), 0);

    if (!view)
    {
        qCritical() << "Invalid view provided for export.";
        return;
    }

    auto model = qobject_cast<QAbstractItemModel*>(view->model());
    if (!model)
    {
        qCritical() << "Invalid model in the provided view.";
        return;
    }

    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        qCritical() << "Failed to open file for writing:" << file.errorString();
        return;
    }

    for (int col = 0; col < model->columnCount(); ++col)
    {
        if (view->isColumnHidden(col))
            continue;

        QVariant data = model->headerData(col, Qt::Horizontal, Qt::DisplayRole);
        if (data.isValid())
            file.write(data.toString().toUtf8() + ";");
    }

    file.write("\n");

    int totalRows = model->rowCount();
    int lastPercent = 0;
    for (int row = 0; row < totalRows; ++row)
    {
        QStringList lineData;
        for (int col = 0; col < model->columnCount(); ++col)
        {
            if (view->isColumnHidden(col))
                continue;

            QModelIndex index = model->index(row, col);
            if (index.isValid())
            {
                QVariant data = model->data(index, Qt::DisplayRole);
                lineData.append(data.toString());
            }
        }
        file.write(lineData.join(";").toUtf8() + "\n");

        int percent = totalRows ? static_cast<int>(100LL * (row + 1) / totalRows) : 0;
        if (percent != lastPercent)
        {
            emit progressUpdated(QStringLiteral("Exporting data to %1 ...").arg(filename), percent);
            lastPercent = percent;
        }
    }

    emit progressUpdated(QStringLiteral("Export finished"), 100);

    QT_SLOT_END
}

void ExportService::exportDataToFile(const QString& filename, const std::chrono::system_clock::time_point& startTime, const std::chrono::system_clock::time_point& endTime,
                                     const std::function<void (QFile&, const LogEntry&)>& writeFunction,
                                     const std::function<void (QFile& file)>& prefix)
{
    auto session = sessionService->getSession();
    if (!session)
    {
        qCritical() << "Session is not initialized.";
        return;
    }

    if (startTime > endTime)
    {
        qCritical() << "Invalid time range for export.";
        return;
    }

    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        qCritical() << "Failed to open file for writing:" << file.errorString();
        return;
    }

    if (prefix)
        prefix(file);

    auto totalMs = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();

    auto iterator = session->getIterator(startTime, endTime);
    int lastPercent = 0;
    while (iterator.hasLogs())
    {
        auto entry = iterator.next();
        if (!entry)
            break;

        writeFunction(file, entry.value());

        auto curMs = std::chrono::duration_cast<std::chrono::milliseconds>(entry->time - startTime).count();
        int percent = totalMs ? static_cast<int>(100LL * curMs / totalMs) : 0;
        if (percent != lastPercent && percent <= 100)
        {
            emit progressUpdated(QStringLiteral("Exporting data to %1 ...").arg(filename), percent);
            lastPercent = percent;
        }
    }

    if (lastPercent < 100)
        emit progressUpdated(QStringLiteral("Exporting data to %1 ...").arg(filename), 100);
}

void ExportService::exportDataToFile(const QString& filename, const std::chrono::system_clock::time_point& startTime, const std::chrono::system_clock::time_point& endTime,
                                     const std::function<void (QFile&, const LogEntry&)>& writeFunction, const QStringList& fields)
{
    exportDataToFile(filename, startTime, endTime, writeFunction,
                     [&fields](QFile& file) {
                         for (const auto& field : fields)
                         {
                             file.write(field.toUtf8());
                             file.write(";");
                         }
                         file.write("\n");
                     });
}
