#include <QApplication>
#include "mainwindow.h"
#include "../base/LanguagesSelect.h"
#include "../base/LocalListener.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setApplicationName("client-ultimate");
    a.setOrganizationName("CatchChallenger");

    if(argc<1)
    {
        std::cerr << "argc<1: wrong arg count" << std::endl;
        return EXIT_FAILURE;
    }
    CatchChallenger::FacilityLibGeneral::applicationDirPath=argv[0];
    
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
    MainWindow w;
    if(w.toQuit)
        return 523;
    w.show();
    return a.exec();
}
