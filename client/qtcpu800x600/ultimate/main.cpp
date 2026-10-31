#include <QApplication>
#include "mainwindow.h"
#include "../base/LanguagesSelect.h"
#include "../../libqtcatchchallenger/CerrLineAtomic.hpp"
#include "../../libqtcatchchallenger/LocalListener.hpp"
#include "../../libqtcatchchallenger/CliClientOptions.hpp"
#include "../../../general/base/FacilityLibGeneral.hpp"
#include <iostream>
#include <cstdlib>
#include <QFontDatabase>

int main(int argc, char *argv[])
{
    /* FIRST: this client is multi threaded (embedded server, SQL, datapack
     * loader, map visualiser, path finder, GUI) and they all log to the same
     * std::cerr, which has no line granularity -- two threads logging at once
     * splice or tear each other's message. Wrap the streams before anything
     * can log. */
    CatchChallenger::CerrLineAtomic::install();
    if(!CliClientOptions::parse(argc,argv))
        return 0;
    // --take-screenshot wants reproducible PNGs: tile-variant selectors (lava,
    // plant cycles, follower-NPC offsets) all call std::rand(), so pin the seed
    // before any of them runs (same as qtopengl/main.cpp).
    if(!CliClientOptions::takeScreenshotPath.isEmpty())
        std::srand(42);
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
    //The QLocalServer remote-control / automation channel is OPT-IN: listen only
    //when --remote-control was passed (mirrored into CliClientOptions above).
    if(CliClientOptions::remoteControl)
    {
        if(!localListener.tryListen())
            return 0;
    }

    //Ship the UI font instead of taking whatever the host happens to have.
    //Qt resolves the default Windows UI font through GDI (Segoe UI, or
    //MS Shell Dlg -> Tahoma); on a machine without those faces every glyph
    //rasterises as a .notdef box and the whole interface becomes unreadable
    //boxes - which is exactly what the Windows build does under a wine prefix
    //that has no Microsoft core font. qtopengl already bundles its own face
    //for the same reason. Liberation Sans is SIL Open Font License (so it may
    //be redistributed with the game) and is metric-compatible with Arial, so
    //the point sizes the .ui files set lay out as they were designed.
    //Only the FAMILY is replaced: size and weight stay whatever Qt picked.
    {
        const int fontId=QFontDatabase::addApplicationFont(":/fonts/LiberationSans-Regular.ttf");
        if(fontId<0)
            std::cerr << "Unable to load the bundled UI font, keeping the system font" << std::endl;
        else
        {
            const QStringList &families=QFontDatabase::applicationFontFamilies(fontId);
            if(families.isEmpty())
                std::cerr << "Bundled UI font exposes no family, keeping the system font" << std::endl;
            else
            {
                QFont appFont=QApplication::font();
                appFont.setFamily(families.first());
                QApplication::setFont(appFont);
            }
        }
    }

    LanguagesSelect::languagesSelect=new LanguagesSelect();
    MainWindow w;
    if(w.toQuit)
        return 523;
    //QLocalServer automation channel: external controllers / tests can now send
    //KEY/CLICKTILE/GOTO/CLOSEDIALOG/GETSTATE/GETDIALOG/... to this instance's
    //socket. Only wired when --remote-control opened the socket above.
    if(CliClientOptions::remoteControl)
        w.wireRemoteControl(&localListener);
    if(!CliClientOptions::takeScreenshotPath.isEmpty())
        //Screenshot regression needs the window painted before grab().
        w.show();
    else if(!CliClientOptions::host.isEmpty() || !CliClientOptions::serverName.isEmpty() ||
            !CliClientOptions::url.isEmpty() || CliClientOptions::closeWhenOnMap ||
            CliClientOptions::dropSendDataAfterOnMap || CliClientOptions::autosolo)
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
