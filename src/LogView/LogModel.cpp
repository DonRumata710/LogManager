#include "LogModel.h"


LogModel::LogModel(std::unique_ptr<LogManager>&& logManager, QObject *parent) :
    QAbstractTableModel(parent),
    manager(std::move(logManager)),
    modules(manager->getModules())
{
    for (const auto& format : manager->getFormats())
    {
        for (const auto& field : format->fields)
            fields.push_back(field);
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

    auto time = logs.back().time;

    std::vector<LogEntry> newLogs;
    while (newLogs.size() < BatchSize)
    {
        auto nextEntry = manager->next();
        if (!nextEntry)
            break;

        newLogs.push_back(nextEntry.value());
    }

    beginInsertRows(QModelIndex(), logs.size(), logs.size() + newLogs.size() - 1);
    logs.insert(logs.end(), newLogs.begin(), newLogs.end());
    endInsertRows();
}

const std::vector<Format::Field>& LogModel::getFields() const
{
    return fields;
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
            return fields[section - 1].name;
    }

    return QVariant();
}

int LogModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;

    return logs.size();
}

int LogModel::columnCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;

    return fields.size() + 1;
}

bool LogModel::hasChildren(const QModelIndex& parent) const
{
    return parent.isValid() ? false : !logs.empty();
}

QVariant LogModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (index.column() < 0 || index.column() >= columnCount() ||
        index.row() < 0 || index.row() >= rowCount())
    {
        return QVariant();
    }

    if (role == Qt::DisplayRole || role == Qt::EditRole)
    {
        const auto& log = logs[index.row()];

        if (index.column() == 0)
        {
            return log.module;
        }
        else
        {
            const auto& field = fields[index.column() - 1];
            auto valueIt = log.values.find(field.name);
            if (valueIt != log.values.end())
                return valueIt->second;
        }
    }

    return QVariant();
}

Qt::ItemFlags LogModel::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    if (index.column() < 0 || index.column() >= columnCount() ||
        index.row() < 0 || index.row() >= rowCount())
    {
        return Qt::NoItemFlags;
    }

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
            logs.push_back(std::move(entry.value()));
        }
    }
}
