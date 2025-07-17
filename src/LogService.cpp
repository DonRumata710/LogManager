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
    if (!logManager || logManager->getMaxTime() < startTime)
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

    auto newLogManager = std::make_shared<LogManager>(file, getFormats(formats));
    logManager = newLogManager;
    emit logManagerCreated();

    QT_SLOT_END
}

void LogService::openFolder(const QString& logDirectory, const QStringList& formats)
{
    QT_SLOT_BEGIN

    auto newLogManager = std::make_shared<LogManager>(std::vector<QString>{logDirectory}, getFormats(formats));
    logManager = newLogManager;
    emit logManagerCreated();

    QT_SLOT_END
}

void LogService::search(const QString& searchTerm, bool lastColumn, bool regexEnabled, bool backward)
{
    QT_SLOT_BEGIN

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

    auto iterator = LogEntryIterator<>(session->getIterator<true>(session->getMinTime(), session->getMaxTime()));
    while(iterator.hasLogs())
    {
        auto entry = iterator.next();
        if (!entry)
            break;

        if (SearchController::checkEntry(entry->line, searchTerm, lastColumn, regexEnabled))
        {
            emit searchFinished(searchTerm, DateTimeFromChronoSystemClock(entry->time));
            return;
        }
    }

    QT_SLOT_END
}

void LogService::exportData(const QString& filename, const QDateTime& startTime, const QDateTime& endTime)
{
    QT_SLOT_BEGIN

    exportDataToFile(filename, startTime, endTime, [](QFile& file, const LogEntry& entry) {
        file.write(entry.line.toUtf8());
        file.write("\n");
    });

    QT_SLOT_END
}

void LogService::exportData(const QString& filename, const QDateTime& startTime, const QDateTime& endTime, const QStringList& fields)
{
    QT_SLOT_BEGIN

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
    });

    QT_SLOT_END
}

void LogService::exportData(const QString& filename, const QDateTime& startTime, const QDateTime& endTime, const QStringList& fields, const LogFilter& filter)
{
    QT_SLOT_BEGIN

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
    });

    QT_SLOT_END
}

void LogService::exportData(const QString& filename, QTreeView* view)
{
    QT_SLOT_BEGIN

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
        file.write("\n");
    }

    for (int row = 0; row < model->rowCount(); ++row)
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
    }

    QT_SLOT_END
}

void LogService::handleIteratorRequest()
{
    QT_SLOT_BEGIN

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

    QT_SLOT_END
}

void LogService::handleReverseIteratorRequest()
{
    QT_SLOT_BEGIN

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

    QT_SLOT_END
}

void LogService::handleDataRequest()
{
    QT_SLOT_BEGIN

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

    while ((request.until ? std::visit(limitVisitor, request.iterator) : true) &&
           result.size() < request.entriesCount)
    {
        auto entry = std::visit(dataRequestVisitor, request.iterator);
        if (!entry)
            break;

        result.emplace_back(std::move(entry.value()));
    }

    dataLoaded(request.index);

    lockedDataRequests.lock();
    lockedDataRequests->pop_front();

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

void LogService::exportDataToFile(const QString& filename, const QDateTime& startTime, const QDateTime& endTime, const std::function<void (QFile&, const LogEntry&)>& writeFunction)
{
    if (!logManager)
    {
        qCritical() << "LogManager is not initialized.";
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

    auto iterator = session->getIterator(startTime.toStdSysMilliseconds(), endTime.toStdSysMilliseconds());
    while (iterator.hasLogs())
    {
        auto entry = iterator.next();
        if (!entry)
            break;

        writeFunction(file, entry.value());
    }
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
