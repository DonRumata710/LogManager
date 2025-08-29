#include "MainWindow.h"
#include "ui_MainWindow.h"

#include "Application.h"
#include "Utils.h"
#include "InitialDataDialog.h"
#include "Settings.h"
#include "LogView/LogModel.h"
#include "LogView/LogFilterModel.h"
#include "LogView/LogView.h"
#include "FormatCreation/FormatCreationWizard.h"
#include "TimelineDialog.h"
#include "TimeFrameDialog.h"
#include "services/SessionService.h"
#include "services/SearchService.h"
#include "services/ExportService.h"
#include "services/TimelineService.h"
#include "SearchBarDockWidget.h"
#include "BookmarkTable.h"
#include "ExportSettingsDialog.h"
#include "SearchResultsWidget.h"

#include <utility>
#include <QScrollBar>
#include <QFileDialog>
#include <QInputDialog>
#include <QProgressBar>
#include <QMessageBox>
#include <QMap>
#include <QFileInfo>
#include <QMenu>


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    formatManager(qobject_cast<Application*>(QApplication::instance())->getFormatManager())
{
    ui->setupUi(this);

    setTitleClosed();

    progressBar = new QProgressBar(this);
    progressBar->setVisible(false);
    progressBar->setMinimum(0);
    progressBar->setMaximum(100);
    progressBar->setTextVisible(true);
    statusBar()->addPermanentWidget(progressBar);

    searchBar = new SearchBarDockWidget(this);
    searchBar->setWindowTitle(tr("Search"));
    searchBar->hide();

    searchResults = new SearchResultsWidget(this);
    searchResults->setWindowTitle(tr("Search Results"));
    searchResults->hide();

    addDockWidget(Qt::BottomDockWidgetArea, searchBar);
    splitDockWidget(searchBar, searchResults, Qt::Vertical);

    bookmarkTable = new BookmarkTable(this);
    bookmarkTable->setWindowTitle(tr("Bookmarks"));
    bookmarkTable->hide();
    addDockWidget(Qt::TopDockWidgetArea, bookmarkTable);
    connect(bookmarkTable, &BookmarkTable::bookmarkActivated, ui->logView, &LogView::bookmarkActivated);

    searchController = new SearchController(searchBar->getSearchBar(), ui->logView, this);
    connect(searchController, &SearchController::searchResults, searchResults, &SearchResultsWidget::showResults);

    loadSettings();

    for (const auto& format : formatManager.getFormats())
        addFormat(format.first);

    setLogActionsEnabled(!selectedFormats.empty());

    updateRecentMenu();

    auto app = qobject_cast<Application*>(QApplication::instance());
    auto sessionService = app->getSessionService();
    auto searchService = app->getSearchService();
    auto exportService = app->getExportService();
    auto timelineService = app->getTimelineService();

    connect(this, &MainWindow::openFile, sessionService, &SessionService::openFile);
    connect(this, &MainWindow::openFolder, sessionService, &SessionService::openFolder);
    connect(sessionService, &SessionService::logManagerCreated, this, &MainWindow::logManagerCreated);

    connect(sessionService, &SessionService::progressUpdated, this, &MainWindow::handleProgress);
    connect(searchService, &SearchService::progressUpdated, this, &MainWindow::handleProgress);
    connect(exportService, &ExportService::progressUpdated, this, &MainWindow::handleProgress);
    connect(timelineService, &TimelineService::progressUpdated, this, &MainWindow::handleProgress);

    connect(sessionService, &SessionService::handleError, this, &MainWindow::handleError);
    connect(searchService, &SearchService::handleError, this, &MainWindow::handleError);
    connect(exportService, &ExportService::handleError, this, &MainWindow::handleError);
    connect(timelineService, &TimelineService::handleError, this, &MainWindow::handleError);

    connect(this,
            qOverload<const QString&, const std::chrono::system_clock::time_point&, const std::chrono::system_clock::time_point&>(&MainWindow::exportData),
            exportService,
            qOverload<const QString&, const std::chrono::system_clock::time_point&, const std::chrono::system_clock::time_point&>(&ExportService::exportData));
    connect(this,
            qOverload<const QString&, const std::chrono::system_clock::time_point&, const std::chrono::system_clock::time_point&, const QStringList&, const LogFilter&>(&MainWindow::exportData),
            exportService,
            qOverload<const QString&, const std::chrono::system_clock::time_point&, const std::chrono::system_clock::time_point&, const QStringList&, const LogFilter&>(&ExportService::exportData));
    connect(this,
            qOverload<const QString&, const std::chrono::system_clock::time_point&, const std::chrono::system_clock::time_point&, const QStringList&>(&MainWindow::exportDataToTable),
            exportService,
            qOverload<const QString&, const std::chrono::system_clock::time_point&, const std::chrono::system_clock::time_point&, const QStringList&>(&ExportService::exportDataToTable));
    connect(this,
            qOverload<const QString&, const std::chrono::system_clock::time_point&, const std::chrono::system_clock::time_point&, const QStringList&, const LogFilter&>(&MainWindow::exportDataToTable),
            exportService,
            qOverload<const QString&, const std::chrono::system_clock::time_point&, const std::chrono::system_clock::time_point&, const QStringList&, const LogFilter&>(&ExportService::exportDataToTable));
    connect(this,
            qOverload<const QString&, QTreeView*>(&MainWindow::exportData),
            exportService,
            qOverload<const QString&, QTreeView*>(&ExportService::exportData));

    connect(this, &MainWindow::openTimeline, timelineService, &TimelineService::showTimeline);
    connect(timelineService, &TimelineService::timelineReady, this, &MainWindow::timelineReady);

    connect(searchBar->getSearchBar(), &SearchBar::handleError, this, &MainWindow::handleError);
    connect(ui->logView, &LogView::handleError, this, &MainWindow::handleError);
    connect(searchResults, &SearchResultsWidget::handleError, this, &MainWindow::handleError);
}

