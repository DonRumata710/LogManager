#include "EntryLinkModel.h"


EntryLinkModel::EntryLinkModel(const QMap<std::chrono::system_clock::time_point, QString>& _entries, QObject *parent) : QAbstractListModel(parent)
{
    for (const auto& entry : _entries.keys())
        entries.emplace(entry, _entries.value(entry));
}

int EntryLinkModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;

    return entries.size();
}

QVariant EntryLinkModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (index.row() < 0 || index.row() >= entries.size())
        return QVariant();

    switch(role)
    {
    case Qt::DisplayRole:
        return entries.nth(index.row())->second;
    case Role::TimeRole:
        return QVariant::fromValue(entries.nth(index.row())->first);
    }

    return QVariant();
}
