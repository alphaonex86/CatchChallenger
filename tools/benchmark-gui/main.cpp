#include "MainWindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setApplicationName("benchmark-gui");
    a.setOrganizationName("CatchChallenger");
    a.setQuitOnLastWindowClosed(false);
    MainWindow w;
    w.show();

    return a.exec();
}
