#include <QSignalSpy>
#include <QBuffer>
#include <QTextStream>
#include <QtTest/QtTest>

#include "Application.h"
#include "LogService.h"
#include "LogView/LogModel.h"
#include "LogView/LogViewUtils.h"
#include "Settings.h"

static std::chrono::system_clock::time_point toTimePoint(const QDateTime &dt)
{
    return std::chrono::system_clock::time_point{ std::chrono::milliseconds{ dt.toMSecsSinceEpoch() } };
}

class LogModelTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    void testInitialLoad();
    void testAvailableModules();
    void testHeaderData();
    void testLoadMultipleBlocks();
    void testFetchDownMore();

private:
    Application *app = nullptr;
    LogService *logService = nullptr;
    QDateTime firstTime;
    QDateTime lastTime;
    QString entryTemplate = "msg%1";
    int entryCount = 10;
    int blockSize = 3;
};


void LogModelTest::initTestCase()
{
    app = qobject_cast<Application *>(qApp);
    QVERIFY(app);
    logService = app->getLogService();

    Settings settings;
    settings.setValue(LogViewSettings + "/blockSize", blockSize);
    settings.setValue(LogViewSettings + "/blockCount", 2);

    QByteArray data;
    QBuffer buffer(&data);
    QVERIFY(buffer.open(QIODevice::WriteOnly));
    QTextStream out(&buffer);

    QDateTime baseTime = QDateTime::fromString("2023-01-01 00:00:00", "yyyy-MM-dd HH:mm:ss");
    firstTime = baseTime;
    for (int i = 0; i < entryCount; ++i)
    {
        QString msg = entryTemplate.arg(i);
        QDateTime entryTime = baseTime.addSecs(i);
        out << entryTime.toString("yyyy-MM-dd HH:mm:ss.zzz")
            << ";1;1;mod;info;" << msg << "\n";
        lastTime = entryTime;
    }
    buffer.close();

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

    logService->openBuffer(data, "test.csv", QStringList() << "TestFormat");
    logService->createSession(logService->getLogManager()->getModules(),
                              toTimePoint(firstTime),
                              toTimePoint(lastTime));
}

void LogModelTest::testInitialLoad()
{
    LogModel model(logService);
    QSignalSpy resetSpy(&model, &QAbstractItemModel::modelReset);
    model.goToTime(firstTime);

    resetSpy.wait(10 * 1000);
    QVERIFY(!resetSpy.empty());

    QCOMPARE(model.rowCount(), blockSize);

    QModelIndex moduleIndex = model.index(0, static_cast<int>(LogModel::PredefinedColumn::Module));
    QCOMPARE(model.data(moduleIndex).toString(), QString("test"));

    for (int i = 0; i < model.rowCount(); ++i)
    {
        QModelIndex messageIndex = model.index(i, 6);
        QString expectedMessage = entryTemplate.arg(i);
        QCOMPARE(model.data(messageIndex).toString(), expectedMessage);
    }
}

void LogModelTest::testAvailableModules()
{
    LogModel model(logService);
    auto modules = model.availableValues(
        static_cast<int>(LogModel::PredefinedColumn::Module));
    QVERIFY(modules.find(QString("test")) != modules.end());
}

void LogModelTest::testHeaderData()
{
    LogModel model(logService);
    QCOMPARE(model.headerData(static_cast<int>(LogModel::PredefinedColumn::Module), Qt::Horizontal).toString(), QStringLiteral("module"));
}

void LogModelTest::testLoadMultipleBlocks()
{
    LogModel model(logService);
    QSignalSpy resetSpy(&model, &QAbstractItemModel::modelReset);

    model.goToTime(std::chrono::system_clock::time_point::max());

    resetSpy.wait(10 * 1000);
    QVERIFY(!resetSpy.empty());

    QCOMPARE(model.rowCount(), blockSize);

    int entryIndex = entryCount - model.rowCount();
    for (int i = 0; i < model.rowCount(); ++i)
    {
        QModelIndex messageIndex = model.index(i, 6);
        QString expectedMessage = entryTemplate.arg(entryIndex + i);
        QCOMPARE(model.data(messageIndex).toString(), expectedMessage);
    }

    model.fetchUpMore();

    resetSpy.wait(10 * 1000);
    QVERIFY(!resetSpy.empty());

    QCOMPARE(model.rowCount(), blockSize * 2);
    QVERIFY(model.isFulled());

    entryIndex = entryCount - model.rowCount();
    for (int i = 0; i < model.rowCount(); ++i)
    {
        QModelIndex messageIndex = model.index(i, 6);
        QString expectedMessage = entryTemplate.arg(entryIndex + i);
        QCOMPARE(model.data(messageIndex).toString(), expectedMessage);
    }

    model.fetchUpMore();

    resetSpy.wait(10 * 1000);
    QVERIFY(!resetSpy.empty());

    QCOMPARE(model.rowCount(), blockSize * 2);
    QVERIFY(model.isFulled());

    entryIndex = entryCount - model.rowCount() - blockSize;
    for (int i = 0; i < model.rowCount(); ++i)
    {
        QModelIndex messageIndex = model.index(i, 6);
        QString expectedMessage = entryTemplate.arg(entryIndex + i);
        QCOMPARE(model.data(messageIndex).toString(), expectedMessage);
    }
}

void LogModelTest::testFetchDownMore()
{
    LogModel model(logService);
    QSignalSpy resetSpy(&model, &QAbstractItemModel::modelReset);

    model.goToTime(firstTime);

    resetSpy.wait(10 * 1000);
    QVERIFY(!resetSpy.empty());

    QCOMPARE(model.rowCount(), blockSize);

    int entryIndex = 0;
    for (int i = 0; i < model.rowCount(); ++i)
    {
        QModelIndex messageIndex = model.index(i, 6);
        QString expectedMessage = entryTemplate.arg(entryIndex + i);
        QCOMPARE(model.data(messageIndex).toString(), expectedMessage);
    }

    model.fetchDownMore();

    resetSpy.wait(10 * 1000);
    QVERIFY(!resetSpy.empty());

    QCOMPARE(model.rowCount(), blockSize * 2);
    QVERIFY(model.isFulled());

    entryIndex = 0;
    for (int i = 0; i < model.rowCount(); ++i)
    {
        QModelIndex messageIndex = model.index(i, 6);
        QString expectedMessage = entryTemplate.arg(entryIndex + i);
        QCOMPARE(model.data(messageIndex).toString(), expectedMessage);
    }

    model.fetchDownMore();

    resetSpy.wait(10 * 1000);
    QVERIFY(!resetSpy.empty());

    QCOMPARE(model.rowCount(), blockSize * 2);
    QVERIFY(model.isFulled());

    entryIndex = blockSize;
    for (int i = 0; i < model.rowCount(); ++i)
    {
        QModelIndex messageIndex = model.index(i, 6);
        QString expectedMessage = entryTemplate.arg(entryIndex + i);
        QCOMPARE(model.data(messageIndex).toString(), expectedMessage);
    }
}

int main(int argc, char **argv)
{
    Application app(argc, argv);
    LogModelTest tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "LogModelTest.moc"
