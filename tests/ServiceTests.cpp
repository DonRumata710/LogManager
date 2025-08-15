#include <QtTest/QtTest>
#include <QTemporaryDir>
#include <QTextStream>

#include "Application.h"
#include "services/SessionService.h"
#include "services/SearchService.h"
#include "services/ExportService.h"


class ServiceTests : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void testSessionService();
    void testSearchService();
    void testExportService();

private:
    Application* app;
    SessionService* sessionService;
    SearchService* searchService;
    ExportService* exportService;
    QTemporaryDir* tempDir;
    QString logFile;
    QDateTime firstTime;
    QDateTime secondTime;
};


void ServiceTests::initTestCase()
{
    app = qobject_cast<Application*>(qApp);
    QVERIFY(app);
    sessionService = new SessionService();
    searchService = new SearchService(sessionService);
    exportService = new ExportService(sessionService);

    tempDir = new QTemporaryDir();
    logFile = tempDir->filePath("test.log.csv");

    QFile file(logFile);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream out(&file);
    out << "2023-01-01 00:00:00.000;1;1;mod;info;hello\n";
    out << "2023-01-01 00:01:00.000;1;1;mod;info;searchterm\n";
    file.close();

    firstTime = QDateTime::fromString("2023-01-01 00:00:00", "yyyy-MM-dd HH:mm:ss");
    secondTime = QDateTime::fromString("2023-01-01 00:01:00", "yyyy-MM-dd HH:mm:ss");

    std::shared_ptr<Format> format = std::make_shared<Format>();
    format->name = "TestFormat";
    format->extension = ".csv";
    format->separator = ";";
    format->timeFieldIndex = 0;
    format->timeMask = "%F %H:%M:%S";
    format->timeFractionalDigits = 3;
    for (int i = 0; i < 6; ++i)
    {
        Format::Field f;
        f.name = QString::number(i);
        f.regex = QRegularExpression(".*");
        f.type = QMetaType::QString;
        format->fields.push_back(f);
    }
    app->getFormatManager().addFormat(format);

    sessionService->openFile(logFile, QStringList() << "TestFormat");
    sessionService->createSession(sessionService->getLogManager()->getModules(), firstTime.toStdSysMilliseconds(), secondTime.toStdSysMilliseconds());
}

void ServiceTests::cleanupTestCase()
{
    delete exportService;
    delete searchService;
    delete sessionService;
    delete tempDir;
}

void ServiceTests::testSessionService()
{
    int idx = sessionService->requestIterator(firstTime.toStdSysMilliseconds(), secondTime.toStdSysMilliseconds());
    QSignalSpy spy(sessionService, &SessionService::iteratorCreated);
    QVERIFY(spy.wait(1000));
    auto iterator = sessionService->getIterator(idx);
    QVERIFY(iterator);
    int req = sessionService->requestLogEntries(iterator, 2);
    QSignalSpy spyData(sessionService, &SessionService::dataLoaded);
    QVERIFY(spyData.wait(1000));
    auto result = sessionService->getResult(req);
    QCOMPARE(result.size(), 2);
    QCOMPARE(result.back().line.contains("searchterm"), true);
}

void ServiceTests::testSearchService()
{
    QSignalSpy spy(searchService, &SearchService::searchFinished);
    QVERIFY2(spy.isValid(), "QSignalSpy: failed to connect to SearchService::searchFinished");
    searchService->search(firstTime, "searchterm", false, false, false);

    if (spy.isEmpty())
        QVERIFY(spy.wait(1000));
    QVERIFY(!spy.isEmpty());

    QList<QVariant> args = spy.takeFirst();
    QCOMPARE(args.at(0).toString(), QString("searchterm"));
}

void ServiceTests::testExportService()
{
    QString outFile = tempDir->filePath("export.csv");
    exportService->exportData(outFile, firstTime, secondTime);
    QFile file(outFile);
    QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));
    QString content = file.readAll();
    QVERIFY(content.contains("searchterm"));
}

int main(int argc, char** argv)
{
    Application app(argc, argv);
    ServiceTests tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "ServiceTests.moc"