MainWindow::~MainWindow()
{
    Settings settings;
    settings.setValue(objectName() + "/geometry", saveGeometry());
    settings.setValue(objectName() + "/state", saveState());

    delete ui;
}

void MainWindow::on_actionOpen_folder_triggered()
{
    QT_SLOT_BEGIN

    QString folderPath = QFileDialog::getExistingDirectory(this, tr("Open Folder"), QDir::currentPath());
    if (folderPath.isEmpty())
        return;

    openFolder(folderPath, selectedFormats);

    addRecentItem(folderPath);

    QT_SLOT_END
}

void MainWindow::on_actionOpen_file_triggered()
{
    QT_SLOT_BEGIN

    QString extensions;
    QMap<QString, QStringList> extToFormatNames;

    for (const auto& formatName : std::as_const(selectedFormats))
    {
        auto format = formatManager.getFormats().at(formatName.toStdString());
        if (!format)
            continue;

        extToFormatNames[format->extension].append(format->name);
    }

    QStringList filters;
    for (auto it = extToFormatNames.constBegin(); it != extToFormatNames.constEnd(); ++it)
    {
        QStringList names = it.value();
        QString ext = it.key();
        QString combinedName = names.join(", ");
        filters << QString("%1 (*%2)").arg(combinedName, ext);
    }

    filters << tr("Archive files (*.zip)");

    QString extensionsString = filters.join(";;");

    QString file = QFileDialog::getOpenFileName(this, tr("Open File"), QDir::currentPath(), extensionsString);
    if (file.isEmpty())
        return;

    QString extension = file.mid(file.lastIndexOf('.'));
    QStringList formats;
    if (extension == ".zip")
        formats = selectedFormats;
    else
        formats = extToFormatNames[extension];
    openFile(file, formats);

    addRecentItem(file);

    QT_SLOT_END
}

void MainWindow::on_actionClose_triggered()
{
    QT_SLOT_BEGIN

    switchModel(nullptr);
    setCloseActionEnabled(false);
    searchBar->hide();
    bookmarkTable->clearBookmarks();
    bookmarkTable->hide();
    searchResults->hide();
    ui->actionShow_bookmarks->setChecked(false);
    setTitleClosed();

    QT_SLOT_END
}

void MainWindow::on_actionTimeline_triggered()
{
    QT_SLOT_BEGIN

    auto app = qobject_cast<Application*>(QApplication::instance());
    auto sessionService = app->getSessionService();
    auto sessionPtr = sessionService->getSession();
    if (!sessionPtr)
        return;

    QDateTime start = DateTimeFromChronoSystemClock(sessionPtr->getMinTime());
    QDateTime end = DateTimeFromChronoSystemClock(sessionPtr->getMaxTime());

    TimeFrameDialog frameDialog(start, end, this);
    if (frameDialog.exec() != QDialog::Accepted)
        return;

    emit openTimeline(ChronoSystemClockFromDateTime(frameDialog.startDateTime()),
                      ChronoSystemClockFromDateTime(frameDialog.endDateTime()));

    QT_SLOT_END
}

