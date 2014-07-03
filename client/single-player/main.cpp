#include <QApplication>
#include "SimpleSoloServer.h"
#include "../base/LanguagesSelect.h"
#include "../base/LocalListener.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setApplicationName("client-single-player");
    a.setOrganizationName("CatchChallenger");
    
    LocalListener localListener;
    const QStringList &arguments=QCoreApplication::arguments();
    if(arguments.size()==2 && arguments.last()=="quit")
    {
        localListener.quitAllRunningInstance();
        return 0;
    }
    else
    {
        if(!localListener.tryListen())
            return 0;
    }
    
    LanguagesSelect::languagesSelect=new LanguagesSelect();
    SimpleSoloServer w;
    w.show();
    return a.exec();
}
