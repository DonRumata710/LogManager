#include "LogModel.h"

#include "LogViewUtils.h"
#include "Settings.h"
#include "Utils.h"
#include "ScopeGuard.h"
#include "../LogManagement/FilteredLogIterator.h"

#include <QFontDatabase>


LogModel::LogModel(LogService* logService, QObject *parent) :
    QAbstractItemModel(parent),
    service(logService),
    startTime(DateTimeFromChronoSystemClock(logService->getSession()->getMinTime())),
    endTime(DateTimeFromChronoSystemClock(logService->getSession()->getMaxTime())),
    modules(logService->getSession()->getModules()),
    blockSize(loadBlockSize()),
    blockCount(loadBlockCount())
{
    connect(service, &LogService::iteratorCreated, this, &LogModel::handleIterator);
    connect(service, &LogService::dataLoaded, this, &LogModel::handleData);

    bool flag = false;
    for (const auto& format : logService->getSession()->getFormats())
    {
        QString timeFormatName = format->fields.at(format->timeFieldIndex).name;

        for (const auto& field : format->fields)
        {
            if (field.name == timeFormatName)
            {
                if (!flag)
                {
                    fields.insert(fields.begin(), field);
                    flag = true;
                }
                continue;
            }

            auto it = std::find_if(fields.begin(), fields.end(), [&field](const Format::Field& f) { return f.name == field.name; });
            if (it != fields.end())
                continue;
            fields.push_back(field);
        }
    }
}

void LogModel::goToTime(const QDateTime& time)
{
    goToTime(time.toStdSysMilliseconds());
}

void LogModel::goToTime(const std::chrono::system_clock::time_point& time)
{
    if (!logs.empty() &&
        ((time >= logs.begin()->entry.time && time <= logs.back().entry.time) ||
         (time < getStartTime().toStdSysMilliseconds() && reverseIterator && !reverseIterator->hasLogs()) ||
         (time > getEndTime().toStdSysMilliseconds() && iterator && !iterator->hasLogs())))
    {
        qDebug() << "LogModel::goToTime: requested time is already in the current range.";
        QModelIndex index;
        for (size_t i = 0; i < logs.size(); ++i)
        {
            if (logs[i].entry.time >= time)
            {
                index = createIndex(i, 0);
                break;
            }
        }
        if (!index.isValid())
            index = createIndex(logs.size() - 1, 0);
        requestedTimeAvailable(index);
        return;
    }

    qDebug() << "Go to time: " << DateTimeFromChronoSystemClock(time).toString(Qt::ISODateWithMs) << " in LogModel.";

    beginResetModel();
    ScopeGuard resetGuard([this] {
        endResetModel();
    });

    logs.clear();
    requestedTime = DateTimeFromChronoSystemClock(time);

    const MergeHeapCache* upperEntryCache = nullptr;
    const MergeHeapCache* lowerEntryCache = nullptr;

    auto upperCacheIt = entryCache.upper_bound(MergeHeapCache{ time });
    if (upperCacheIt != entryCache.end())
        upperEntryCache = &*upperCacheIt;
    auto lowerCacheIt = upperCacheIt;
    if (lowerCacheIt != entryCache.begin())
    {
        --lowerCacheIt;
        lowerEntryCache = &*lowerCacheIt;
    }

    if ((lowerEntryCache && !lowerEntryCache->heap.empty() && upperEntryCache && !upperEntryCache->heap.empty()) ||
        (lowerEntryCache && !lowerEntryCache->heap.empty() && lowerEntryCache->time == time))
    {
        if (!upperEntryCache || time - lowerEntryCache->time < upperEntryCache->time - time)
        {
            if (!reverseIterator || reverseIterator->getCurrentTime() != lowerEntryCache->time)
                reverseIterator = createIterator<false>(*lowerEntryCache, startTime.toStdSysMilliseconds(), lowerEntryCache->time);
            if (!iterator || iterator->getCurrentTime() != lowerEntryCache->time)
                iterator = createIterator<true>(*lowerEntryCache, lowerEntryCache->time, endTime.toStdSysMilliseconds());
        }
        else if (!lowerEntryCache || upperEntryCache->time - time < time - lowerEntryCache->time)
        {
            if (!reverseIterator || reverseIterator->getCurrentTime() != upperEntryCache->time)
                reverseIterator = createIterator<false>(*upperEntryCache, startTime.toStdSysMilliseconds(), upperEntryCache->time);
            if (!iterator || iterator->getCurrentTime() != upperEntryCache->time)
                iterator = createIterator<true>(*upperEntryCache, upperEntryCache->time, endTime.toStdSysMilliseconds());
        }
        return;
    }
    else if (lowerEntryCache && !lowerEntryCache->heap.empty())
    {
        auto newTime = lowerEntryCache->time;
        ++newTime;
        entryCache.emplace_hint(upperCacheIt, MergeHeapCache{ newTime });
    }
    else if (upperEntryCache && !upperEntryCache->heap.empty())
    {
        auto newTime = upperEntryCache->time;
        --newTime;
        entryCache.emplace_hint(upperCacheIt, MergeHeapCache{ newTime });
    }

    if (time < getStartTime().toStdSysMilliseconds())
    {
        MergeHeapCache newCache{ getStartTime().toStdSysMilliseconds() };
        iterator = createIterator<true>(newCache, startTime.toStdSysMilliseconds(), endTime.toStdSysMilliseconds());
        reverseIterator = createIterator<false>(newCache, startTime.toStdSysMilliseconds(), endTime.toStdSysMilliseconds());
        dataRequests[service->requestLogEntries(iterator, blockSize)] = DataRequestType::ReplaceForward;
        entryCache.emplace(std::move(newCache));
    }
    else if (time > getEndTime().toStdSysMilliseconds())
    {
        MergeHeapCache newCache{ getEndTime().toStdSysMilliseconds() };
        iterator = createIterator<true>(newCache, startTime.toStdSysMilliseconds(), endTime.toStdSysMilliseconds());
        reverseIterator = createIterator<false>(newCache, startTime.toStdSysMilliseconds(), endTime.toStdSysMilliseconds());
        dataRequests[service->requestLogEntries(reverseIterator, blockSize)] = DataRequestType::ReplaceBackward;
        entryCache.emplace(std::move(newCache));
    }
    else
    {
        iteratorIndex = service->requestIterator(time, endTime.toStdSysMilliseconds());
    }
}