void MainWindow::on_actionAdd_format_triggered()
{
    QT_SLOT_BEGIN

    FormatCreationWizard wizard(this);
    connect(&wizard, &FormatCreationWizard::handleError, this, &MainWindow::handleError);
    if (wizard.exec() != QDialog::Accepted)
        return;

    auto newFormat = std::make_shared<Format>(wizard.getFormat());
    addFormat(newFormat->name.toStdString());
    formatManager.addFormat(std::move(newFormat));

    QT_SLOT_END
}


void MainWindow::on_actionRemove_format_triggered()
{
    QT_SLOT_BEGIN

    QInputDialog dialog(this);

    QStringList items;
    for (const auto& format : formatManager.getFormats())
        items.append(QString::fromStdString(format.first));
    dialog.setComboBoxItems(items);

    dialog.setWindowTitle(tr("Remove Format"));
    dialog.setLabelText(tr("Select format to remove:"));

    if (dialog.exec() != QDialog::Accepted)
        return;

    QString selectedFormat = dialog.textValue();
    formatManager.removeFormat(selectedFormat);
    for (const auto& action : formatActions)
    {
        if (action->text() == selectedFormat)
        {
            selectedFormats.remove(selectedFormats.indexOf(selectedFormat));
            ui->menuFormats->removeAction(action);
            formatActions.erase(std::remove(formatActions.begin(), formatActions.end(), action), formatActions.end());
            delete action;
            break;
        }
    }

    QT_SLOT_END
}

void MainWindow::on_actionRefresh_format_triggered()
{
    QT_SLOT_BEGIN

    QStringList selectedFormatsCopy = selectedFormats;

    formatManager.updateFormats();

    for (auto action : std::as_const(formatActions))
    {
        ui->menuFormats->removeAction(action);
        delete action;
    }
    formatActions.clear();
    selectedFormats.clear();

    for (const auto& format : formatManager.getFormats())
    {
        addFormat(format.first);
        if (!selectedFormatsCopy.contains(format.second->name))
        {
            auto action = formatActions.back();
            action->setChecked(false);
        }
    }

    QT_SLOT_END
}

void MainWindow::on_actionSelect_all_triggered()
{
    QT_SLOT_BEGIN

    updateFormatActions(true);

    QT_SLOT_END
}


void MainWindow::on_actionDeselect_all_triggered()
{
    QT_SLOT_BEGIN

    updateFormatActions(false);

    QT_SLOT_END
}

void MainWindow::on_actionExport_current_view_triggered()
{
    QT_SLOT_BEGIN

    QString fileName = QFileDialog::getSaveFileName(this, tr("Export Logs"), QDir::currentPath(), tr("CSV Files (*.csv)"));
    if (fileName.isEmpty())
        return;

    exportData(fileName, ui->logView);

    QT_SLOT_END
}

void MainWindow::on_actionFull_export_triggered()
{
    QT_SLOT_BEGIN

    ExportSettingsDialog settingsDialog(this);
    if (settingsDialog.exec() != QDialog::Accepted)
        return;

    QString fileName;
    QString folder;
    if (settingsDialog.csvFormat())
        fileName = QFileDialog::getSaveFileName(this, tr("Export Logs"), QDir::currentPath(), tr("CSV Files (*.csv)"));
    else
        folder = QFileDialog::getExistingDirectory(this, tr("Export Logs"), QDir::currentPath());

    if (fileName.isEmpty() && folder.isEmpty())
        return;

    auto logModel = getLogModel();
    if (!logModel)
    {
        qCritical() << "Unexpected model type in log view";
        return;
    }

    auto logFilterModel = qobject_cast<LogFilterModel*>(ui->logView->model());
    if (!logFilterModel && settingsDialog.useFilters())
    {
        qWarning() << "Log model is not set.";
        return;
    }

    if (settingsDialog.csvFormat())
    {
        if (settingsDialog.useFilters())
        {
            exportDataToTable(fileName,
                              ChronoSystemClockFromDateTime(logModel->getStartTime()),
                              ChronoSystemClockFromDateTime(logModel->getEndTime()),
                              logModel->getFieldsName());
        }
        else
        {
            exportDataToTable(fileName,
                              ChronoSystemClockFromDateTime(logModel->getStartTime()),
                              ChronoSystemClockFromDateTime(logModel->getEndTime()),
                              logModel->getFieldsName(),
                              logFilterModel->exportFilter());
        }
    }
    else
    {
        if (settingsDialog.useFilters())
        {
            exportData(fileName,
                       ChronoSystemClockFromDateTime(logModel->getStartTime()),
                       ChronoSystemClockFromDateTime(logModel->getEndTime()),
                       logModel->getFieldsName(),
                       logFilterModel->exportFilter());
        }
        else
        {
            exportData(fileName,
                       ChronoSystemClockFromDateTime(logModel->getStartTime()),
                       ChronoSystemClockFromDateTime(logModel->getEndTime()));
        }
    }

    QT_SLOT_END
}

