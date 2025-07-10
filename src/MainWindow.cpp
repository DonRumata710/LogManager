#include "MainWindow.h"
#include "ui_MainWindow.h"

#include "Application.h"
#include "Utils.h"
#include "InitialDataDialog.h"
#include "Settings.h"
#include "LogView/LogModel.h"
#include "LogView/LogFilterModel.h"
#include "FormatCreation/FormatCreationWizard.h"

#include <QScrollBar>
#include <QFileDialog>
#include <QInputDialog>


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    formatManager(qobject_cast<Application*>(QApplication::instance())->getFormatManager())
{
    ui->setupUi(this);
    setWindowTitle(QApplication::instance()->applicationName());

    ui->searchBar->hide();

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

    for (const auto& format : formatManager.getFormats())
    {
        addFormat(format.first);
        selectedFormats += format.second->name;
    }

    setLogActionsEnabled(!selectedFormats.empty());

    auto logService = qobject_cast<Application*>(QApplication::instance())->getLogService();
    connect(this, &MainWindow::openFile, logService, &LogService::openFile);
    connect(this, &MainWindow::openFolder, logService, &LogService::openFolder);
    connect(logService, &LogService::logManagerCreated, this, &MainWindow::logManagerCreated);

    connect(this, SIGNAL(exportData(QString,QDateTime,QDateTime)), logService, SLOT(exportData(QString,QDateTime,QDateTime)));
    connect(this, SIGNAL(exportData(QString,QDateTime,QDateTime,QStringList)), logService, SLOT(exportData(QString,QDateTime,QDateTime,QStringList)));
    connect(this, SIGNAL(exportData(QString,QTreeView*)), logService, SLOT(exportData(QString,QTreeView*)));
    connect(this, SIGNAL(exportData(QString,QDateTime,QDateTime,QStringList,LogFilter)), logService, SLOT(exportData(QString,QDateTime,QDateTime,QStringList,LogFilter)));
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

    QT_SLOT_END
}

void MainWindow::on_actionOpen_file_triggered()
{
    QT_SLOT_BEGIN

    QString extensions;
    QMap<QString, QString> formatMap;
    for (const auto& formatName : std::as_const(selectedFormats))
    {
        auto format = formatManager.getFormats().at(formatName.toStdString());
        if (!format)
            continue;

        formatMap.insert(format->extension, format->name);

        if (!extensions.isEmpty())
            extensions += ";;";
        extensions += formatName + " (*" + format->extension + ")";
    }

    QString file = QFileDialog::getOpenFileName(this, tr("Open File"), QDir::currentPath(), extensions);    
    if (file.isEmpty())
        return;

    QStringList formats;
    auto range = formatMap.equal_range(file.mid(file.lastIndexOf('.')));
    for (auto it = range.first; it != range.second; ++it)
    {
        formats += *it;
    }

    openFile(file, formats);

    QT_SLOT_END
}

void MainWindow::on_actionClose_triggered()
{
    QT_SLOT_BEGIN

    switchModel(nullptr);
    setCloseActionEnabled(false);
    ui->searchBar->hide();

    QT_SLOT_END
}

