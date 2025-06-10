
#include "MainWindow.h"

#include <QApplication>


int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    app.setApplicationName(APPLICATION_NAME);
    app.setApplicationVersion(APPLICATION_VERSION);

    MainWindow mainWindow;
    mainWindow.show();

    return app.exec();
}
