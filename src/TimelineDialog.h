#pragma once

#include <QDialog>
#include <vector>

#include "Statistics/LogHistogram.h"

class TimelineDialog : public QDialog
{
    Q_OBJECT
public:
    explicit TimelineDialog(const std::vector<Statistics::Bucket>& data, QWidget* parent = nullptr);

private:
    void populateChart(const std::vector<Statistics::Bucket>& data);
};

