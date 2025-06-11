#include "Application.h"


Application::Application(int& argc, char** argv) :
    QApplication(argc, argv)
{}

FormatManager& Application::getFormatManager()
{
    return formatManager;
}
