#pragma once

#include <QHeaderView>
#include <QLineEdit>

#include <vector>


class FilterHeader : public QHeaderView
{
    Q_OBJECT

public:
    explicit FilterHeader(Qt::Orientation orientation, QWidget *parent = nullptr);

    void setFields();

    QString filterText(int section) const;

signals:
    void filterChanged(int section, const QString &text);

protected:
    void setModel(QAbstractItemModel *model) override;
    void resizeEvent(QResizeEvent *event) override;
    QSize sizeHint() const override;
    void paintSection(QPainter *painter, const QRect &rect, int logicalIndex) const override;

private:
    void setupEditors();
    void updatePositions();

private:
    std::vector<QLineEdit*> editors;
    int filterHeight = 20;
};
