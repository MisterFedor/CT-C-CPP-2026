#include "mainwindow.hpp"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    MainWindow GameWindow;
    GameWindow.show();
    return app.exec();
}
