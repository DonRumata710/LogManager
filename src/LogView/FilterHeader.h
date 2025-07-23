#pragma once

#include "LogManagement/Format.h"
#include "LogView/LogFilterModel.h"

#include <QHeaderView>
#include <QLineEdit>

#include <vector>
#include <unordered_set>


class FilterHeader : public QHeaderView
{
    Q_OBJECT

public:
    explicit FilterHeader(Qt::Orientation orientation, QWidget *parent = nullptr);

signals:
    void filterChanged(int section, const QString &text);

    void handleError(const QString &message);

private slots:
    void showContextMenu(const QPoint &point);
    void updateEditors();

protected:
    void setModel(QAbstractItemModel *model) override;
    void resizeEvent(QResizeEvent *event) override;
    QSize sizeHint() const override;
    void paintSection(QPainter *painter, const QRect &rect, int logicalIndex) const override;

private:
    void updatePositions();
    void setupEditors();
    void adjustLastColumn();

    QWidget* createComboBoxEditor(int section, const std::unordered_set<QVariant, VariantHash>& values, LogFilterModel* proxyModel);

private:
    std::vector<QWidget*> editors;
    int filterHeight = 20;
};
