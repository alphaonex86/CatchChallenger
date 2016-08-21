#include "GlobalControler.h"
#include <QApplication>

/* return code:
 * 23: server or charactersGroupIndex not found
 * 24: charactersGroupIndex not found
 * 25: character not found
 * 26: receive an error message
 * /

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setOrganizationDomain("CatchChallenger");
    a.setOrganizationName("bot-test-connect-to-gameserver");
    GlobalControler w;
    return a.exec();
}
