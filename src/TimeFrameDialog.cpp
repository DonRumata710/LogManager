#include "TimeFrameDialog.h"

#include <QDateTimeEdit>
#include <QDialogButtonBox>
#include <QVBoxLayout>

TimeFrameDialog::TimeFrameDialog(const QDateTime& start, const QDateTime& end, QWidget* parent)
    : QDialog(parent)
{
    auto layout = new QVBoxLayout(this);
    startEdit = new QDateTimeEdit(start, this);
    endEdit = new QDateTimeEdit(end, this);
    startEdit->setCalendarPopup(true);
    endEdit->setCalendarPopup(true);
    startEdit->setMaximumDateTime(end);
    endEdit->setMinimumDateTime(start);
    connect(startEdit, &QDateTimeEdit::dateTimeChanged, this, &TimeFrameDialog::onStartChanged);
    connect(endEdit, &QDateTimeEdit::dateTimeChanged, this, &TimeFrameDialog::onEndChanged);
    layout->addWidget(startEdit);
    layout->addWidget(endEdit);

    buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    layout->addWidget(buttonBox);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

QDateTime TimeFrameDialog::startDateTime() const
{
    return startEdit->dateTime();
}

QDateTime TimeFrameDialog::endDateTime() const
{
    return endEdit->dateTime();
}

void TimeFrameDialog::onStartChanged(const QDateTime& dt)
{
    endEdit->setMinimumDateTime(dt);
}

void TimeFrameDialog::onEndChanged(const QDateTime& dt)
{
    startEdit->setMaximumDateTime(dt);
}