void MainWindow::on_actionAdd_format_triggered()
{
    QT_SLOT_BEGIN

    FormatCreationWizard wizard(this);
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

void MainWindow::on_searchBar_localSearch(const QString& searchTerm, bool lastColumn, bool regexEnabled, bool backward)
{
    QT_SLOT_BEGIN

    search(ui->logView->currentIndex(), searchTerm, lastColumn, regexEnabled, backward, false);

    QT_SLOT_END
}

void MainWindow::on_searchBar_commonSearch(const QString& searchTerm, bool lastColumn, bool regexEnabled, bool backward)
{
    QT_SLOT_BEGIN

    search(ui->logView->currentIndex(), searchTerm, lastColumn, regexEnabled, backward, true);

    QT_SLOT_END
}

void MainWindow::search(const QModelIndex& from, const QString& searchTerm, bool lastColumn, bool regexEnabled, bool backward, bool globalSearch)
{
    QT_SLOT_BEGIN

    auto proxyModel = qobject_cast<QSortFilterProxyModel*>(ui->logView->model());
    auto model = getLogModel();

    size_t start = from.row() + (backward ? -1 : 1);
    for (size_t i = start; i < model->rowCount(); backward ? --i : ++i)
    {
        QModelIndex index{ model->index(i, 0) };

        QString textToSearch = model->data(index, static_cast<int>(lastColumn ? LogModel::MetaData::Message : LogModel::MetaData::Line)).toString();

        bool flag = false;
        if (regexEnabled)
        {
            QRegularExpression regex(searchTerm, QRegularExpression::CaseInsensitiveOption);
            if (regex.match(textToSearch).hasMatch())
                flag = true;
        }
        else
        {
            if (textToSearch.contains(searchTerm))
                flag = true;
        }

        if (flag)
        {
            qDebug() << "Search found at row:" << i << "text:" << textToSearch;

            if (proxyModel)
                index = proxyModel->mapFromSource(index);

            ui->logView->selectionModel()->setCurrentIndex(index, QItemSelectionModel::SelectionFlag::SelectCurrent | QItemSelectionModel::SelectionFlag::Rows);
            ui->logView->scrollTo(index, QTreeView::PositionAtCenter);
            return;
        }
    }

    if (globalSearch)
    {
        // TODO
    }

    QT_SLOT_END
}

void MainWindow::on_actionExport_as_is_triggered()
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

    QString fileName = QFileDialog::getSaveFileName(this, tr("Export Logs"), QDir::currentPath(), tr("Origin format logs (*.log);; CSV Files (*.csv)"));
    if (fileName.isEmpty())
        return;

    auto logModel = getLogModel();
    if (!logModel)
    {
        qCritical() << "Unexpected model type in log view";
        return;
    }

    bool originFormat = fileName.endsWith(".log", Qt::CaseInsensitive);
    if (originFormat)
        exportData(fileName, logModel->getStartTime(), logModel->getEndTime());
    else
        exportData(fileName, logModel->getStartTime(), logModel->getEndTime(), logModel->getFieldsName());

    QT_SLOT_END
}

void MainWindow::on_actionFiltered_export_triggered()
{
    QT_SLOT_BEGIN

    QString fileName = QFileDialog::getSaveFileName(this, tr("Export Logs"), QDir::currentPath(), tr("Origin format logs (*.log);; CSV Files (*.csv)"));
    if (fileName.isEmpty())
        return;

    auto logFilterModel = qobject_cast<LogFilterModel*>(ui->logView->model());
    if (!logFilterModel)
    {
        qWarning() << "Log model is not set.";
        return;
    }

    auto logModel = qobject_cast<LogModel*>(logFilterModel->sourceModel());

    exportData(fileName, logModel->getStartTime(), logModel->getEndTime(), logModel->getFieldsName(), logFilterModel->exportFilter());

    QT_SLOT_END
}

void MainWindow::logManagerCreated()
{
    QT_SLOT_BEGIN

    auto logService = qobject_cast<Application*>(QApplication::instance())->getLogService();

    auto logManager = logService->getLogManager();
    if (!logService->getLogManager())
    {
        qWarning() << "LogManager is not created.";
        return;
    }

    InitialDataDialog dialog(*logManager.get());
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

    logService->createSession(modules, startDate.toStdSysMilliseconds(), endDate.addMSecs(1).toStdSysMilliseconds());

    auto proxyModel = new LogFilterModel(this);

    auto logModel = new LogModel(logService, proxyModel);

    if (scrollToEnd)
        logModel->goToTime(std::chrono::system_clock::time_point::max());
    else
        logModel->goToTime(std::chrono::system_clock::time_point::min());

    proxyModel->setSourceModel(logModel);

    switchModel(proxyModel);
    setCloseActionEnabled(true);
    ui->searchBar->show();

    QT_SLOT_END
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
}

void MainWindow::updateFormatActions(bool enabled)
{
    for (const auto& action : formatActions)
        action->setChecked(enabled);
    checkActions();
}

void MainWindow::switchModel(QAbstractItemModel* model)
{
    if (model)
    {
        auto resizeFunc = [view = ui->logView]() {
            view->header()->resizeSections(QHeaderView::ResizeMode::ResizeToContents);
        };

        connect(model, &QAbstractItemModel::modelReset, resizeFunc);
    }

    auto oldModel = ui->logView->model();
    ui->logView->setLogModel(model);
    ui->logView->header()->resizeSections(QHeaderView::ResizeMode::ResizeToContents);
    if (oldModel)
        delete oldModel;
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
