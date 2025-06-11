
#include "Application.h"
#include "MainWindow.h"


int main(int argc, char *argv[])
{
    Application app(argc, argv);

    app.setApplicationName(APPLICATION_NAME);
    app.setApplicationVersion(APPLICATION_VERSION);

    MainWindow mainWindow;
    mainWindow.show();

    return app.exec();
}
