#include <QtTest/QtTest>

#include <LogView/LogModel.h>
#include <LogService.h>


class LogModelTest : public QObject
{
    Q_OBJECT

private slots:
    void smokeTest();
};


void LogModelTest::smokeTest()
{

}

QTEST_MAIN(LogModelTest)
#include "LogModelTest.moc"
