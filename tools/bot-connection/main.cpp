#include "MainWindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setOrganizationDomain("CatchChallenger");
    a.setOrganizationName("bot-connection");
    MainWindow w;
    w.show();

    return a.exec();
}
