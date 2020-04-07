#include <QApplication>
#include "../qt/LanguagesSelect.hpp"
#include "ScreenTransition.hpp"
#include "../qt/LocalListener.hpp"
#include "../qt/QtDatapackClientLoader.hpp"
#include "../general/base/FacilityLibGeneral.hpp"
//#include "mainwindow.h"
#include <iostream>
#include <QFontDatabase>
#include <QStyleFactory>
#include "../qt/Options.hpp"
#include "../qt/QtDatapackChecksum.hpp"
#include <QStandardPaths>
#include <QScreen>
#include "Language.hpp"
#include "../qt/Settings.hpp"

int main(int argc, char *argv[])
{
    //QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
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
    Settings::init();

    QFontDatabase::addApplicationFont(":/other/comicbd.ttf");
    QFont font("Comic Sans MS");
    font.setStyleHint(QFont::Monospace);
    //font.setBold(true);
    font.setPixelSize(15);
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

    if(Settings::settings->contains("language"))
        Language::language.setLanguage(Settings::settings->value("language").toString());
    Options::options.loadVar();

    ScreenTransition s;
    s.setWindowTitle(QObject::tr("CatchChallenger loading..."));
    /*QScreen *screen = QApplication::screens().at(0);
    s.setMinimumSize(QSize(screen->availableSize().width(),
                           screen->availableSize().height()));*/
    #ifndef  Q_OS_ANDROID
    s.setMinimumSize(QSize(640,480));
    #endif
    s.move(0,0);
    s.show();
    QIcon icon;
    icon.addFile(":/CC/images/catchchallenger.png", QSize(), QIcon::Normal, QIcon::Off);
    s.setWindowIcon(icon);
#ifdef  Q_OS_ANDROID
    s.showFullScreen();
#else
    //s.showMaximized();
#endif
    QtDatapackClientLoader::datapackLoader=new QtDatapackClientLoader();
    const auto returnCode=a.exec();
    delete QtDatapackClientLoader::datapackLoader;
    #if ! defined(QT_NO_EMIT) && ! defined(EPOLLCATCHCHALLENGERSERVER) && !defined(NOTHREADS)
    CatchChallenger::QtDatapackChecksum::thread.quit();
    CatchChallenger::QtDatapackChecksum::thread.wait();
    #endif
    return returnCode;
}
