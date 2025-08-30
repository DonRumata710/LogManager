#include "EntryLinkView.h"

#include "EntryLinkModel.h"

#include <QMouseEvent>


EntryLinkView::EntryLinkView(QWidget* parent) : QListView(parent)
{}

void EntryLinkView::mouseDoubleClickEvent(QMouseEvent* event)
{
    QModelIndex index = indexAt(event->pos());
    if (index.isValid())
    {
        EntryLinkModel* entryModel = qobject_cast<EntryLinkModel*>(model());
        if (entryModel)
        {
            auto entry = entryModel->data(index, EntryLinkModel::Role::TimeRole).value<std::chrono::system_clock::time_point>();
            emit entryActivated(entry);
        }
    }

    QListView::mouseDoubleClickEvent(event);
}
