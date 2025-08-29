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
    void testComplexFileStructure1();
    void testComplexFileStructure2();

private:
    void addFile(DirectoryScanner& scanner, const QString& moduleName, const QString& filePath, const std::chrono::system_clock::time_point& start, const std::chrono::system_clock::time_point& end)
    {
        LogMetadata meta;
        meta.format = std::make_shared<Format>();
        meta.filename = filePath;
        meta.fileBuilder = [](const QString&, const std::shared_ptr<Format>&) { return std::shared_ptr<Log>(); };

        scanner.addFile(moduleName, std::move(meta), start, end);
    }
};

void DirectoryScannerTest::testSingleFile()
{
    DirectoryScanner scanner;

    auto start = std::chrono::system_clock::time_point{std::chrono::milliseconds{0}};
    auto end = std::chrono::system_clock::time_point{std::chrono::milliseconds{100}};
    addFile(scanner, "file", "dir1/file.log", start, end);

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

    auto s1 = std::chrono::system_clock::time_point{std::chrono::milliseconds{0}};
    auto e1 = std::chrono::system_clock::time_point{std::chrono::milliseconds{1000}};
    addFile(scanner, "file", "A/file.log", s1, e1);

    auto s2 = std::chrono::system_clock::time_point{std::chrono::milliseconds{500}};
    auto e2 = std::chrono::system_clock::time_point{std::chrono::milliseconds{1500}};
    addFile(scanner, "file", "B/file.log", s2, e2);

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

void DirectoryScannerTest::testComplexFileStructure1()
{
    DirectoryScanner scanner;

    auto s1 = std::chrono::system_clock::time_point{std::chrono::milliseconds{0}};
    auto e1 = std::chrono::system_clock::time_point{std::chrono::milliseconds{1000}};
    addFile(scanner, "file", "A/1/2/3/file.log", s1, e1);

    auto s2 = std::chrono::system_clock::time_point{std::chrono::milliseconds{500}};
    auto e2 = std::chrono::system_clock::time_point{std::chrono::milliseconds{1500}};
    addFile(scanner, "file", "B/1/2/3/file.log", s2, e2);

    auto s3 = std::chrono::system_clock::time_point{std::chrono::milliseconds{2000}};
    auto e3 = std::chrono::system_clock::time_point{std::chrono::milliseconds{3000}};
    addFile(scanner, "file", "C/1/2/3/file.log", s3, e3);

    auto files = scanner.scan();
    QCOMPARE(files.size(), 3);

    std::unordered_map<QString, DirectoryScanner::LogFile> map;
    for (const auto& f : files)
        map[f.module] = f;

    QVERIFY(map.contains("A/file"));
    QVERIFY(map.contains("B/file"));
    QVERIFY(map.contains("C/file"));

    const auto& fA = map["A/file"];
    QCOMPARE(fA.metadata.filename, QString("A/1/2/3/file.log"));
    QCOMPARE(fA.start.time_since_epoch().count(), s1.time_since_epoch().count());

    const auto& fB = map["B/file"];
    QCOMPARE(fB.metadata.filename, QString("B/1/2/3/file.log"));
    QCOMPARE(fB.start.time_since_epoch().count(), s2.time_since_epoch().count());

    const auto& fC = map["C/file"];
    QCOMPARE(fC.metadata.filename, QString("C/1/2/3/file.log"));
    QCOMPARE(fC.start.time_since_epoch().count(), s3.time_since_epoch().count());
}

void DirectoryScannerTest::testComplexFileStructure2()
{
    DirectoryScanner scanner;

    auto s1 = std::chrono::system_clock::time_point{std::chrono::milliseconds{0}};
    auto e1 = std::chrono::system_clock::time_point{std::chrono::milliseconds{1000}};
    addFile(scanner, "", "A/1/2/3/file.log", s1, e1);

    auto s2 = std::chrono::system_clock::time_point{std::chrono::milliseconds{500}};
    auto e2 = std::chrono::system_clock::time_point{std::chrono::milliseconds{1500}};
    addFile(scanner, "", "B/1/2/3/file.log", s2, e2);

    auto s3 = std::chrono::system_clock::time_point{std::chrono::milliseconds{2000}};
    auto e3 = std::chrono::system_clock::time_point{std::chrono::milliseconds{3000}};
    addFile(scanner, "", "C/1/X/3/file.log", s3, e3);

    auto s4 = std::chrono::system_clock::time_point{std::chrono::milliseconds{500}};
    auto e4 = std::chrono::system_clock::time_point{std::chrono::milliseconds{1000}};
    addFile(scanner, "", "C/1/Y/3/file.log", s4, e4);

    auto files = scanner.scan();
    QCOMPARE(files.size(), 4);

    std::unordered_map<QString, DirectoryScanner::LogFile> map;
    for (const auto& f : files)
        map[f.metadata.filename] = f;

    QVERIFY(map.contains("A/1/2/3/file.log"));
    QVERIFY(map.contains("B/1/2/3/file.log"));
    QVERIFY(map.contains("C/1/X/3/file.log"));
    QVERIFY(map.contains("C/1/Y/3/file.log"));

    const auto& fA = map["A/1/2/3/file.log"];
    QCOMPARE(fA.metadata.filename, QString("A/1/2/3/file.log"));
    QCOMPARE(fA.start.time_since_epoch().count(), s1.time_since_epoch().count());

    const auto& fB = map["B/1/2/3/file.log"];
    QCOMPARE(fB.metadata.filename, QString("B/1/2/3/file.log"));
    QCOMPARE(fB.start.time_since_epoch().count(), s2.time_since_epoch().count());

    const auto& fC = map["C/1/X/3/file.log"];
    QCOMPARE(fC.metadata.filename, QString("C/1/X/3/file.log"));
    QCOMPARE(fC.start.time_since_epoch().count(), s3.time_since_epoch().count());

    const auto& fD = map["C/1/Y/3/file.log"];
    QCOMPARE(fD.metadata.filename, QString("C/1/Y/3/file.log"));
    QCOMPARE(fD.start.time_since_epoch().count(), s4.time_since_epoch().count());
}

int main(int argc, char** argv)
{
    Application app(argc, argv);
    DirectoryScannerTest tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "DirectoryScannerTest.moc"