bool LogModel::canFetchUpMore() const
{
    return reverseIterator && reverseIterator->hasLogs();
}

void LogModel::fetchUpMore()
{
    fetchUpMoreImpl(reverseIterator);
}

void LogModel::fetchUpMore(const LogFilter& filter)
{
    auto filtered = std::make_shared<FilteredLogIterator<false>>(reverseIterator, filter);
    fetchUpMoreImpl(filtered);
}

bool LogModel::canFetchDownMore() const
{
    return iterator && iterator->hasLogs();
}

bool LogModel::isFulled() const
{
    return logs.size() >= static_cast<size_t>(blockSize * blockCount);
}

void LogModel::fetchDownMore()
{
    fetchDownMoreImpl(iterator);
}

void LogModel::fetchDownMore(const LogFilter& filter)
{
    auto filtered = std::make_shared<FilteredLogIterator<true>>(iterator, filter);
    fetchDownMoreImpl(filtered);
}

void LogModel::fetchUpMoreImpl(const std::shared_ptr<LogEntryIterator<false>>& it)
{
    if (!dataRequests.empty())
        return;

    if (!it || !it->hasLogs())
        return;

    auto cacheIt = entryCache.lower_bound({ it->getCurrentTime() });
    if (cacheIt != entryCache.begin())
    {
        --cacheIt;
        if (cacheIt != entryCache.begin() && cacheIt->heap.empty())
            --cacheIt;
    }
    else
    {
        cacheIt = entryCache.end();
    }

    if (cacheIt == entryCache.end())
        dataRequests[service->requestLogEntries(it, blockSize)] = DataRequestType::Prepend;
    else
        dataRequests[service->requestLogEntries(it, blockSize, cacheIt->time)] = DataRequestType::Prepend;
}

void LogModel::fetchDownMoreImpl(const std::shared_ptr<LogEntryIterator<true>>& it)
{
    if (!dataRequests.empty())
        return;

    if (!it || !it->hasLogs())
        return;

    auto cacheIt = entryCache.upper_bound({ it->getCurrentTime() });
    while (cacheIt != entryCache.end() && cacheIt->heap.empty())
        ++cacheIt;

    if (cacheIt == entryCache.end())
        dataRequests[service->requestLogEntries(it, blockSize)] = DataRequestType::Append;
    else
        dataRequests[service->requestLogEntries(it, blockSize, cacheIt->time)] = DataRequestType::Append;
}

const std::vector<Format::Field>& LogModel::getFields() const
{
    return fields;
}

