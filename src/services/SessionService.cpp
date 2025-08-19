#include "SessionService.h"
#include "Application.h"
#include "Utils.h"

#include <QMetaType>

SessionService::SessionService(QObject* parent)
    : QObject(parent),
      iterators(ThreadSafePtr<std::map<int, std::shared_ptr<LogEntryIterator<>>>>::DefaultConstructor{}),
      reverseIterators(ThreadSafePtr<std::map<int, std::shared_ptr<LogEntryIterator<false>>>>::DefaultConstructor{}),
      dataRequestResults(ThreadSafePtr<std::map<int, std::vector<LogEntry>>>::DefaultConstructor{})
{
    qRegisterMetaType<SessionService::DataRequest>("SessionService::DataRequest");
    connect(this, &SessionService::iteratorRequested, this, &SessionService::handleIteratorRequest, Qt::QueuedConnection);
    connect(this, &SessionService::reverseIteratorRequested, this, &SessionService::handleReverseIteratorRequest, Qt::QueuedConnection);
    connect(this, &SessionService::logEntriesRequested, this, &SessionService::handleDataRequest, Qt::QueuedConnection);
}

const ThreadSafePtr<LogManager>& SessionService::getLogManager() const
{
    return logManager;
}

const ThreadSafePtr<Session>& SessionService::getSession() const
{
    return session;
}

std::unordered_set<QVariant, VariantHash> SessionService::getEnumList(const QString& field) const
{
    auto sessionPtr = getSession();
    if (!sessionPtr)
        return {};
    auto sessionLocker = sessionPtr.getLocker();
    return sessionLocker->getEnumList(field);
}

void SessionService::createSession(const std::unordered_set<QString>& modules,
                                   const std::chrono::system_clock::time_point& minTime,
                                   const std::chrono::system_clock::time_point& maxTime)
{
    if (!logManager)
        throw std::runtime_error("LogManager is not initialized.");

    session = logManager->createSession(modules, minTime, maxTime);
}

void SessionService::openFile(const QString& file, const QStringList& formats)
{
    QT_SLOT_BEGIN

    emit progressUpdated(QStringLiteral("Opening file %1 ...").arg(file), 0);

    auto newLogManager = std::make_shared<LogManager>(file, getFormats(formats));
    logManager = newLogManager;
    emit logManagerCreated(file);

    emit progressUpdated(QStringLiteral("File %1 opened").arg(file), 100);

    QT_SLOT_END
}

void SessionService::openBuffer(const QByteArray& data, const QString& filename, const QStringList& formats)
{
    QT_SLOT_BEGIN

    emit progressUpdated(QStringLiteral("Opening buffer %1 ...").arg(filename), 0);

    auto newLogManager = std::make_shared<LogManager>(data, filename, getFormats(formats));
    logManager = newLogManager;
    emit logManagerCreated(filename);

    emit progressUpdated(QStringLiteral("Buffer %1 opened").arg(filename), 100);

    QT_SLOT_END
}

void SessionService::openFolder(const QString& logDirectory, const QStringList& formats)
{
    QT_SLOT_BEGIN

    emit progressUpdated(QStringLiteral("Opening folder %1 ...").arg(logDirectory), 0);

    auto newLogManager = std::make_shared<LogManager>(std::vector<QString>{logDirectory}, getFormats(formats));
    logManager = newLogManager;
    emit logManagerCreated(logDirectory);

    emit progressUpdated(QStringLiteral("Folder %1 opened").arg(logDirectory), 100);

    QT_SLOT_END
}

int SessionService::requestIterator(const std::chrono::system_clock::time_point& startTime,
                                    const std::chrono::system_clock::time_point& endTime)
{
    if (!session || session->getMaxTime() < startTime)
    {
        qCritical() << "Invalid iterator request parameters.";
        return -1;
    }

    int index = nextRequestIndex++;
    emit iteratorRequested(index, startTime, endTime);
    return index;
}

std::shared_ptr<LogEntryIterator<>> SessionService::getIterator(int index)
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

int SessionService::requestReverseIterator(const std::chrono::system_clock::time_point& startTime,
                                           const std::chrono::system_clock::time_point& endTime)
{
    if (!logManager || logManager->getMinTime() > endTime)
    {
        qCritical() << "Invalid reverse iterator request parameters.";
        throw std::runtime_error("Invalid reverse iterator request parameters.");
    }

    int index = nextRequestIndex++;
    emit reverseIteratorRequested(index, startTime, endTime);
    return index;
}

std::shared_ptr<LogEntryIterator<false>> SessionService::getReverseIterator(int index)
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

std::vector<LogEntry> SessionService::getResult(int index)
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

void SessionService::handleIteratorRequest(int index,
                                           const std::chrono::system_clock::time_point& startTime,
                                           const std::chrono::system_clock::time_point& endTime)
{
    QT_SLOT_BEGIN

    emit progressUpdated(QStringLiteral("Creating iterator ..."), 0);

    if (!session)
    {
        qCritical() << "Session is not initialized.";
        return;
    }

    iterators->emplace(index, std::make_shared<LogEntryIterator<>>(session->getIterator<true>(startTime, endTime)));
    iteratorCreated(index, true);

    emit progressUpdated(QStringLiteral("Iterator created"), 100);

    QT_SLOT_END
}

void SessionService::handleReverseIteratorRequest(int index,
                                                  const std::chrono::system_clock::time_point& startTime,
                                                  const std::chrono::system_clock::time_point& endTime)
{
    QT_SLOT_BEGIN

    emit progressUpdated(QStringLiteral("Creating reverse iterator ..."), 0);

    reverseIterators->emplace(index, std::make_shared<LogEntryIterator<false>>(session->getIterator<false>(startTime, endTime)));
    iteratorCreated(index, false);

    emit progressUpdated(QStringLiteral("Reverse iterator created"), 100);

    QT_SLOT_END
}

void SessionService::handleDataRequest(const DataRequest& request)
{
    QT_SLOT_BEGIN

    emit progressUpdated(QStringLiteral("Loading data ..."), 0);

    static auto dataRequestVisitor = [](const auto& iterator) -> std::optional<LogEntry> {
        return iterator->next();
    };

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
    emit progressUpdated(QStringLiteral("Data loaded"), 100);

    QT_SLOT_END
}

std::vector<std::shared_ptr<Format>> SessionService::getFormats(const QStringList& formats)
{
    auto formatList = static_cast<Application*>(qApp)->getFormatManager().getFormats();
    std::vector<std::shared_ptr<Format>> res;
    for (const auto& formatName : formats)
        res.push_back(formatList.at(formatName.toStdString()));
    return res;
}
