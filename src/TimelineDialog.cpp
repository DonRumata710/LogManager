#include "TimelineDialog.h"
#include "Utils.h"

#include <vector>
#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QValueAxis>
#include <QVBoxLayout>


TimelineDialog::TimelineDialog(const std::vector<Statistics::Bucket>& data, QWidget* parent) : QDialog(parent)
{
    auto layout = new QVBoxLayout(this);
    setLayout(layout);

    populateChart(data);
}

void TimelineDialog::populateChart(const std::vector<Statistics::Bucket>& data)
{
    auto series = new QBarSeries(this);
    auto set = new QBarSet(tr("Messages"));
    QStringList categories;
    for (const auto& bucket : data)
    {
        *set << bucket.count;
        categories << DateTimeFromChronoSystemClock(bucket.start).toString("HH:mm");
    }
    series->append(set);

    auto chart = new QChart();
    chart->addSeries(series);
    chart->setTitle(tr("Timeline"));
    chart->setAnimationOptions(QChart::SeriesAnimations);

    auto axisX = new QBarCategoryAxis();
    axisX->append(categories);
    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    auto axisY = new QValueAxis();
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);

    auto chartView = new QChartView(chart, this);
    chartView->setRenderHint(QPainter::Antialiasing);

    if (layout())
        layout()->addWidget(chartView);
}