const std::unordered_set<QVariant, VariantHash> LogModel::availableValues(int section) const
{
    if (section < 0 || section >= columnCount())
        return {};

    if (section == static_cast<int>(PredefinedColumn::Module))
    {
        std::unordered_set<QVariant, VariantHash> res;
        for (const auto& module : modules)
            res.emplace(module);
        return res;
    }

    const auto& field = getField(section);
    if (!field.isEnum)
        return {};

    return service->getSession()->getEnumList(field.name);
}

QDateTime convertToQDateTime(const std::chrono::system_clock::time_point& timePoint)
{
    auto duration = timePoint.time_since_epoch();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    return QDateTime::fromMSecsSinceEpoch(millis, Qt::UTC);
}

QDateTime LogModel::getStartTime() const
{
    return startTime;
}

QDateTime LogModel::getEndTime() const
{
    return endTime;
}

QDateTime LogModel::getFirstEntryTime() const
{
    if (logs.empty())
        return QDateTime();

    return convertToQDateTime(logs.front().entry.time);
}

QDateTime LogModel::getLastEntryTime() const
{
    if (logs.empty())
        return QDateTime();

    return convertToQDateTime(logs.back().entry.time);
}

QStringList LogModel::getFieldsName()
{
    QStringList fieldNames;
    for (const auto& field : fields)
        fieldNames.append(field.name);
    fieldNames.insert(fieldNames.begin() + 1, "__module");
    return fieldNames;
}

QVariant LogModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (section < 0 || section >= columnCount())
        return QVariant();

    if (orientation != Qt::Horizontal)
        return QVariant();

    if (role == Qt::DisplayRole)
    {
        if (section == static_cast<int>(PredefinedColumn::Module))
            return tr("module");
        else
            return getField(section).name;
    }

    return QVariant();
}

QModelIndex LogModel::index(int row, int column, const QModelIndex& parent) const
{
    if (parent.isValid())
    {
        if (parent.internalPointer() != nullptr)
            return QModelIndex();

        if (parent.row() < 0 || parent.row() >= logs.size() || parent.column() < 0 || parent.column() >= columnCount())
            return QModelIndex();

        if (row < 0 || row >= 1)
            return QModelIndex();

        if (column < 0 || column >= columnCount())
            return QModelIndex();

        return createIndex(row, column, &logs[parent.row()].index);
    }

    if (row < 0 || row >= rowCount() || column < 0 || column >= columnCount())
        return QModelIndex();

    return createIndex(row, column);
}

QModelIndex LogModel::parent(const QModelIndex& index) const
{
    if (!index.isValid() || index.internalPointer() == nullptr)
        return QModelIndex();

    size_t parentIndex = getParentIndex(index);
    if (parentIndex >= logs.size() || logs[parentIndex].entry.additionalLines.isEmpty() || index.column() < 0 || index.column() >= columnCount())
        return QModelIndex();

    return createIndex(parentIndex, 0);
}

int LogModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid())
    {
        if (parent.internalPointer() != nullptr)
            return 0;

        if (parent.row() < 0 || parent.row() >= logs.size() ||
            parent.column() < 0 || parent.column() >= columnCount())
        {
            return 0;
        }

        return logs[parent.row()].entry.additionalLines.isEmpty() ? 0 : 1;
    }

    return logs.size();
}

int LogModel::columnCount(const QModelIndex& parent) const
{
    return fields.size() + 1;
}

bool LogModel::hasChildren(const QModelIndex& parent) const
{
    if (parent.isValid())
    {
        if (parent.row() < 0 || parent.row() >= logs.size() ||
            parent.column() < 0 || parent.column() >= columnCount())
        {
            return false;
        }

        if (parent.internalPointer() == nullptr)
            return !logs[parent.row()].entry.additionalLines.isEmpty();
        else
            return false;
    }

    return !logs.empty();
}

