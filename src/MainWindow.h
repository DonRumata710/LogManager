#pragma once

#include "LogManagement/FormatManager.h"
#include "LogView/LogModel.h"
#include "SearchController.h"

#include <QMainWindow>
#include <QProgressBar>
#include <QAbstractItemModel>
#include <QTreeView>
#include <QDateTime>


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
    void on_actionAdd_format_triggered();
    void on_actionRemove_format_triggered();
    void on_actionRefresh_format_triggered();
    void on_actionSelect_all_triggered();
    void on_actionDeselect_all_triggered();

    void on_actionExport_as_is_triggered();

    void on_actionFull_export_triggered();
    void on_actionFiltered_export_triggered();

    void on_actionShow_bookmarks_triggered();

    void logManagerCreated(const QString& source);

    void handleProgress(const QString& message, int percent);

    void handleError(const QString& message);

signals:
    void openFile(const QString& file, const QStringList& formats);
    void openFolder(const QString& logDirectory, const QStringList& formats);

    void exportData(const QString& filename, const QDateTime& startTime, const QDateTime& endTime);
    void exportData(const QString& filename, const QDateTime& startTime, const QDateTime& endTime, const QStringList& fields);
    void exportData(const QString& filename, const QDateTime& startTime, const QDateTime& endTime, const QStringList& fields, const LogFilter& filter);

    void exportData(const QString& filename, QTreeView* view);

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

private:
    Ui::MainWindow *ui;
    FormatManager& formatManager;

    std::vector<QAction*> formatActions;

    QStringList selectedFormats;

    QProgressBar* progressBar = nullptr;

    SearchController* searchController;
};
