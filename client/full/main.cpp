#include <QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setApplicationName("client-multi-server");
    a.setOrganizationName("CatchChallenger");
    MainWindow w;
    w.show();
    return a.exec();
}