QVariant LogModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (index.column() < 0 || index.column() >= columnCount(index.parent()) ||
        index.row() < 0 || index.row() >= rowCount(index.parent()))
    {
        return QVariant();
    }

    switch(role)
    {
    case Qt::FontRole:
        return QFontDatabase::systemFont(QFontDatabase::FixedFont);

    case Qt::DisplayRole:
    case Qt::EditRole:
    {
        if (index.internalPointer() != nullptr)
        {
            if (index.column() == columnCount() - 1)
                return logs[getParentIndex(index)].entry.additionalLines;
            else
                return QVariant();
        }

        const auto& log = logs[index.row()];

        if (index.column() == static_cast<int>(PredefinedColumn::Module))
        {
            return log.entry.module;
        }
        else
        {
            const auto& field = getField(index.column());
            auto valueIt = log.entry.values.find(field.name);
            if (valueIt != log.entry.values.end())
                return valueIt->second;
        }
        break;
    }
    case static_cast<int>(MetaData::Line):
        if (index.internalPointer() == nullptr)
            return logs[index.row()].entry.line;
        break;
    case static_cast<int>(MetaData::Message):
        if (index.internalPointer() == nullptr)
        {
            const auto& log = logs[index.row()];
            const auto& field = getField(index.column());
            auto valueIt = log.entry.values.find(field.name);
            if (valueIt != log.entry.values.end())
                return valueIt->second.toString() + '\n' + logs[index.row()].entry.additionalLines;
        }
        break;
    case static_cast<int>(MetaData::Time):
        if (index.internalPointer() == nullptr)
        {
            const auto& log = logs[index.row()];
            return convertToQDateTime(log.entry.time);
        }
        break;
    }

    return QVariant();
}

Qt::ItemFlags LogModel::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    return QAbstractItemModel::flags(index) | Qt::ItemIsSelectable | Qt::ItemIsEditable;
}

LogService* LogModel::getService() const
{
    return service;
}

void LogModel::handleIterator(int index, bool isStraight)
{
    QT_SLOT_BEGIN
    if (iteratorIndex == index)
    {
        skipDataRequests();

        if (isStraight)
        {
            iterator = service->getIterator(index);

            auto newCache = iterator->getCache();
            auto connection = detectConnection(newCache);
            if (connection != Connection::None)
            {
                qWarning() << "LogModel::handleIterator: iterator expected to be isolated from previos data, but it is not";
                reinitIteratorsWithClosestTime(newCache, connection);
                return;
            }

            reverseIterator = createIterator<false>(newCache, startTime.toStdSysMilliseconds(), endTime.toStdSysMilliseconds());
            dataRequests[service->requestLogEntries(iterator, blockSize)] = DataRequestType::ReplaceForward;

            entryCache.emplace(std::move(newCache));
        }
        else
        {
            reverseIterator = service->getReverseIterator(index);

            auto newCache = reverseIterator->getCache();
            auto connection = detectConnection(newCache);
            if (connection != Connection::None)
            {
                qWarning() << "LogModel::handleIterator: iterator expected to be isolated from previos data, but it is not";
                reinitIteratorsWithClosestTime(newCache, connection);
                return;
            }

            iterator = createIterator<true>(newCache, startTime.toStdSysMilliseconds(), endTime.toStdSysMilliseconds());
            dataRequests[service->requestLogEntries(reverseIterator, blockSize)] = DataRequestType::ReplaceBackward;

            entryCache.emplace(std::move(newCache));
        }
    }
    QT_SLOT_END
}

