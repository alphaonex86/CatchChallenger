#include <QApplication>
#include "SimpleSoloServer.h"
#include "../base/LanguagesSelect.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setApplicationName("client");
    a.setOrganizationName("CatchChallenger");
    LanguagesSelect::languagesSelect=new LanguagesSelect();
    SimpleSoloServer w;
    w.show();
    return a.exec();
}