void MainWindow::on_actionShow_bookmarks_triggered()
{
    QT_SLOT_BEGIN

    Settings settings;
    bool checked = ui->actionShow_bookmarks->isChecked();
    bookmarkTable->setVisible(checked);
    settings.setValue(objectName() + "/bookmarksVisible", checked);

    QT_SLOT_END
}

void MainWindow::on_actionShow_search_bar_triggered(bool checked)
{
    QT_SLOT_BEGIN

    searchBar->setVisible(checked);

    QT_SLOT_END
}

void MainWindow::logManagerCreated(const QString& source)
{
    QT_SLOT_BEGIN

    auto app = qobject_cast<Application*>(QApplication::instance());
    auto sessionService = app->getSessionService();

    auto logManager = sessionService->getLogManager();
    if (!sessionService->getLogManager())
    {
        qWarning() << "LogManager is not created.";
        return;
    }

    InitialDataDialog dialog(*logManager.get());
    connect(&dialog, &InitialDataDialog::handleError, this, &MainWindow::handleError);
    if (dialog.exec() != QDialog::Accepted)
        return;

    auto startDate = dialog.getStartDate();
    auto endDate = dialog.getEndDate();
    auto modules = dialog.getModules();
    auto scrollToEnd = dialog.scrollToEnd();

    if (startDate > endDate)
    {
        qWarning() << "Start date cannot be later than end date.";
        return;
    }

    if (modules.empty())
    {
        qWarning() << "No modules selected.";
        return;
    }

    sessionService->createSession(modules,
                                 ChronoSystemClockFromDateTime(startDate),
                                 ChronoSystemClockFromDateTime(endDate.addMSecs(1)));

    auto proxyModel = new LogFilterModel(this);
    connect(proxyModel, &LogFilterModel::handleError, this, &MainWindow::handleError);

    auto logModel = new LogModel(sessionService, proxyModel);
    connect(logModel, &LogModel::handleError, this, &MainWindow::handleError);
    connect(logModel, &LogModel::bookmarksChanged, this, &MainWindow::updateBookmarks);

    if (scrollToEnd)
    {
        logModel->goToTime(std::chrono::system_clock::time_point::max());
        ui->logView->scrollToBottom();
    }
    else
    {
        logModel->goToTime(std::chrono::system_clock::time_point::min());
    }

    proxyModel->setSourceModel(logModel);

    switchModel(proxyModel);
    setCloseActionEnabled(true);
    searchBar->getSearchBar()->handleColumnCountChanged(logModel->columnCount());
    searchBar->show();

    Settings settings;
    bool bookmarkTableVisible = settings.value(objectName() + "/bookmarksVisible", false).toBool();
    bookmarkTable->setVisible(bookmarkTableVisible);
    ui->actionShow_bookmarks->setChecked(bookmarkTableVisible);

    updateBookmarks();

    setTitleOpened(source);

    QT_SLOT_END
}

void MainWindow::timelineReady(std::vector<Statistics::Bucket> data)
{
    QT_SLOT_BEGIN

    TimelineDialog dialog(std::move(data), this);
    dialog.exec();

    QT_SLOT_END
}


void MainWindow::handleProgress(const QString& message, int percent)
{
    if (percent >= 100)
        statusBar()->showMessage(message, 2000);
    else
        statusBar()->showMessage(message);

    if (percent >= 0)
    {
        progressBar->setRange(0, 100);
        progressBar->setValue(percent);
    }
    else
    {
        progressBar->setRange(0, 0);
    }

    if (percent >= 100)
        progressBar->setVisible(false);
    else
        progressBar->setVisible(true);
}

