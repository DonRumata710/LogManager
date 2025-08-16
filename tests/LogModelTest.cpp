#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTextStream>
#include <QtTest/QtTest>

#include "Application.h"
#include "LogService.h"
#include "LogView/LogModel.h"
#include "LogView/LogViewUtils.h"
#include "Settings.h"

class LogModelTest : public QObject {
  Q_OBJECT

private slots:
  void initTestCase();
  void cleanupTestCase();

  void testInitialLoad();
  void testAvailableModules();
  void testHeaderData();
  void testLoadMultipleBlocks();

private:
  Application *app = nullptr;
  LogService *logService = nullptr;
  QTemporaryDir *tempDir = nullptr;
  QString logFile;
  QDateTime firstTime;
  QDateTime lastTime;
};

void LogModelTest::initTestCase() {
  app = qobject_cast<Application *>(qApp);
  QVERIFY(app);
  logService = app->getLogService();

  tempDir = new QTemporaryDir();
  logFile = tempDir->filePath("test.log.csv");

  Settings settings;
  settings.setValue(LogViewSettings + "/blockSize", 2);

  QFile file(logFile);
  QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
  QTextStream out(&file);

  QDateTime baseTime =
      QDateTime::fromString("2023-01-01 00:00:00", "yyyy-MM-dd HH:mm:ss");
  for (int i = 0; i < 5; ++i) {
    QString msg;
    if (i == 0)
      msg = "hello";
    else if (i == 1)
      msg = "searchterm";
    else if (i == 4)
      msg = "tail";
    else
      msg = QStringLiteral("msg%1").arg(i);

    out << baseTime.addSecs(i).toString("yyyy-MM-dd HH:mm:ss.zzz")
        << ";1;1;mod;info;" << msg << "\n";
  }
  file.close();

  firstTime = baseTime;
  lastTime = baseTime.addSecs(4);

  std::shared_ptr<Format> format = std::make_shared<Format>();
  format->name = "TestFormat";
  format->extension = ".csv";
  format->separator = ";";
  format->timeFieldIndex = 0;
  format->timeMask = "%F %H:%M:%S";
  format->timeFractionalDigits = 3;
  for (int i = 0; i < 6; ++i) {
    Format::Field f;
    f.name = QString::number(i);
    f.regex = QRegularExpression(".*");
    f.type = QMetaType::QString;
    format->fields.push_back(f);
  }
  app->getFormatManager().addFormat(format);

  logService->openFile(logFile, QStringList() << "TestFormat");
  logService->createSession(logService->getLogManager()->getModules(),
                            firstTime.toStdSysMilliseconds(),
                            lastTime.toStdSysMilliseconds());
}

void LogModelTest::cleanupTestCase() { delete tempDir; }

void LogModelTest::testInitialLoad() {
  LogModel model(logService);
  QSignalSpy resetSpy(&model, &QAbstractItemModel::modelReset);
  model.goToTime(firstTime);
  QVERIFY(resetSpy.wait(1000));

  QCOMPARE(model.rowCount(), 2);

  QModelIndex moduleIndex =
      model.index(0, static_cast<int>(LogModel::PredefinedColumn::Module));
  QCOMPARE(model.data(moduleIndex).toString(), QString("test"));

  QModelIndex messageIndex = model.index(0, 6);
  QCOMPARE(model.data(messageIndex).toString(), QString("hello"));
}

void LogModelTest::testAvailableModules() {
  LogModel model(logService);
  auto modules = model.availableValues(
      static_cast<int>(LogModel::PredefinedColumn::Module));
  QVERIFY(modules.find(QString("test")) != modules.end());
}

void LogModelTest::testHeaderData() {
  LogModel model(logService);
  QCOMPARE(model
               .headerData(static_cast<int>(LogModel::PredefinedColumn::Module),
                           Qt::Horizontal)
               .toString(),
           QStringLiteral("module"));
}

void LogModelTest::testLoadMultipleBlocks() {
  LogModel model(logService);
  QSignalSpy resetSpy(&model, &QAbstractItemModel::modelReset);
  model.goToTime(lastTime);
  QVERIFY(resetSpy.wait(1000));

  QCOMPARE(model.rowCount(), 2);

  QModelIndex messageIndex = model.index(model.rowCount() - 1, 6);
  QCOMPARE(model.data(messageIndex).toString(), QString("tail"));
}

int main(int argc, char **argv) {
  Application app(argc, argv);
  LogModelTest tc;
  return QTest::qExec(&tc, argc, argv);
}

#include "LogModelTest.moc"
