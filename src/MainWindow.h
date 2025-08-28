#pragma once

#include "LogManagement/FormatManager.h"
#include "LogView/LogModel.h"
#include "SearchController.h"
#include "Statistics/LogHistogram.h"

#include <QMainWindow>
#include <QProgressBar>
#include <QAbstractItemModel>
#include <QTreeView>
#include <QStringList>
#include <chrono>


class SearchBar;
class BookmarkTable;
class SearchResultsWidget;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_actionOpen_folder_triggered();
    void on_actionOpen_file_triggered();
    void on_actionClose_triggered();
    void on_actionTimeline_triggered();
    void on_actionAdd_format_triggered();
    void on_actionRemove_format_triggered();
    void on_actionRefresh_format_triggered();
    void on_actionSelect_all_triggered();
    void on_actionDeselect_all_triggered();

    void on_actionExport_current_view_triggered();
    void on_actionFull_export_triggered();

    void on_actionShow_bookmarks_triggered();

    void logManagerCreated(const QString& source);

    void timelineReady(std::vector<Statistics::Bucket> data);

    void handleProgress(const QString& message, int percent);

    void handleError(const QString& message);

    void openRecent();

signals:
    void openFile(const QString& file, const QStringList& formats);
    void openFolder(const QString& logDirectory, const QStringList& formats);

    void exportData(const QString& folder, const std::chrono::system_clock::time_point& startTime, const std::chrono::system_clock::time_point& endTime);
    void exportData(const QString& folder, const std::chrono::system_clock::time_point& startTime, const std::chrono::system_clock::time_point& endTime, const QStringList& fields, const LogFilter& filter);
    void exportDataToTable(const QString& filename, const std::chrono::system_clock::time_point& startTime, const std::chrono::system_clock::time_point& endTime, const QStringList& fields);
    void exportDataToTable(const QString& filename, const std::chrono::system_clock::time_point& startTime, const std::chrono::system_clock::time_point& endTime, const QStringList& fields, const LogFilter& filter);

    void exportData(const QString& filename, QTreeView* view);

    void openTimeline(const std::chrono::system_clock::time_point& start, const std::chrono::system_clock::time_point& end);

private:
    void addFormat(const std::string& format);

    void checkActions();
    void setLogActionsEnabled(bool enabled);
    void setCloseActionEnabled(bool enabled);
    void updateFormatActions(bool enabled);

    void switchModel(QAbstractItemModel* model);
    void updateBookmarks();

    LogModel* getLogModel();

    void setTitleOpened(const QString& source);
    void setTitleClosed();

    void addRecentItem(const QString& path);
    void updateRecentMenu();

private:
    Ui::MainWindow *ui;
    FormatManager& formatManager;

    std::vector<QAction*> formatActions;

    QStringList selectedFormats;

    QStringList recentItems;

    QProgressBar* progressBar = nullptr;

    SearchController* searchController;

    SearchBar* searchBar = nullptr;
    BookmarkTable* bookmarkTable = nullptr;
    SearchResultsWidget* searchResults = nullptr;
};
