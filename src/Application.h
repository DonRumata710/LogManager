#pragma once

#include "FormatManager.h"

#include <QApplication>


class Application : public QApplication
{
    Q_OBJECT

public:
    Application(int& argc, char** argv);

    FormatManager& getFormatManager();

private:
    FormatManager formatManager;
};