void MainWindow::openRecent()
{
    QT_SLOT_BEGIN

        auto action = qobject_cast<QAction*>(sender());
    if (!action)
        return;

    QString path = action->data().toString();
    if (QFileInfo(path).isDir())
    {
        openFolder(path, selectedFormats);
    }
    else
    {
        QMap<QString, QStringList> extToFormatNames;
        for (const auto& formatName : std::as_const(selectedFormats))
        {
            auto format = formatManager.getFormats().at(formatName.toStdString());
            if (!format)
                continue;
            extToFormatNames[format->extension].append(format->name);
        }

        QString extension = path.mid(path.lastIndexOf('.'));
        QStringList formats;
        if (extension == ".zip")
            formats = selectedFormats;
        else
            formats = extToFormatNames.value(extension);

        openFile(path, formats);
    }

    addRecentItem(path);

    QT_SLOT_END
}

void MainWindow::handleError(const QString& message)
{
    statusBar()->showMessage(message, 5000);
    progressBar->setVisible(false);
    QMessageBox::critical(this, tr("Error"), message);
}

void MainWindow::addFormat(const std::string& format)
{
    QAction* action = formatActions.emplace_back(new QAction(QString::fromStdString(format), this));
    action->setCheckable(true);
    action->setChecked(true);
    QString formatName = QString::fromStdString(format);
    connect(action, &QAction::toggled, this, [this, formatName](bool checked) {
        if (checked)
            selectedFormats += formatName;
        else
            selectedFormats.remove(selectedFormats.indexOf(formatName));
        checkActions();
    });
    ui->menuFormats->addAction(action);

    selectedFormats += formatName;
    checkActions();
}

void MainWindow::checkActions()
{
    setLogActionsEnabled(!selectedFormats.empty());
}

void MainWindow::setLogActionsEnabled(bool enabled)
{
    ui->actionOpen_file->setEnabled(enabled);
    ui->actionOpen_folder->setEnabled(enabled);
}

void MainWindow::setCloseActionEnabled(bool enabled)
{
    ui->actionClose->setEnabled(enabled);
    ui->actionTimeline->setEnabled(enabled);
}

void MainWindow::updateFormatActions(bool enabled)
{
    for (const auto& action : formatActions)
        action->setChecked(enabled);
    checkActions();
}

void MainWindow::switchModel(QAbstractItemModel* model)
{
    auto oldModel = ui->logView->model();
    ui->logView->setLogModel(model);
    if (oldModel)
        delete oldModel;

    searchController->updateModel();
}

void MainWindow::updateBookmarks()
{
    auto logModel = getLogModel();
    if (!logModel)
    {
        bookmarkTable->clearBookmarks();
        return;
    }

    bookmarkTable->setBookmarks(logModel->getBookmarks());
}

LogModel* MainWindow::getLogModel()
{
    auto logModel = qobject_cast<LogModel*>(ui->logView->model());
    if (!logModel)
    {
        auto logFilterModel = qobject_cast<QSortFilterProxyModel*>(ui->logView->model());
        if (logFilterModel)
        {
            logModel = qobject_cast<LogModel*>(logFilterModel->sourceModel());
        }
    }
    return logModel;
}

void MainWindow::setTitleOpened(const QString& source)
{
    setWindowTitle(QApplication::instance()->applicationName() + " - " + source);
}

void MainWindow::setTitleClosed()
{
    setWindowTitle(QApplication::instance()->applicationName());
}

void MainWindow::addRecentItem(const QString& path)
{
    Settings settings;
    int maxItems = settings.value("recentItemsLimit", 10).toInt();

    recentItems.removeAll(path);
    recentItems.prepend(path);
    while (recentItems.size() > maxItems)
        recentItems.removeLast();

    settings.setValue("recentItems", recentItems);
    updateRecentMenu();
}

void MainWindow::updateRecentMenu()
{
    QMenu* menu = ui->menuOpenRecent;
    menu->clear();
    for (const auto& path : std::as_const(recentItems))
    {
        QAction* action = menu->addAction(path, this, &MainWindow::openRecent);
        action->setData(path);
    }
    menu->setEnabled(!recentItems.isEmpty());
}

void MainWindow::loadSettings()
{
    Settings settings;
    qDebug() << "Settings location: " << settings.fileName();

    if (settings.contains(objectName() + "/geometry") && settings.contains(objectName() + "/state"))
    {
        restoreGeometry(settings.value(objectName() + "/geometry").toByteArray());
        restoreState(settings.value(objectName() + "/state").toByteArray());
    }
    else
    {
        setWindowState(Qt::WindowState::WindowMaximized);
    }

    recentItems = settings.value("recentItems").toStringList();
}
