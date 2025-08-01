#pragma once

#include "../LogManagement/Format.h"

#include <QWizard>

namespace Ui {
class FormatCreationWizard;
}

class FormatCreationWizard : public QWizard
{
    Q_OBJECT

public:
    explicit FormatCreationWizard(QWidget *parent = nullptr);
    ~FormatCreationWizard();

    Format getFormat();

    int nextId() const override;

signals:
    void handleError(const QString& message);

private:
    Ui::FormatCreationWizard *ui;
};