void LogModel::handleData(int index)
{
    QT_SLOT_BEGIN

    DataRequestType requestType = handleDataRequest(index);
    auto data = service->getResult(index);

    switch(requestType)
    {
    case DataRequestType::Append:
        if (logs.size() + data.size() > blockSize * blockCount)
            startPageSwap();

        beginInsertRows(QModelIndex(), logs.size(), logs.size() + data.size() - 1);
        for (const auto& entry : data)
        {
            LogItem item;
            item.entry = entry;
            item.index = logs.size();
            logs.push_back(item);
        }
        endInsertRows();

        if (iterator)
        {
            auto newCache = iterator->getCache();
            auto placeIt = entryCache.lower_bound(newCache);
            if (placeIt == entryCache.end() || placeIt->time != newCache.time)
            {
                auto it = entryCache.emplace_hint(placeIt, std::move(newCache));
                if (it != entryCache.begin())
                {
                    --it;
                    if (it->heap.empty())
                    {
                        it = entryCache.erase(it);
                        auto emptyCacheTime = newCache.time;
                        ++emptyCacheTime;
                        entryCache.emplace_hint(placeIt, MergeHeapCache{ emptyCacheTime });
                    }
                }
            }
            else
            {
                --placeIt;
                if (placeIt->heap.empty())
                    entryCache.erase(placeIt);
            }
        }

        if (logs.size() > blockSize * blockCount)
        {
            qDebug() << "Logs size exceeds block count limit (" << blockSize * blockCount << ") and will be truncated at the beginning.";

            decltype(entryCache)::iterator cacheIt;
            if (reverseIterator)
                cacheIt = entryCache.find({ reverseIterator->getCurrentTime() });
            else
                cacheIt = entryCache.begin();

            if (cacheIt == entryCache.end())
                qCritical() << "LogModel::handleData: no cache found for reverse iterator";
            else if (++cacheIt != entryCache.end())
                reverseIterator = createIterator<false>(*cacheIt, startTime.toStdSysMilliseconds(), endTime.toStdSysMilliseconds());

            beginRemoveRows(QModelIndex(), 0, logs.size() - blockSize * blockCount - 1);
            logs.erase(logs.begin(), logs.begin() + logs.size() - blockSize * blockCount);
            for (size_t i = 0; i < logs.size(); ++i)
                logs[i].index = i;
            endRemoveRows();

            endPageSwap(logs.size() + data.size() - blockSize * blockCount);
        }
        break;

    case DataRequestType::Prepend:
        if (logs.size() + data.size() > blockSize * blockCount)
            startPageSwap();

        beginInsertRows(QModelIndex(), 0, data.size() - 1);
        for (int i = 0; i < data.size(); ++i)
        {
            LogItem item;
            item.entry = data[i];
            item.index = data.size() - i - 1;
            logs.push_front(item);
        }
        for (size_t i = data.size(); i < logs.size(); ++i)
            logs[i].index = i;
        endInsertRows();

        if (reverseIterator)
        {
            auto newCache = reverseIterator->getCache();
            auto placeIt = entryCache.lower_bound(newCache);
            if (placeIt->time != newCache.time)
            {
                auto it = entryCache.emplace_hint(placeIt, std::move(newCache));
                if (placeIt->heap.empty())
                {
                    entryCache.erase(placeIt);
                    auto emptyCacheTime = newCache.time;
                    --emptyCacheTime;
                    entryCache.emplace_hint(it, MergeHeapCache{ emptyCacheTime });
                }
            }
            else
            {
                ++placeIt;
                if (placeIt->heap.empty())
                    entryCache.erase(placeIt);
            }
        }

        if (logs.size() > blockSize * blockCount)
        {
            qDebug() << "Logs size exceeds block count limit (" << blockSize * blockCount << ") and will be truncated at the end.";

            decltype(entryCache)::iterator cacheIt;
            if (iterator)
                cacheIt = entryCache.find({ iterator->getCurrentTime() });
            else
                cacheIt = entryCache.end();

            if (cacheIt != entryCache.begin())
                --cacheIt;

            if (cacheIt != entryCache.end())
                iterator = createIterator<true>(*cacheIt, cacheIt->time, endTime.toStdSysMilliseconds());

            beginRemoveRows(QModelIndex(), blockSize * blockCount, logs.size() - 1);
            logs.erase(logs.begin() + blockSize * blockCount, logs.end());
            endRemoveRows();

            endPageSwap(blockSize * blockCount - data.size() - logs.size());
        }
        break;

    case DataRequestType::ReplaceForward:
    case DataRequestType::ReplaceBackward:
    {
        beginResetModel();
        logs.clear();

        if (requestType == DataRequestType::ReplaceForward)
        {
            for (int i = 0; i < data.size(); ++i)
            {
                LogItem item;
                item.entry = data[i];
                item.index = logs.size();
                logs.push_back(item);
            }
        }
        else
        {
            for (int i = 0; i < data.size(); ++i)
            {
                LogItem item;
                item.entry = data[i];
                item.index = data.size() - i - 1;
                logs.push_front(item);
            }
        }
        endResetModel();

        if (requestedTime.isValid())
        {
            int requestedEntry = -1;
            for (int i = 0; i < logs.size(); ++i)
            {
                std::chrono::system_clock::time_point requestedTimePoint = requestedTime.toStdSysMilliseconds();
                if (logs[i].entry.time >= requestedTimePoint)
                {
                    requestedEntry = logs[i].index;
                    break;
                }
            }
            if (requestedEntry != -1)
                requestedTimeAvailable(createIndex(requestedEntry, 0));
        }

        if (iterator && requestType == DataRequestType::ReplaceForward)
            entryCache.emplace(iterator->getCache());
        else if (reverseIterator && requestType == DataRequestType::ReplaceBackward)
            entryCache.emplace(reverseIterator->getCache());

        if (logs.size() > blockSize * blockCount)
        {
            qWarning() << "LogModel::handleData: logs size exceeds block count limit (" << blockSize * blockCount << ")";
        }
        else if (requestType == DataRequestType::ReplaceForward)
        {
            if (reverseIterator && reverseIterator->hasLogs())
                dataRequests[service->requestLogEntries(reverseIterator, blockSize)] = DataRequestType::Prepend;
            else if (iterator && iterator->hasLogs())
                dataRequests[service->requestLogEntries(iterator, blockSize)] = DataRequestType::Append;
        }
        else if (requestType == DataRequestType::ReplaceBackward)
        {
            if (iterator && iterator->hasLogs())
                dataRequests[service->requestLogEntries(iterator, blockSize)] = DataRequestType::Append;
            else if (reverseIterator && reverseIterator->hasLogs())
                dataRequests[service->requestLogEntries(reverseIterator, blockSize)] = DataRequestType::Prepend;
        }
        break;
    }

    default:
        qWarning() << "LogModel::handleData: unexpected data request type:" << static_cast<int>(requestType);
        break;
    }

    QT_SLOT_END
}

