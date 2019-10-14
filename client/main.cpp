#include <QApplication>
#include "qt/LanguagesSelect.h"
#include "qt/ScreenTransition.h"
#include "qt/LocalListener.h"
#include "../general/base/FacilityLibGeneral.h"
//#include "mainwindow.h"
#include <iostream>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QStyleFactory>
#include "qt/Options.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setApplicationName("client");
    a.setOrganizationName("CatchChallenger");
    a.setStyle(QStyleFactory::create("Fusion"));

    if(argc<1)
    {
        std::cerr << "argc<1: wrong arg count" << std::endl;
        return EXIT_FAILURE;
    }
    CatchChallenger::FacilityLibGeneral::applicationDirPath=argv[0];

    QFont font("Comic Sans MS");
    font.setStyleHint(QFont::Monospace);
    //font.setBold(true);
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
    Options::options.loadVar();

    ScreenTransition s;
    s.setWindowTitle(QObject::tr("CatchChallenger loading..."));
    s.setMinimumSize(QSize(320,240));
    s.showMaximized();
    QIcon icon;
    icon.addFile(":/images/catchchallenger.png", QSize(), QIcon::Normal, QIcon::Off);
    s.setWindowIcon(icon);
    s.show();
    /*LoadingScreen l;
    l.show();*/
    const auto returnCode=a.exec();
    /*if(w.toQuit)
        return 523;
    else
        return returnCode;*/
    return returnCode;
}
