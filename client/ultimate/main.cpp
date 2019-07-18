#include <QApplication>
#include "../base/CCBackground.h"
#include "../base/LoadingScreen.h"
#include "../base/LanguagesSelect.h"
#include "../base/LocalListener.h"
#include "../../general/base/FacilityLibGeneral.h"
#include <iostream>
#include <QFontDatabase>

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

    QFont font("Comic Sans MS");
    font.setStyleHint(QFont::Monospace);
    font.setBold(true);
    QApplication::setFont(font);

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

    //QFontDatabase::addApplicationFont(":/fonts/komika_font.ttf");

    LanguagesSelect::languagesSelect=new LanguagesSelect();
    /*MainWindow w;
    if(w.toQuit)
        return 523;
    w.show();*/
    CCBackground background;
    background.setWindowTitle(QObject::tr("CatchChallenger loading..."));
    background.show();
    /*LoadingScreen l;
    l.show();*/
    const auto returnCode=a.exec();
    /*if(w.toQuit)
        return 523;
    else
        return returnCode;*/
    return returnCode;
}
