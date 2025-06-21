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
        selectedFormats.insert(format.first);
    }

    setLogActionsEnabled(!selectedFormats.empty());

    auto header = new FilterHeader(Qt::Horizontal, ui->logView);
    ui->logView->setHeader(header);
    ui->logView->setUniformRowHeights(false);

    connect(ui->logView->verticalScrollBar(), &QScrollBar::valueChanged, this, &MainWindow::checkFetchNeeded);
    connect(ui->logView->verticalScrollBar(), &QScrollBar::rangeChanged, this, &MainWindow::checkFetchNeeded);
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

    std::unique_ptr<LogManager> manager = std::make_unique<LogManager>(std::vector<QString>{ folderPath }, getSelectedFormats());
    showLogs(std::move(manager));

    QT_SLOT_END
}

void MainWindow::on_actionOpen_file_triggered()
{
    QT_SLOT_BEGIN

    QString extensions;
    std::unordered_multimap<QString, std::shared_ptr<Format>> formatMap;
    for (const auto& formatName : selectedFormats)
    {
        auto format = formatManager.getFormats().at(formatName);
        if (!format)
        {
            continue;
        }

        formatMap.emplace(format->extension, format);

        if (!extensions.isEmpty())
            extensions += ";;";
        extensions += QString::fromStdString(formatName) + " (*" + format->extension + ")";
    }

    QString file = QFileDialog::getOpenFileName(this, tr("Open File"), QDir::currentPath(), extensions);

    auto range = formatMap.equal_range(file.mid(file.lastIndexOf('.')));
    std::vector<std::shared_ptr<Format>> formats;
    for (auto it = range.first; it != range.second; ++it)
        formats.push_back(it->second);

    std::unique_ptr<LogManager> manager = std::make_unique<LogManager>(file, formats);
    showLogs(std::move(manager));

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
            selectedFormats.erase(selectedFormat.toStdString());
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

    if (!model)
    {
        qWarning() << "Unexpected model type in log view";
        return;
    }

    // TODO: upside fetching

    if (value - min >= range * 0.9 || max == 0 || !sb->isVisible())
        model->fetchDownMore();

    QT_SLOT_END
}

void MainWindow::addFormat(const std::string& format)
{
    QAction* action = formatActions.emplace_back(new QAction(QString::fromStdString(format), this));
    action->setCheckable(true);
    action->setChecked(true);
    connect(action, &QAction::toggled, this, [this, format](bool checked) {
        if (checked)
            selectedFormats.insert(format);
        else
            selectedFormats.erase(format);
        checkActions();
    });
    ui->menuFormats->addAction(action);

    selectedFormats.insert(format);
    checkActions();
}

void MainWindow::showLogs(std::unique_ptr<LogManager>&& logManager)
{
    InitialDataDialog dialog(*logManager, this);
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

    auto logModel = new LogModel(std::move(logManager), startDate, proxyModel);
    logModel->setModules(modules);

    proxyModel->setSourceModel(logModel);

    switchModel(proxyModel);
    setCloseActionEnabled(true);
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
    auto oldModel = ui->logView->model();
    ui->logView->setModel(model);
    ui->logView->header()->resizeSections(QHeaderView::ResizeMode::ResizeToContents);
    if (oldModel)
        delete oldModel;
}

std::vector<std::shared_ptr<Format>> MainWindow::getSelectedFormats() const
{
    std::vector<std::shared_ptr<Format>> formats;
    for (const auto& formatName : selectedFormats)
    {
        auto format = formatManager.getFormats().at(formatName);
        if (format)
            formats.push_back(format);
    }
    return formats;
}