void LogModel::skipDataRequests()
{
    dataRequests.clear();
}

const Format::Field& LogModel::getField(int section) const
{
    if (section == 0)
        return fields[0];
    return fields[section - 1];
}

size_t LogModel::getParentIndex(const QModelIndex& index) const
{
    return *reinterpret_cast<size_t*>(index.internalPointer());
}

LogModel::DataRequestType LogModel::handleDataRequest(int index)
{
    auto it = dataRequests.find(index);
    if (it == dataRequests.end())
        return DataRequestType::None;

    auto requestType = it->second;
    dataRequests.erase(it);
    return requestType;
}

LogModel::Connection LogModel::detectConnection(const MergeHeapCache& entry) const
{
    auto it = entryCache.upper_bound(entry);
    if (it != entryCache.end() && !it->heap.empty())
        return Connection::Up;

    if (it != entryCache.begin())
    {
        --it;
        if (!it->heap.empty() || it->time == entry.time)
            return Connection::Down;
    }

    return Connection::None;
}

void LogModel::reinitIteratorsWithClosestTime(const MergeHeapCache& newCache, Connection connection)
{
    switch (connection)
    {
    default:
    case Connection::Up:
    {
        auto it = entryCache.upper_bound(newCache);
        if (it == entryCache.end())
            throw std::logic_error("LogModel::handleIterator: unexpected end of cache");

        iterator = createIterator<true>(*it, startTime.toStdSysMilliseconds(), endTime.toStdSysMilliseconds());
        reverseIterator = createIterator<false>(*it, startTime.toStdSysMilliseconds(), endTime.toStdSysMilliseconds());

        break;
    }
    case Connection::Down:
    {
        auto it = entryCache.upper_bound(newCache);
        if (it == entryCache.begin())
            throw std::logic_error("LogModel::handleIterator: unexpected beginning of cache");
        --it;

        iterator = createIterator<true>(*it, startTime.toStdSysMilliseconds(), endTime.toStdSysMilliseconds());
        reverseIterator = createIterator<false>(*it, startTime.toStdSysMilliseconds(), endTime.toStdSysMilliseconds());

        break;
    }
    }

    if (iterator->hasLogs())
    {
        dataRequests[service->requestLogEntries(iterator, blockSize)] = DataRequestType::ReplaceForward;
        if (reverseIterator->hasLogs())
            dataRequests[service->requestLogEntries(reverseIterator, blockSize)] = DataRequestType::Prepend;
    }
    else
    {
        dataRequests[service->requestLogEntries(reverseIterator, blockSize)] = DataRequestType::ReplaceBackward;
    }
}

int LogModel::loadBlockSize()
{
    static const int defaultSize = 2000;
    static const QString BlockSizeParameter = "/blockSize";

    Settings settings;
    if (settings.contains(LogViewSettings + BlockSizeParameter))
        return settings.value(LogViewSettings + BlockSizeParameter, defaultSize).toInt();
    else
        settings.setValue(LogViewSettings + BlockSizeParameter, defaultSize);
    return defaultSize;
}

int LogModel::loadBlockCount()
{
    static const int defaultSize = 4;
    static const QString BlockCountParameter = "/blockCount";

    Settings settings;
    if (settings.contains(LogViewSettings + BlockCountParameter))
        return settings.value(LogViewSettings + BlockCountParameter, defaultSize).toInt();
    else
        settings.setValue(LogViewSettings + BlockCountParameter, defaultSize);
    return defaultSize;
}
