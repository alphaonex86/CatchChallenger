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
    // Mirror the TEST-ONLY flags into the shared CliClientOptions: the click-to-
    // sign self-test lives in the shared map controller and the dialog-overflow
    // self-test reads CliClientOptions, so both clients route through it.
    CliClientOptions::clickSignTest=AutoArgs::clickSignTest;
    CliClientOptions::clickDoorTest=AutoArgs::clickDoorTest;
    CliClientOptions::keyboardTest=AutoArgs::keyboardTest;
    CliClientOptions::dialogOverflowTest=AutoArgs::dialogOverflowTest;
    CliClientOptions::autosoloClick=AutoArgs::autosoloClick;
    CliClientOptions::autosoloClickDx=AutoArgs::autosoloClickDx;
    CliClientOptions::autosoloClickDy=AutoArgs::autosoloClickDy;
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
    //QLocalServer automation channel: external controllers / tests can now send
    //KEY/CLICKTILE/CLICKPIXEL/GETSTATE/GETINVENTORY to this instance's socket.
    w.wireRemoteControl(&localListener);
    if(!AutoArgs::takeScreenshotPath.isEmpty())
        //Screenshot regression needs the window painted before grab().
        w.show();
    else if(!AutoArgs::host.isEmpty() || !AutoArgs::server.isEmpty() ||
            !AutoArgs::url.isEmpty() || AutoArgs::closeWhenOnMap ||
            AutoArgs::dropSendDataAfterOnMap || AutoArgs::autosolo)
        //Automated connect / autosolo / closeWhenOnMap (test & CI
        //path): self-terminates on the map marker, no operator — keep
        //the window off the user's screen. showMinimized() still
        //creates+exposes+paints the widget so the scene reaches
        //MapVisualiserPlayer::mapDisplayedSlot() exactly as before; a
        //full hide would suppress the expose and the marker.
        w.showMinimized();
    else
        w.show();
    const int returnCode=a.exec();
    if(w.toQuit)
        return 523;
    else
        return returnCode;
}
