#include "LogModel.h"


LogModel::LogModel(std::unique_ptr<LogManager>&& logManager, QObject *parent) :
    QAbstractItemModel(parent),
    manager(std::move(logManager)),
    modules(manager->getModules())
{
    for (const auto& format : manager->getFormats())
    {
        for (const auto& field : format->fields)
        {
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
        if (!manager->getModules().contains(module))
            throw std::runtime_error("Unexpected module: " + module.toStdString());
    }

    beginResetModel();
    modules = _modules;
    update();
    endResetModel();
}

bool LogModel::canFetchDownMore() const
{
    return manager->hasLogs();
}

void LogModel::fetchDownMore()
{
    if (!canFetchDownMore())
        return;

    auto time = logs.back().entry.time;

    std::vector<LogEntry> newLogs;
    while (newLogs.size() < BatchSize)
    {
        auto nextEntry = manager->next();
        if (!nextEntry)
            break;

        newLogs.push_back(nextEntry.value());
    }

    beginInsertRows(QModelIndex(), logs.size(), logs.size() + newLogs.size() - 1);
    for (const auto& entry : newLogs)
        logs.emplace_back(std::move(entry), logs.size());
    endInsertRows();
}

const std::vector<Format::Field>& LogModel::getFields() const
{
    return fields;
}

const std::unordered_set<QVariant, VariantHash> LogModel::availableValues(int section) const
{
    if (section < 0 || section >= fields.size())
        return {};

    if (section == 0)
    {
        std::unordered_set<QVariant, VariantHash> modules;
        for (const auto& module: manager->getModules())
            modules.emplace(module);
        return modules;
    }

    const auto& field = getField(section);
    if (!field.isEnum)
        return {};

    return manager->getEnumList(field.name);
}

QVariant LogModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (section < 0 || section >= columnCount())
        return QVariant();

    if (orientation != Qt::Horizontal)
        return QVariant();

    if (role == Qt::DisplayRole)
    {
        if (section == 0)
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

        if (index.column() == 0)
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

void LogModel::update()
{
    logs.clear();

    while (logs.size() < BatchSize * 2)
    {
        auto entry = manager->next();
        if (!entry)
            break;

        if (modules.empty() || modules.contains(entry->module))
        {
            logs.emplace_back(std::move(entry.value()), logs.size());
        }
    }
}

const Format::Field& LogModel::getField(int section) const
{
    return fields[section - 1];
}

size_t LogModel::getParentIndex(const QModelIndex& index) const
{
    return *reinterpret_cast<size_t*>(index.internalPointer());
}
