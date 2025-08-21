#pragma once

#include <QDialog>

class QDateTimeEdit;
class QDialogButtonBox;

class TimeFrameDialog : public QDialog
{
    Q_OBJECT

public:
    TimeFrameDialog(const QDateTime& start, const QDateTime& end, QWidget* parent = nullptr);

    QDateTime startDateTime() const;
    QDateTime endDateTime() const;

private slots:
    void onStartChanged(const QDateTime& dt);
    void onEndChanged(const QDateTime& dt);

private:
    QDateTimeEdit* startEdit;
    QDateTimeEdit* endEdit;
    QDialogButtonBox* buttonBox;
};

