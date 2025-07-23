#include "LogService.h"

#include "Utils.h"
#include "Application.h"
#include "SearchController.h"


LogService::LogService(QObject *parent) :
    QObject{parent},
    iteratorRequests(ThreadSafePtr<std::deque<IteratorRequest>>::DefaultConstructor{}),
    iterators(ThreadSafePtr<std::map<int, std::shared_ptr<LogEntryIterator<>>>>::DefaultConstructor{}),
    reverseIteratorRequests(ThreadSafePtr<std::deque<ReverseIteratorRequest>>::DefaultConstructor{}),
    reverseIterators(ThreadSafePtr<std::map<int, std::shared_ptr<LogEntryIterator<false>>>>::DefaultConstructor{}),
    dataRequests(ThreadSafePtr<std::deque<DataRequest>>::DefaultConstructor{}),
    dataRequestResults(ThreadSafePtr<std::map<int, std::vector<LogEntry>>>::DefaultConstructor{})
{}

const ThreadSafePtr<LogManager>& LogService::getLogManager() const
{
    return logManager;
}

void LogService::createSession(const std::unordered_set<QString>& modules, const std::chrono::system_clock::time_point& minTime, const std::chrono::system_clock::time_point& maxTime)
{
    session = logManager->createSession(modules, minTime, maxTime);
}

const ThreadSafePtr<Session>& LogService::getSession() const
{
    return session;
}

int LogService::requestIterator(const std::chrono::system_clock::time_point& startTime,
                                const std::chrono::system_clock::time_point& endTime)
{
    if (!session || session->getMaxTime() < startTime)
    {
        qCritical() << "Invalid iterator request parameters.";
        return -1;
    }

    int index = nextRequestIndex++;
    iteratorRequests->emplace_back(index, startTime, endTime);
    QMetaObject::invokeMethod(this, "handleIteratorRequest", Qt::QueuedConnection);

    return index;
}

std::shared_ptr<LogEntryIterator<>> LogService::getIterator(int index)
{
    auto lockedIteratorList = iterators.getLocker();

    auto it = lockedIteratorList->find(index);
    if (it != lockedIteratorList->end())
    {
        auto iterator = it->second;
        lockedIteratorList->erase(it);
        return iterator;
    }
    qCritical() << "Iterator request with index" << index << "not found.";
    return nullptr;
}

int LogService::requestReverseIterator(const std::chrono::system_clock::time_point& startTime, const std::chrono::system_clock::time_point& endTime)
{
    if (!logManager || logManager->getMinTime() > endTime)
    {
        qCritical() << "Invalid reverse iterator request parameters.";
        throw std::runtime_error("Invalid reverse iterator request parameters.");
    }

    int index = nextRequestIndex++;
    reverseIteratorRequests->emplace_back(index, startTime, endTime);
    QMetaObject::invokeMethod(this, "handleReverseIteratorRequest", Qt::QueuedConnection);

    return index;
}

std::shared_ptr<LogEntryIterator<false>> LogService::getReverseIterator(int index)
{
    auto lockedIteratorList = reverseIterators.getLocker();

    auto it = lockedIteratorList->find(index);
    if (it != lockedIteratorList->end())
    {
        auto iterator = it->second;
        lockedIteratorList->erase(it);
        return iterator;
    }
    qCritical() << "Iterator request with index" << index << "not found.";
    return nullptr;
}

std::vector<LogEntry> LogService::getResult(int index)
{
    auto lockedDataRequestResults = dataRequestResults.getLocker();

    auto it = lockedDataRequestResults->find(index);
    if (it != lockedDataRequestResults->end())
    {
        auto res = std::move(it->second);
        lockedDataRequestResults->erase(it);
        return res;
    }
    return {};
}

void LogService::openFile(const QString& file, const QStringList& formats)
{
    QT_SLOT_BEGIN

    emit progressUpdated(QStringLiteral("Opening file %1 ...").arg(file), 0);

    auto newLogManager = std::make_shared<LogManager>(file, getFormats(formats));
    logManager = newLogManager;
    emit logManagerCreated();

    emit progressUpdated(QStringLiteral("File %1 opened").arg(file), 100);

    QT_SLOT_END
}

void LogService::openFolder(const QString& logDirectory, const QStringList& formats)
{
    QT_SLOT_BEGIN

    emit progressUpdated(QStringLiteral("Opening folder %1 ...").arg(logDirectory), 0);

    auto newLogManager = std::make_shared<LogManager>(std::vector<QString>{logDirectory}, getFormats(formats));
    logManager = newLogManager;
    emit logManagerCreated();

    emit progressUpdated(QStringLiteral("Folder %1 opened").arg(logDirectory), 100);

    QT_SLOT_END
}

