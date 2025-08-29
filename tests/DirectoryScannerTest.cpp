#include <QtTest/QtTest>
#include <memory>
#include <unordered_map>

#include "Application.h"
#include "LogManagement/DirectoryScanner.h"

class DirectoryScannerTest : public QObject
{
    Q_OBJECT

private slots:
    void testSingleFile();
    void testConflictingFiles();
};

void DirectoryScannerTest::testSingleFile()
{
    DirectoryScanner scanner;

    LogMetadata meta;
    meta.format = std::make_shared<Format>();
    meta.filename = "dir1/file.log";
    meta.fileBuilder = [](const QString&, const std::shared_ptr<Format>&) { return std::shared_ptr<Log>(); };

    auto start = std::chrono::system_clock::time_point{std::chrono::milliseconds{0}};
    auto end = std::chrono::system_clock::time_point{std::chrono::milliseconds{100}};
    scanner.addFile("file", std::move(meta), start, end);

    auto files = scanner.scan();
    QCOMPARE(files.size(), 1);

    const auto& f = files.front();
    QCOMPARE(f.module, QString("file"));
    QCOMPARE(f.metadata.filename, QString("dir1/file.log"));
    QCOMPARE(f.start.time_since_epoch().count(), start.time_since_epoch().count());
    QCOMPARE(f.end.time_since_epoch().count(), end.time_since_epoch().count());
}

void DirectoryScannerTest::testConflictingFiles()
{
    DirectoryScanner scanner;

    LogMetadata meta1;
    meta1.format = std::make_shared<Format>();
    meta1.filename = "A/file.log";
    meta1.fileBuilder = [](const QString&, const std::shared_ptr<Format>&) { return std::shared_ptr<Log>(); };
    auto s1 = std::chrono::system_clock::time_point{std::chrono::milliseconds{0}};
    auto e1 = std::chrono::system_clock::time_point{std::chrono::milliseconds{1000}};
    scanner.addFile("file", std::move(meta1), s1, e1);

    LogMetadata meta2;
    meta2.format = std::make_shared<Format>();
    meta2.filename = "B/file.log";
    meta2.fileBuilder = [](const QString&, const std::shared_ptr<Format>&) { return std::shared_ptr<Log>(); };
    auto s2 = std::chrono::system_clock::time_point{std::chrono::milliseconds{500}};
    auto e2 = std::chrono::system_clock::time_point{std::chrono::milliseconds{1500}};
    scanner.addFile("file", std::move(meta2), s2, e2);

    auto files = scanner.scan();
    QCOMPARE(files.size(), 2);

    std::unordered_map<QString, DirectoryScanner::LogFile> map;
    for (const auto& f : files)
        map[f.module] = f;

    QVERIFY(map.contains("A/file"));
    QVERIFY(map.contains("B/file"));

    const auto& fA = map["A/file"];
    QCOMPARE(fA.metadata.filename, QString("A/file.log"));
    QCOMPARE(fA.start.time_since_epoch().count(), s1.time_since_epoch().count());

    const auto& fB = map["B/file"];
    QCOMPARE(fB.metadata.filename, QString("B/file.log"));
    QCOMPARE(fB.start.time_since_epoch().count(), s2.time_since_epoch().count());
}

int main(int argc, char** argv)
{
    Application app(argc, argv);
    DirectoryScannerTest tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "DirectoryScannerTest.moc"

