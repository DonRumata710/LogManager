#pragma once

#include "LogManagement/FormatManager.h"
#include "LogManagement/LogManager.h"
#include "LogService.h"

#include <QMainWindow>
#include <QAbstractItemModel>


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
    void on_actionSelect_all_triggered();
    void on_actionDeselect_all_triggered();

    void checkFetchNeeded();

    void on_actionExport_as_is_triggered();

    void on_actionFull_export_triggered();

    void logManagerCreated();

signals:
    void openFile(const QString& file, const QStringList& formats);
    void openFolder(const QString& logDirectory, const QStringList& formats);

private:
    void addFormat(const std::string& format);

    void checkActions();
    void setLogActionsEnabled(bool enabled);
    void setCloseActionEnabled(bool enabled);
    void updateFormatActions(bool enabled);

    void switchModel(QAbstractItemModel* model);

private:
    Ui::MainWindow *ui;
    FormatManager& formatManager;

    std::vector<QAction*> formatActions;

    QStringList selectedFormats;
};