void LogService::search(const QDateTime& time, const QString& searchTerm, bool lastColumn, bool regexEnabled, bool backward)
{
    QT_SLOT_BEGIN

    emit progressUpdated(QStringLiteral("Searching for '%1' ...").arg(searchTerm), 0);

    if (!session)
    {
        qCritical() << "Session is not initialized.";
        return;
    }

    if (searchTerm.isEmpty())
    {
        qWarning() << "Search term is empty.";
        return;
    }

    auto startTime = time.toStdSysMilliseconds();
    auto endTime = session->getMaxTime();
    auto totalMs = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
    auto iterator = LogEntryIterator<>(session->getIterator<true>(startTime, endTime));
    int lastPercent = 0;
    while(iterator.hasLogs())
    {
        auto entry = iterator.next();
        if (!entry)
            break;

        auto curMs = std::chrono::duration_cast<std::chrono::milliseconds>(entry->time - startTime).count();
        int percent = totalMs ? static_cast<int>(100LL * curMs / totalMs) : 0;
        if (percent != lastPercent && percent <= 100) {
            emit progressUpdated(QStringLiteral("Searching for '%1' ...").arg(searchTerm), percent);
            lastPercent = percent;
        }

        if (SearchController::checkEntry(entry->line, searchTerm, lastColumn, regexEnabled))
        {
            emit searchFinished(searchTerm, DateTimeFromChronoSystemClock(entry->time));
            emit progressUpdated(QStringLiteral("Search finished"), 100);
            return;
        }
    }

    emit progressUpdated(QStringLiteral("Search finished"), 100);

    QT_SLOT_END
}

void LogService::searchWithFilter(const QDateTime& time, const QString& searchTerm, bool lastColumn, bool regexEnabled, bool backward, const LogFilter& filter)
{
    QT_SLOT_BEGIN

    emit progressUpdated(QStringLiteral("Searching for '%1' ...").arg(searchTerm), 0);

    if (!session)
    {
        qCritical() << "Session is not initialized.";
        return;
    }

    if (searchTerm.isEmpty())
    {
        qWarning() << "Search term is empty.";
        return;
    }

    auto startTimeF = time.toStdSysMilliseconds();
    auto endTimeF = session->getMaxTime();
    auto totalMsF = std::chrono::duration_cast<std::chrono::milliseconds>(endTimeF - startTimeF).count();
    auto iterator = LogEntryIterator<>(session->getIterator<true>(startTimeF, endTimeF));
    int lastPercentF = 0;
    while(iterator.hasLogs())
    {
        auto entry = iterator.next();
        if (!entry)
            break;

        auto curMsF = std::chrono::duration_cast<std::chrono::milliseconds>(entry->time - startTimeF).count();
        int percentF = totalMsF ? static_cast<int>(100LL * curMsF / totalMsF) : 0;
        if (percentF != lastPercentF && percentF <= 100) {
            emit progressUpdated(QStringLiteral("Searching for '%1' ...").arg(searchTerm), percentF);
            lastPercentF = percentF;
        }

        if (SearchController::checkEntry(entry->line, searchTerm, lastColumn, regexEnabled) && filter.check(entry.value()))
        {
            emit searchFinished(searchTerm, DateTimeFromChronoSystemClock(entry->time));
            emit progressUpdated(QStringLiteral("Search finished"), 100);
            return;
        }
    }

    emit progressUpdated(QStringLiteral("Search finished"), 100);

    QT_SLOT_END
}

void LogService::exportData(const QString& filename, const QDateTime& startTime, const QDateTime& endTime)
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

