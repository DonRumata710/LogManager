#include "LogModel.h"

#include "LogViewUtils.h"
#include "./Settings.h"
#include "./Utils.h"


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
    for (const auto& format : logService->getLogManager()->getFormats())
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
    beginResetModel();

    logs.clear();

    anchorTime = time;
    if (entryCache.empty())
    {
        if (time < getStartTime().toStdSysMilliseconds())
            iteratorIndex = service->requestIterator(startTime.toStdSysMilliseconds(), endTime.toStdSysMilliseconds());
        else if (time > getEndTime().toStdSysMilliseconds())
            iteratorIndex = service->requestReverseIterator(startTime.toStdSysMilliseconds(), endTime.toStdSysMilliseconds());
        else if (time - getStartTime().toStdSysMilliseconds() < getEndTime().toStdSysMilliseconds() - time)
            iteratorIndex = service->requestIterator(time, endTime.toStdSysMilliseconds());
        else
            iteratorIndex = service->requestReverseIterator(startTime.toStdSysMilliseconds(), time);
        return;
    }

    const MergeHeapCache* upperEntryCache = nullptr;
    const MergeHeapCache* lowerEntryCache = nullptr;

    auto cacheIt = entryCache.upper_bound(MergeHeapCache{ time });
    if (cacheIt != entryCache.end())
        upperEntryCache = &*cacheIt;
    if (cacheIt != entryCache.begin())
    {
        --cacheIt;
        lowerEntryCache = &*cacheIt;
    }

    if (lowerEntryCache && (!upperEntryCache || time - lowerEntryCache->time < upperEntryCache->time - time))
    {
        if (!reverseIterator || reverseIterator->getCurrentTime() != lowerEntryCache->time)
            reverseIterator = std::make_shared<LogEntryIterator<false>>(service->getSession()->createIterator<false>(lowerEntryCache->heap, time, lowerEntryCache->time));
        if (!iterator || iterator->getCurrentTime() != lowerEntryCache->time)
            iterator = std::make_shared<LogEntryIterator<true>>(service->getSession()->createIterator<true>(lowerEntryCache->heap, time, lowerEntryCache->time));
    }
    else if (upperEntryCache && (!lowerEntryCache || upperEntryCache->time - time < time - lowerEntryCache->time))
    {
        if (!reverseIterator || reverseIterator->getCurrentTime() != upperEntryCache->time)
            reverseIterator = std::make_shared<LogEntryIterator<false>>(service->getSession()->createIterator<false>(upperEntryCache->heap, time, upperEntryCache->time));
        if (!iterator || iterator->getCurrentTime() != upperEntryCache->time)
            iterator = std::make_shared<LogEntryIterator<true>>(service->getSession()->createIterator<true>(upperEntryCache->heap, time, upperEntryCache->time));
    }

    endResetModel();
}

bool LogModel::canFetchUpMore() const
{
    return reverseIterator && reverseIterator->hasLogs();
}

void LogModel::fetchUpMore()
{
    if (!dataRequests.empty())
        return;

    if (!canFetchUpMore())
        return;

    dataRequests[service->requestLogEntries(reverseIterator, blockSize)] = DataRequestType::Prepend;
}

bool LogModel::canFetchDownMore() const
{
    return iterator && iterator->hasLogs();
}

void LogModel::fetchDownMore()
{
    if (!dataRequests.empty())
        return;

    if (!canFetchDownMore())
        return;

    dataRequests[service->requestLogEntries(iterator, blockSize)] = DataRequestType::Append;
}

const std::vector<Format::Field>& LogModel::getFields() const
{
    return fields;
}

const std::unordered_set<QVariant, VariantHash> LogModel::availableValues(int section) const
{
    if (section < 0 || section >= fields.size())
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

    return service->getLogManager()->getEnumList(field.name);
}

QDateTime convertToQDateTime(const std::chrono::system_clock::time_point& timePoint)
{
    auto duration = timePoint.time_since_epoch();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    return QDateTime::fromMSecsSinceEpoch(millis, Qt::UTC);
}

QDateTime LogModel::getStartTime() const
{
    return convertToQDateTime(iterator->getStartTime());
}

