#include "MainWindow.h"
#include <QCoreApplication>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    a.setOrganizationDomain("CatchChallenger");
    a.setOrganizationName("bot-connection");
    MainClass w;
    if(!w.needQuit)
        return a.exec();
    else
        return -1;
}
