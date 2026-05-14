#include <QApplication>
#include "mainwindow.h"
#include "../base/LanguagesSelect.h"
#include "../base/AutoArgs.h"
#include "../../libqtcatchchallenger/LocalListener.hpp"
#include "../../libqtcatchchallenger/CliClientOptions.hpp"
#include "../../../general/base/FacilityLibGeneral.hpp"
#include <iostream>
#include <cstdlib>
#include <QFontDatabase>

int main(int argc, char *argv[])
{
    AutoArgs::parse(argc,argv);
    // --take-screenshot wants reproducible PNGs: tile-variant
    // selectors (lava, plant cycles, follower-NPC offsets) all
    // call std::rand(), so pin the seed before any of them runs.
    // Mirrors the same srand(42) qtopengl/main.cpp:344 does for
    // the same reason — the per-pixel-tolerance diff in
    // testingcompilationwindows otherwise flaps on every
    // random-frame draw.
    if(!AutoArgs::takeScreenshotPath.isEmpty())
    {
        std::srand(42);
        // Mirror AutoArgs::takeScreenshotPath into the shared-lib's
        // CliClientOptions::takeScreenshotPath. The shared
        // MapVisualiser/MapController code only knows about
        // CliClientOptions, so without this the follower-NPC
        // animation timers in MapVisualiser-map.cpp:258 still get
        // created and tile-cycle the sprite between paint() and
        // grab(), producing a ~0.83 % per-pixel drift at the player
        // sprite + follower NPC coords (x=172..243, y=226..315) on
        // every run. Same gate also forces bot lookAt
        // "random"/"loop"/"move" to "bottom" in MapController.cpp:281
        // for deterministic facing.
        CliClientOptions::takeScreenshotPath=AutoArgs::takeScreenshotPath;
    }
    QApplication a(argc, argv);
    a.setApplicationName("client-qtcpu800x600");
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

    //QFontDatabase::addApplicationFont(":/fonts/komika_font.ttf");

    LanguagesSelect::languagesSelect=new LanguagesSelect();
    MainWindow w;
    if(w.toQuit)
        return 523;
    w.show();
    const int returnCode=a.exec();
    if(w.toQuit)
        return 523;
    else
        return returnCode;
}
