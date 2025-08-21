#include <QtTest/QtTest>

#include "Statistics/LogHistogram.h"

using namespace std::chrono;

class LogHistogramTest : public QObject
{
    Q_OBJECT

private slots:
    void testSuggestBucketSize();
};

void LogHistogramTest::testSuggestBucketSize()
{
    auto start = system_clock::time_point{};
    auto end = start + minutes(10);
    auto bucket = Statistics::LogHistogram::suggestBucketSize(start, end, 5);
    QCOMPARE(bucket, minutes(2));

    auto shortEnd = start + seconds(30);
    auto bucket2 = Statistics::LogHistogram::suggestBucketSize(start, shortEnd, 100);
    QCOMPARE(bucket2, seconds(1));
}

QTEST_APPLESS_MAIN(LogHistogramTest)
#include "LogHistogramTest.moc"

