#include "FilterHeader.h"

#include "LogFilterModel.h"

#include <QAbstractItemModel>
#include <QResizeEvent>


FilterHeader::FilterHeader(Qt::Orientation orientation, QWidget *parent) : QHeaderView(orientation, parent)
{
    setSectionsClickable(true);
    connect(this, &QHeaderView::sectionResized, this, &FilterHeader::updatePositions);
    connect(this, &QHeaderView::sectionMoved, this, &FilterHeader::updatePositions);
}

QString FilterHeader::filterText(int section) const
{
    if (section < 0 || section >= static_cast<int>(editors.size()))
        return QString();
    return editors[section]->text();
}

void FilterHeader::setModel(QAbstractItemModel* model)
{
    QHeaderView::setModel(model);
    setupEditors();
    adjustColumnWidths(model);
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

    for (int i = 0; i < model()->columnCount(); ++i)
    {
        auto edit = new QLineEdit(this);
        edit->setPlaceholderText(tr("Filter"));
        connect(edit, &QLineEdit::textChanged, this, [this, i, proxyModel](const QString& t) {
            emit filterChanged(i, t);
            proxyModel->setFilterWildcard(i, t);
        });

        editors.push_back(edit);
    }
    updatePositions();
}

void FilterHeader::updatePositions()
{
    int y = QHeaderView::sizeHint().height();
    for (int i = 0; i < static_cast<int>(editors.size()); ++i)
    {
        int logical = logicalIndex(i);
        if (isSectionHidden(logical)) {
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

void FilterHeader::adjustColumnWidths(QAbstractItemModel* model)
{
    if (model && model->columnCount() > 0)
    {
        for (int i = 0; i < model->columnCount() - 1; ++i)
            setSectionResizeMode(i, QHeaderView::ResizeMode::ResizeToContents);
        setSectionResizeMode(model->columnCount() - 1, QHeaderView::ResizeMode::Stretch);
    }
}
