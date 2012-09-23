#include <QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setApplicationName("client");
    a.setOrganizationName("Pokecraft");
    MainWindow w;
    w.show();
    return a.exec();
}
