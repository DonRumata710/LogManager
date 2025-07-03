#include "MainWindow.h"
#include "ui_MainWindow.h"

#include "Application.h"
#include "Utils.h"
#include "InitialDataDialog.h"
#include "LogView/LogModel.h"
#include "LogView/FilterHeader.h"
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

    for (const auto& format : formatManager.getFormats())
    {
        addFormat(format.first);
        selectedFormats += format.second->name;
    }

    setLogActionsEnabled(!selectedFormats.empty());

    auto header = new FilterHeader(Qt::Horizontal, ui->logView);
    ui->logView->setHeader(header);
    ui->logView->setUniformRowHeights(false);

    connect(ui->logView->verticalScrollBar(), &QScrollBar::valueChanged, this, &MainWindow::checkFetchNeeded);
    connect(ui->logView->verticalScrollBar(), &QScrollBar::rangeChanged, this, &MainWindow::checkFetchNeeded);

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

void MainWindow::checkFetchNeeded()
{
    QT_SLOT_BEGIN

    QScrollBar* sb = ui->logView->verticalScrollBar();
    int min = sb->minimum();
    int max = sb->maximum();
    int value = sb->value();
    int range = max - min;

    auto* model = qobject_cast<LogModel*>(ui->logView->model());
    auto* proxyModel = qobject_cast<QAbstractProxyModel*>(ui->logView->model());
    if (proxyModel)
        model = qobject_cast<LogModel*>(proxyModel->sourceModel());

    if (!model || model->rowCount() == 0)
    {
        qWarning() << "Unexpected model type in log view";
        return;
    }

    if (value - min >= range * 0.9 || max == 0)
        model->fetchDownMore();

    if (value - min <= range * 0.1 || max == 0)
        model->fetchUpMore();

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

    auto logFilterModel = qobject_cast<LogFilterModel*>(ui->logView->model());
    if (!logFilterModel)
    {
        qWarning() << "Log model is not set.";
        return;
    }

    auto logModel = qobject_cast<LogModel*>(logFilterModel->sourceModel());

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

    auto proxyModel = new LogFilterModel(this);

    auto logModel = new LogModel(logService, startDate, endDate, proxyModel);
    logModel->setModules(modules);

    proxyModel->setSourceModel(logModel);

    switchModel(proxyModel);
    setCloseActionEnabled(true);

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
    ui->logView->setModel(model);
    ui->logView->header()->resizeSections(QHeaderView::ResizeMode::ResizeToContents);
    if (oldModel)
        delete oldModel;
}
