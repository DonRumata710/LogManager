#pragma once

#include <QListView>


class EntryLinkView : public QListView
{
    Q_OBJECT

public:
    EntryLinkView(QWidget* parent = nullptr);

signals:
    void entryActivated(const std::chrono::system_clock::time_point& entry);

private:
    void mouseDoubleClickEvent(QMouseEvent* event) override;
};
