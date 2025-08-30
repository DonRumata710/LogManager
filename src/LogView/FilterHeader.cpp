#include "FilterHeader.h"

#include "LogFilterModel.h"
#include "LogModel.h"
#include "Utils.h"

#include <QAbstractItemModel>
#include <QResizeEvent>
#include <QMenu>
#include <QStyle>


FilterHeader::FilterHeader(Qt::Orientation orientation, QWidget *parent) : QHeaderView(orientation, parent)
{
    setSectionsClickable(true);
    connect(this, &QHeaderView::sectionResized, this, &FilterHeader::updatePositions);
    connect(this, &QHeaderView::sectionMoved, this, &FilterHeader::updatePositions);

    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &QWidget::customContextMenuRequested, this, &FilterHeader::showContextMenu);
}

void FilterHeader::scroll(int dx)
{
    for (const auto& editor : editors)
    {
        if (editor)
            editor->move(editor->x() + dx, editor->y());
    }
}

void FilterHeader::showContextMenu(const QPoint& point)
{
    QT_SLOT_BEGIN

    QMenu menu(this);

    if (!model())
        return;

    for (int col = 0; col < model()->columnCount(); ++col)
    {
        QString header = model()->headerData(col, Qt::Horizontal).toString();
        QAction* action = menu.addAction(header);
        action->setCheckable(true);
        action->setChecked(!isSectionHidden(col));

        connect(action, &QAction::toggled, this, [this, col](bool checked) {
            setSectionHidden(col, !checked);
        });
    }

    menu.exec(mapToGlobal(point));

    QT_SLOT_END
}

void FilterHeader::updateEditors()
{
    QT_SLOT_BEGIN

    auto proxyLogModel = qobject_cast<LogFilterModel*>(model());
    if (!proxyLogModel)
    {
        qWarning() << "FilterHeader requires a QSortFilterProxyModel as the model.";
        return;
    }

    auto logModel = qobject_cast<LogModel*>(proxyLogModel->sourceModel());
    if (!logModel)
    {
        qWarning() << "FilterHeader requires a LogModel as the source model.";
        return;
    }

    if (model()->rowCount() == 0)
        return;

    for (int i = 0; i < model()->columnCount(); ++i)
    {
        auto cb = qobject_cast<MultiSelectComboBox*>(editors[i]);
        auto values = logModel->availableValues(i);
        if (!cb)
        {
            if (values.empty())
                continue;

            cb = qobject_cast<MultiSelectComboBox*>(editors.emplace_back(createComboBoxEditor(i, values, proxyLogModel)));
            if (!cb)
            {
                qCritical() << "Failed to create MultiSelectComboBox for column" << i;
                continue;
            }
        }

        for (const auto& val : values)
            cb->addItem(val.toString(), val);
    }

    QT_SLOT_END
}

void FilterHeader::setModel(QAbstractItemModel* model)
{
    QHeaderView::setModel(model);
    setupEditors();
    adjustLastColumn();
    setSectionHidden(static_cast<int>(LogModel::PredefinedColumn::Module), true);
    connect(model, &QAbstractItemModel::modelReset, this, &FilterHeader::updateEditors);
    connect(model, &QAbstractItemModel::rowsInserted, this, &FilterHeader::updateEditors);
}

void FilterHeader::resizeEvent(QResizeEvent *event)
{
    QHeaderView::resizeEvent(event);
    updatePositions();
}

QSize FilterHeader::sizeHint() const
{
    QSize sz = QHeaderView::sizeHint();
    sz.setHeight(sz.height() + filterHeight);
    return sz;
}

