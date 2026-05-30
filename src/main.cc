#include <QApplication>
#include "MainWindow.h"

int main(int argc, char *argv[])
{
    // Initialize the Qt framework
    QApplication app(argc, argv);

    // Create and show your window
    MainWindow window;
    window.show();

    // Start the event loop (listens for mouse clicks)
    return app.exec();
}