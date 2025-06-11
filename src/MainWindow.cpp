#include "MainWindow.h"
#include "ui_MainWindow.h"

#include "Application.h"
#include "Utils.h"
#include "LogManager.h"
#include "InitialDataDialog.h"
#include "LogModel.h"

#include <QFileDialog>
#include <QInputDialog>


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    formatManager(qobject_cast<Application*>(QApplication::instance())->getFormatManager())
{
    ui->setupUi(this);

    for (const auto& format : formatManager.getFormats())
    {
        QAction* action = formatActions.emplace_back(new QAction(QString::fromStdString(format.first), this));
        action->setCheckable(true);
        action->setChecked(true);
        connect(action, &QAction::toggled, this, [this, format](bool checked) {
            if (checked)
                selectedFormats.insert(format.first);
            else
                selectedFormats.erase(format.first);
        });
        ui->menuFormats->addAction(action);

        selectedFormats.insert(format.first);
    }
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

    std::unique_ptr<LogManager> manager = std::make_unique<LogManager>();
    auto scanResult = manager->loadFolders({ folderPath }, getSelectedFormats());

    InitialDataDialog dialog(scanResult);
    if (dialog.exec() != QDialog::Accepted)
        return;

    auto startDate = dialog.getStartDate();
    auto endDate = dialog.getEndDate();
    auto modules = dialog.getModules();

    if (startDate > endDate)
    {
        qWarning() << tr("Start date cannot be later than end date.");
        return;
    }

    if (modules.empty())
    {
        qWarning() << tr("No modules selected.");
        return;
    }

    manager->setTimeRange(startDate, endDate);

    auto logModel = new LogModel(std::move(manager), this);
    logModel->setModules(modules);

    ui->tableView->setModel(logModel);

    QT_SLOT_END
}

void MainWindow::checkActions()
{
    setLogActionsEnabled(!selectedFormats.empty());
}

void MainWindow::on_actionClose_triggered()
{
    QT_SLOT_BEGIN

    ui->tableView->setModel(nullptr);

    QT_SLOT_END
}


void MainWindow::on_actionAdd_format_triggered()
{
    QT_SLOT_BEGIN


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

    QT_SLOT_END
}

void MainWindow::on_actionSelect_all_triggered()
{
    updateFormatActions(true);
}


void MainWindow::on_actionDeselect_all_triggered()
{
    updateFormatActions(false);
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
