#include "MainWindow.h"
#include "DatabaseBot.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    DatabaseBot::databaseBot.init();
    a.setOrganizationDomain("CatchChallenger");
    a.setOrganizationName("bot-action");
    MainWindow w;
    w.show();

    return a.exec();
}