void FilterHeader::paintSection(QPainter* painter, const QRect& rect, int logicalIndex) const
{
    QStyleOptionHeader opt;
    initStyleOption(&opt);
    opt.rect = rect;
    opt.section = logicalIndex;
    style()->drawControl(QStyle::CE_HeaderSection, &opt, painter, this);

    int margin = style()->pixelMetric(QStyle::PM_HeaderMargin, &opt, this);
    QRect textRect = rect.adjusted(margin, margin, -margin, -rect.height() / 2);

    opt.text = model()->headerData(logicalIndex, orientation(), Qt::DisplayRole).toString();
    opt.rect = textRect;
    opt.position = QStyleOptionHeader::Middle;
    style()->drawControl(QStyle::CE_HeaderLabel, &opt, painter, this);
}

void FilterHeader::updatePositions()
{
    int y = QHeaderView::sizeHint().height();
    for (int i = 0; i < static_cast<int>(editors.size()); ++i)
    {
        int logical = logicalIndex(i);
        if (isSectionHidden(logical))
        {
            editors[i]->setVisible(false);
            continue;
        }

        int pos = sectionPosition(logical);
        int size = sectionSize(logical);

        QRect rect = QRect(pos, height() / 2, size, filterHeight);
        editors[i]->setGeometry(rect);
        editors[i]->setVisible(true);
    }
}

void FilterHeader::setupEditors()
{
    qDeleteAll(editors);
    editors.clear();

    if (!model())
        return;

    LogFilterModel* proxyModel = qobject_cast<LogFilterModel*>(model());
    if (!proxyModel)
    {
        qWarning() << "FilterHeader requires a LogFilterModel as the model.";
        return;
    }

    LogModel* logModel = qobject_cast<LogModel*>(proxyModel->sourceModel());
    if (!logModel)
    {
        qWarning() << "FilterHeader requires a LogModel as the source model.";
        return;
    }

    for (int i = 0; i < model()->columnCount(); ++i)
    {
        QAction* modeAction = nullptr;
        auto values = logModel->availableValues(i);
        if (values.empty())
        {
            auto edit = new QLineEdit(this);
            edit->setPlaceholderText(tr("Filter"));
            connect(edit, &QLineEdit::textChanged, this, [this, i, proxyModel](const QString& t) {
                emit filterChanged(i, t);

                auto filter = t;
                proxyModel->setFilterWildcard(i, filter);
            });

            modeAction = edit->addAction(whitelistIcon, QLineEdit::TrailingPosition);

            editors.push_back(edit);
        }
        else
        {
            auto cb = createComboBoxEditor(i, values, proxyModel);
            modeAction = cb->AddAction(whitelistIcon);
            editors.push_back(cb);
        }
        
        if (!modeAction)
            throw std::runtime_error("Failed to create mode action");

        if (proxyModel->filterMode(i) == FilterType::Blacklist)
            modeAction->setIcon(blacklistIcon);
        modeAction->setToolTip(tr("Toggle whitelist/blacklist"));
        connect(modeAction, &QAction::triggered, this, [proxyModel, i, modeAction, this]() {
            auto newMode = ((proxyModel->filterMode(i) == FilterType::Whitelist) ? FilterType::Blacklist : FilterType::Whitelist);
            proxyModel->setFilterMode(i, newMode);
            if (newMode == FilterType::Whitelist)
                modeAction->setIcon(whitelistIcon);
            else
                modeAction->setIcon(blacklistIcon);
        });
    }
    updatePositions();
}

void FilterHeader::adjustLastColumn()
{
    if (model() && model()->columnCount() > 0)
        setSectionResizeMode(model()->columnCount() - 1, QHeaderView::ResizeMode::Stretch);
}

MultiSelectComboBox* FilterHeader::createComboBoxEditor(int i, const std::unordered_set<QVariant, VariantHash>& values, LogFilterModel* proxyModel)
{
    auto comboBox = new MultiSelectComboBox(this);
    comboBox->setPlaceholderText(tr("Filter"));

    for (const auto& val : values)
        comboBox->addItem(val.toString(), val);

    connect(comboBox, &MultiSelectComboBox::currentTextChanged, this, [this, i, proxyModel, comboBox](const QString& text) {
        emit filterChanged(i, text);
        proxyModel->setVariantList(i, comboBox->currentText());
    });
    return comboBox;
}
