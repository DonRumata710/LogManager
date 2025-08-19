#include <QSignalSpy>
#include <QBuffer>
#include <QTextStream>
#include <QStandardItemModel>
#include <QtTest/QtTest>

#include "Application.h"
#include "LogService.h"
#include "LogView/LogFilterModel.h"
#include "LogView/LogModel.h"
#include "LogView/FilteredLogModel.h"
#include "Settings.h"
#include "LogView/LogViewUtils.h"

static std::chrono::system_clock::time_point toTimePoint(const QDateTime &dt)
{
    return std::chrono::system_clock::time_point{ std::chrono::milliseconds{ dt.toMSecsSinceEpoch() } };
}
class LogFilterModelTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testFilterWildcard();
    void testVariantList();
    void testFilteredModelCreated();

private:
    QString getModule(int);

private:
    Application *app = nullptr;
    LogService *logService = nullptr;
    LogModel *model = nullptr;
    QDateTime firstTime;
    QString entryTemplate = "msg%1";
    int entryCount = 10;
};

void LogFilterModelTest::initTestCase()
{
    app = qobject_cast<Application *>(qApp);
    QVERIFY(app);
    logService = app->getLogService();

    Settings settings;
    settings.setValue(LogViewSettings + "/blockSize", entryCount);
    settings.setValue(LogViewSettings + "/blockCount", 2);

    QByteArray data;
    QBuffer buffer(&data);
    QVERIFY(buffer.open(QIODevice::WriteOnly));
    QTextStream out(&buffer);

    QDateTime baseTime = QDateTime::fromString("2023-01-01 00:00:00", "yyyy-MM-dd HH:mm:ss");
    firstTime = baseTime;
    QDateTime lastTime;
    for (int i = 0; i < entryCount * 10; ++i)
    {
        QString msg = entryTemplate.arg(i);
        QString module = getModule(i);
        QDateTime entryTime = baseTime.addSecs(i);
        out << entryTime.toString("yyyy-MM-dd HH:mm:ss.zzz")
            << ';' << module << ";1;1;info;" << msg << "\n";
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

    model = new LogModel(logService, this);
    QSignalSpy resetSpy(model, &QAbstractItemModel::modelReset);
    model->goToTime(firstTime);
    QVERIFY(resetSpy.wait(10 * 1000));
    QCOMPARE(model->rowCount(), entryCount);

    QSignalSpy dataSpy(model, &QAbstractItemModel::rowsInserted);
    QVERIFY(dataSpy.wait(10 * 1000));
}

void LogFilterModelTest::testFilterWildcard()
{
    QStandardItemModel source(entryCount, 7);
    source.setHeaderData(6, Qt::Horizontal, QStringLiteral("message"));
    for (int i = 0; i < entryCount; ++i)
        source.setData(source.index(i, 6), entryTemplate.arg(i));

    LogFilterModel filterModel;
    filterModel.setSourceModel(&source);
    filterModel.setFilterWildcard(6, "*1");

    QCOMPARE(filterModel.rowCount(), 1);
    QModelIndex idx = filterModel.index(0, 6);
    QCOMPARE(filterModel.data(idx).toString(), entryTemplate.arg(1));
}

void LogFilterModelTest::testVariantList()
{
    QStandardItemModel source(entryCount, 7);
    source.setHeaderData(2, Qt::Horizontal, QStringLiteral("field1"));
    for (int i = 0; i < entryCount; ++i)
    {
        QString module = getModule(i);
        source.setData(source.index(i, 2), module);
    }

    LogFilterModel filterModel;
    filterModel.setSourceModel(&source);
    filterModel.setVariantList(2, QStringList() << "modA");

    int expected = (entryCount + 9) / 10;
    QCOMPARE(filterModel.rowCount(), expected);
    for (int i = 0; i < filterModel.rowCount(); ++i)
    {
        QModelIndex idx = filterModel.index(i, 2);
        QCOMPARE(filterModel.data(idx).toString(), QStringLiteral("modA"));
    }

    LogFilter filter = filterModel.exportFilter();
    LogEntry okEntry; okEntry.values.emplace("field1", QStringLiteral("modA"));
    LogEntry badEntry; badEntry.values.emplace("field1", QStringLiteral("modB"));
    QVERIFY(filter.check(okEntry));
    QVERIFY(!filter.check(badEntry));
}

void LogFilterModelTest::testFilteredModelCreated()
{
    LogFilterModel filterModel;
    filterModel.setSourceModel(model);
    QSignalSpy modelSpy(&filterModel, &LogFilterModel::sourceModelChanged);

    filterModel.setVariantList(2, QStringList() << "modA");

    QCOMPARE(modelSpy.count(), 1);
    QVERIFY(qobject_cast<FilteredLogModel*>(filterModel.sourceModel()));

    modelSpy.wait(10 * 1000);

    QCOMPARE(filterModel.rowCount(), entryCount * 2);
}

QString LogFilterModelTest::getModule(int i)
{
    switch (i % 10)
    {
    case 0: return "modA";
    case 1: return "modB";
    case 2: return "modC";
    case 3: return "modD";
    case 4: return "modE";
    case 5: return "modF";
    case 6: return "modG";
    case 7: return "modH";
    case 8: return "modI";
    case 9: return "modJ";
    default: return QString();
    }
}

int main(int argc, char **argv)
{
    Application app(argc, argv);
    LogFilterModelTest tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "LogFilterModelTest.moc"
