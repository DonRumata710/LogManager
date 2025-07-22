#pragma once

#include <QHeaderView>
#include <QLineEdit>

#include <vector>


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

private:
    std::vector<QWidget*> editors;
    int filterHeight = 20;
};
