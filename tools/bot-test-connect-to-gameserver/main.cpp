#include "GlobalControler.h"
#include <QCoreApplication>

/* return code:
 * 23: server or charactersGroupIndex not found
 * 24: charactersGroupIndex not found
 * 25: character not found
 * 26: receive an error message
 * 27: Your password need to be at minimum of 6 characters
 * 28: Your login need to be at minimum of 3 characters
 * 29: Query too slow to responds
 */

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    a.setOrganizationDomain("CatchChallenger");
    a.setOrganizationName("bot-test-connect-to-gameserver");
    GlobalControler w;
    return a.exec();
}
