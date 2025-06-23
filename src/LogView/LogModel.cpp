#include "LogModel.h"

#include "Utils.h"


LogModel::LogModel(LogService* logService, const QDateTime& startTime, QObject *parent) :
    QAbstractItemModel(parent),
    service(logService),
    modules(logService->getLogManager()->getModules())
{
    connect(service, &LogService::iteratorCreated, this, &LogModel::handleIterator);
    connect(service, &LogService::dataLoaded, this, &LogModel::handleData);

    iteratorIndex = service->requestIterator(startTime.toStdSysSeconds());

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

            auto it = std::find_if(fields.begin(), fields.end(),
                         [&field](const Format::Field& f) { return f.name == field.name; });
            if (it != fields.end())
                continue;
            fields.push_back(field);
        }
    }
}

void LogModel::setModules(const std::unordered_set<QString>& _modules)
{
    for (const auto& module : _modules)
    {
        if (!service->getLogManager()->getModules().contains(module))
            throw std::runtime_error("Unexpected module: " + module.toStdString());
    }

    beginResetModel();
    modules = _modules;
    update();
    endResetModel();
}

bool LogModel::canFetchDownMore() const
{
    return iterator && iterator->hasLogs();
}

void LogModel::fetchDownMore()
{
    if (!canFetchDownMore())
        return;

    dataRequests[service->requestLogEntries(iterator, BatchSize)] = DataRequestType::Append;
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
        std::unordered_set<QVariant, VariantHash> modules;
        for (const auto& module : service->getLogManager()->getModules())
            modules.emplace(module);
        return modules;
    }

    const auto& field = getField(section);
    if (!field.isEnum)
        return {};

    return service->getLogManager()->getEnumList(field.name);
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

    if (role == Qt::DisplayRole || role == Qt::EditRole)
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

    auto it = dataRequests.find(index);
    if (it == dataRequests.end())
        return;

    auto requestType = it->second;
    dataRequests.erase(it);

    auto data = service->getResult(index);

    switch(requestType)
    {
    case DataRequestType::Append:
        beginInsertRows(QModelIndex(), logs.size(), logs.size() + BatchSize - 1);
        for (const auto& entry : data)
        {
            LogItem item;
            item.entry = entry;
            item.index = logs.size();
            logs.push_back(item);
        }
        endInsertRows();
        break;

    case DataRequestType::Prepend:
        beginInsertRows(QModelIndex(), 0, BatchSize - 1);
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
        break;

    case DataRequestType::Replace:
        beginResetModel();
        logs.clear();
        for (const auto& entry : data)
        {
            LogItem item;
            item.entry = entry;
            item.index = logs.size();
            logs.push_back(item);
        }
        endResetModel();
        break;
    }

    QT_SLOT_END
}

void LogModel::update()
{
    dataRequests.clear();
    dataRequests[service->requestLogEntries(iterator, BatchSize * 4)] = DataRequestType::Replace;
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