QDateTime LogModel::getEndTime() const
{
    return convertToQDateTime(iterator->getEndTime());
}

QStringList LogModel::getFieldsName()
{
    QStringList fieldNames;
    for (const auto& field : fields)
        fieldNames.append(field.name);
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
    if (parentIndex >= logs.size() || logs[parentIndex].entry.additionalLines.isEmpty() || index.column() < 0 || index.column() >= 1)
        return QModelIndex();

    return createIndex(parentIndex, index.column());
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
    }

    return QVariant();
}

Qt::ItemFlags LogModel::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    return QAbstractItemModel::flags(index) | Qt::ItemIsSelectable | Qt::ItemIsEditable;
}

void LogModel::handleIterator(int index)
{
    QT_SLOT_BEGIN
    if (iteratorIndex == index)
    {
        iterator = service->getIterator(index);
        update();
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
            entryCache.emplace(iterator->getCache());

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
                reverseIterator = std::make_shared<LogEntryIterator<false>>(service->getSession()->createIterator<false>(cacheIt->heap, startTime.toStdSysMilliseconds(), cacheIt->time));

            beginRemoveRows(QModelIndex(), 0, logs.size() - blockSize * blockCount - 1);
            logs.erase(logs.begin(), logs.begin() + logs.size() - blockSize * blockCount);
            for (size_t i = 0; i < logs.size(); ++i)
                logs[i].index = i;
            endRemoveRows();

            endPageSwap();
        }
        break;

    case DataRequestType::Prepend:
        if (logs.size() + data.size() > blockSize * blockCount)
            startPageSwap();

        beginInsertRows(QModelIndex(), 0, data.size() - 1);
        for (size_t i = data.size() - 1; i < data.size(); --i)
        {
            LogItem item;
            item.entry = data[i];
            item.index = i;
            logs.push_front(item);
        }
        for (size_t i = data.size(); i < logs.size(); ++i)
            logs[i].index = i;
        endInsertRows();

        if (reverseIterator)
            entryCache.emplace(reverseIterator->getCache());

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
                iterator = std::make_shared<LogEntryIterator<true>>(service->getSession()->createIterator<true>(cacheIt->heap));

            beginRemoveRows(QModelIndex(), blockSize * blockCount, logs.size() - 1);
            logs.erase(logs.begin() + blockSize * blockCount, logs.end());
            endRemoveRows();

            endPageSwap();
        }
        break;

    case DataRequestType::ReplaceForward:
    case DataRequestType::ReplaceBackward:
        beginResetModel();
        logs.clear();

        for (int i = 0; i < data.size(); ++i)
        {
            LogItem item;
            item.entry = data[i];
            item.index = logs.size();
            logs.push_back(item);
        }
        endResetModel();

        if (iterator && requestType == DataRequestType::ReplaceForward)
            entryCache.emplace(iterator->getCache());
        else if (reverseIterator && requestType == DataRequestType::ReplaceBackward)
            entryCache.emplace(reverseIterator->getCache());

        if (logs.size() > blockSize * blockCount)
            qWarning() << "LogModel::handleData: logs size exceeds block count limit (" << blockSize * blockCount << ")";
        else
            dataRequests[service->requestLogEntries(iterator, blockSize)] = DataRequestType::Append;
        break;

    default:
        qWarning() << "LogModel::handleData: unexpected data request type:" << static_cast<int>(requestType);
        break;
    }

    QT_SLOT_END
}

void LogModel::update()
{
    if (!iterator && !reverseIterator)
        return;

    if (entryCache.empty())
        entryCache.emplace(iterator->getCache());

    dataRequests.clear();

    if (iterator)
        dataRequests[service->requestLogEntries(iterator, blockSize)] = DataRequestType::ReplaceForward;
    if (reverseIterator)
        dataRequests[service->requestLogEntries(reverseIterator, blockSize)] = (iterator ? DataRequestType::Prepend : DataRequestType::ReplaceBackward);
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

LogModel::DataRequestType LogModel::handleDataRequest(int index)
{
    auto it = dataRequests.find(index);
    if (it == dataRequests.end())
        return DataRequestType::None;

    auto requestType = it->second;
    dataRequests.erase(it);
    return requestType;
}
