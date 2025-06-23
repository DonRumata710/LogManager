#include "LogService.h"

#include "Utils.h"
#include "Application.h"


LogService::LogService(QObject *parent) :
    QObject{parent},
    iteratorRequests(ThreadSafePtr<std::deque<IteratorRequest>>::DefaultConstructor{}),
    iterators(ThreadSafePtr<std::map<int, std::shared_ptr<LogEntryIterator>>>::DefaultConstructor{}),
    dataRequests(ThreadSafePtr<std::deque<DataRequest>>::DefaultConstructor{}),
    dataRequestResults(ThreadSafePtr<std::map<int, std::vector<LogEntry>>>::DefaultConstructor{})
{}

const std::shared_ptr<LogManager>& LogService::getLogManager() const
{
    return logManager;
}

int LogService::requestIterator(const std::chrono::system_clock::time_point& startTime)
{
    if (!logManager || logManager->getMaxTime() < startTime)
    {
        qCritical() << "Invalid iterator request parameters.";
        return -1;
    }

    int index = nextRequestIndex++;
    iteratorRequests->emplace_back(index, startTime);
    QMetaObject::invokeMethod(this, "handleIteratorRequest", Qt::QueuedConnection);

    return index;
}

std::shared_ptr<LogEntryIterator> LogService::getIterator(int index)
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

int LogService::requestLogEntries(const std::shared_ptr<LogEntryIterator>& iterator, int entryCount)
{
    if (!logManager || !iterator || entryCount <= 0 || !iterator->hasLogs())
    {
        qCritical() << "Invalid log entry request parameters.";
        return -1;
    }

    int index = nextRequestIndex++;
    dataRequests->emplace_back(index, iterator, entryCount);
    QMetaObject::invokeMethod(this, "handleDataRequest", Qt::QueuedConnection);

    return index;
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

void LogService::handleIteratorRequest()
{
    QT_SLOT_BEGIN

    auto lockedIteratorRequests = iteratorRequests.getLocker();

    auto it = lockedIteratorRequests->begin();
    if (it == lockedIteratorRequests->end())
    {
        qWarning() << "No iterator requests available.";
        return;
    }

    auto& request = *it;
    lockedIteratorRequests.unlock();

    iterators->emplace(request.index, std::make_shared<LogEntryIterator>(logManager->getIterator(request.startTime)));

    iteratorCreated(request.index);

    lockedIteratorRequests.lock();
    iteratorRequests->pop_front();

    QT_SLOT_END
}

void LogService::handleDataRequest()
{
    QT_SLOT_BEGIN

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

    result.reserve(request.entryCount);
    while (result.size() < request.entryCount)
    {
        auto entry = request.iterator->next();
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

LogService::IteratorRequest::IteratorRequest(int index, const std::chrono::system_clock::time_point& time) : index(index), startTime(time)
{}

LogService::DataRequest::DataRequest(int index, std::shared_ptr<LogEntryIterator> it, int count) : index(index), iterator(std::move(it)), entryCount(count)
{}
