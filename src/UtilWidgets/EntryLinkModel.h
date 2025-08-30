#pragma once

#include <QAbstractListModel>

#include <boost/container/flat_map.hpp>


class EntryLinkModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Role
    {
        TimeRole = Qt::UserRole + 1
    };

public:
    explicit EntryLinkModel(const QMap<std::chrono::system_clock::time_point, QString>& entries, QObject *parent = nullptr);

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

private:
    boost::container::flat_map<std::chrono::system_clock::time_point, QString> entries;
};
