#include <QApplication>
#include "SimpleSoloServer.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setApplicationName("client");
    a.setOrganizationName("CatchChallenger");
    SimpleSoloServer w;
    w.show();
    return a.exec();
}