void LogService::exportData(const QString& filename, const QDateTime& startTime, const QDateTime& endTime, const QStringList& fields)
{
    QT_SLOT_BEGIN

    emit progressUpdated(QStringLiteral("Exporting data to %1 ...").arg(filename), 0);

    exportDataToFile(filename, startTime, endTime, [&fields](QFile& file, const LogEntry& entry) {
        for (const auto& field : fields)
        {
            auto value = entry.values.find(field);
            if (value != entry.values.end())
                file.write(value->second.toString().toUtf8());
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

void LogService::exportData(const QString& filename, const QDateTime& startTime, const QDateTime& endTime, const QStringList& fields, const LogFilter& filter)
{
    QT_SLOT_BEGIN

    emit progressUpdated(QStringLiteral("Exporting data to %1 ...").arg(filename), 0);

    exportDataToFile(filename, startTime, endTime, [&fields, &filter](QFile& file, const LogEntry& entry) {
        if (!filter.check(entry))
            return;

        for (const auto& field : fields)
        {
            auto value = entry.values.find(field);
            if (value != entry.values.end())
                file.write(value->second.toString().toUtf8());
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

void LogService::exportData(const QString& filename, QTreeView* view)
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

void LogService::handleIteratorRequest()
{
    QT_SLOT_BEGIN

    emit progressUpdated(QStringLiteral("Creating iterator ..."), 0);

    if (!session)
    {
        qCritical() << "Session is not initialized.";
        return;
    }

    auto lockedIteratorRequests = iteratorRequests.getLocker();

    auto it = lockedIteratorRequests->begin();
    if (it == lockedIteratorRequests->end())
    {
        qWarning() << "No iterator requests available.";
        return;
    }

    auto& request = *it;
    lockedIteratorRequests.unlock();

    iterators->emplace(request.index, std::make_shared<LogEntryIterator<>>(session->getIterator<true>(request.startTime, request.endTime)));

    iteratorCreated(request.index, true);

    lockedIteratorRequests.lock();
    iteratorRequests->pop_front();

    emit progressUpdated(QStringLiteral("Iterator created"), 100);

    QT_SLOT_END
}

void LogService::handleReverseIteratorRequest()
{
    QT_SLOT_BEGIN

    emit progressUpdated(QStringLiteral("Creating reverse iterator ..."), 0);

    auto lockedIteratorRequests = reverseIteratorRequests.getLocker();

    auto it = lockedIteratorRequests->begin();
    if (it == lockedIteratorRequests->end())
    {
        qWarning() << "No iterator requests available.";
        return;
    }

    auto& request = *it;
    lockedIteratorRequests.unlock();

    reverseIterators->emplace(request.index, std::make_shared<LogEntryIterator<false>>(session->getIterator<false>(request.startTime, request.endTime)));

    iteratorCreated(request.index, false);

    lockedIteratorRequests.lock();
    reverseIteratorRequests->pop_front();

    emit progressUpdated(QStringLiteral("Reverse iterator created"), 100);

    QT_SLOT_END
}

void LogService::handleDataRequest()
{
    QT_SLOT_BEGIN

    emit progressUpdated(QStringLiteral("Loading data ..."), 0);

    static auto dataRequestVisitor = [](const auto& iterator) -> std::optional<LogEntry> {
        return iterator->next();
    };

    auto lockedDataRequests = dataRequests.getLocker();

    while (lockedDataRequests->begin() != lockedDataRequests->end() && !lockedDataRequests->begin()->active)
        lockedDataRequests->pop_front();

    if (lockedDataRequests->begin() == lockedDataRequests->end())
    {
        qWarning() << "No active data requests available.";
        return;
    }

    auto& request = *lockedDataRequests->begin();

    lockedDataRequests.unlock();

    std::vector<LogEntry>& result = dataRequestResults->emplace(request.index, std::vector<LogEntry>{}).first->second;
    result.clear();

    result.reserve(request.entriesCount);

    auto limitVisitor = [&until = request.until](const auto& iterator) -> bool {
        return iterator->isValueAhead(until.value());
    };

    int lastPercentR = 0;
    while ((request.until ? std::visit(limitVisitor, request.iterator) : true) &&
           result.size() < request.entriesCount)
    {
        auto entry = std::visit(dataRequestVisitor, request.iterator);
        if (!entry)
            break;

        result.emplace_back(std::move(entry.value()));

        int percent = request.entriesCount ? static_cast<int>(100LL * result.size() / request.entriesCount) : 0;
        if (percent != lastPercentR)
        {
            emit progressUpdated(QStringLiteral("Loading data ..."), percent);
            lastPercentR = percent;
        }
    }

    dataLoaded(request.index);

    lockedDataRequests.lock();
    lockedDataRequests->pop_front();

    emit progressUpdated(QStringLiteral("Data loaded"), 100);

    QT_SLOT_END
}

std::vector<std::shared_ptr<Format>> LogService::getFormats(const QStringList& formats)
{
    auto formatList = static_cast<Application*>(qApp)->getFormatManager().getFormats();
    std::vector<std::shared_ptr<Format>> res;
    for (const auto& formatName : formats)
        res.push_back(formatList.at(formatName.toStdString()));
    return res;
}

void LogService::exportDataToFile(const QString& filename, const QDateTime& startTime, const QDateTime& endTime, const std::function<void (QFile&, const LogEntry&)>& writeFunction, const std::function<void (QFile& file)>& prefix)
{
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

    auto startMs = startTime.toStdSysMilliseconds();
    auto endMs = endTime.toStdSysMilliseconds();
    auto totalMs = std::chrono::duration_cast<std::chrono::milliseconds>(endMs - startMs).count();

    auto iterator = session->getIterator(startMs, endMs);
    int lastPercent = 0;
    while (iterator.hasLogs())
    {
        auto entry = iterator.next();
        if (!entry)
            break;

        writeFunction(file, entry.value());

        auto curMs = std::chrono::duration_cast<std::chrono::milliseconds>(entry->time - startMs).count();
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

void LogService::exportDataToFile(const QString& filename, const QDateTime& startTime, const QDateTime& endTime, const std::function<void (QFile&, const LogEntry&)>& writeFunction, const QStringList& fields)
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

LogService::IteratorRequest::IteratorRequest(int index, const std::chrono::system_clock::time_point& start, const std::chrono::system_clock::time_point& end) :
    index(index),
    startTime(start),
    endTime(end)
{}

LogService::ReverseIteratorRequest::ReverseIteratorRequest(int index, const std::chrono::system_clock::time_point& start, const std::chrono::system_clock::time_point& end) :
    index(index),
    startTime(start),
    endTime(end)
{}
