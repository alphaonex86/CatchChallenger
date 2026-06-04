#include <QApplication>
#include <QMessageBox>
#include <QTimer>
#include <QCoreApplication>
#include <QFileInfo>
#include <QDebug>
#include <QStringLiteral>
#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>
#include "MainWindow.hpp"
#include "../base/NormalServerGlobal.hpp"
#include "../base/GlobalServerData.hpp"
#include "../../general/base/FacilityLibGeneral.hpp"
// Tee std::cout / std::cerr into the GUI's textEditConsole.  Lives in
// general/base/ so any non-Qt server binary could also opt in (gated
// on CATCHCHALLENGER_GUI_STATS, defined only for this binary).
#include "../../general/base/CCGuiLog.hpp"

// Qt aborts (qFatal → default handler → abort() → SIGABRT + core
// dump) when no QPA platform plugin can be initialised — typical on
// a headless box with no DISPLAY and QT_QPA_PLATFORM pointing at a
// missing plugin.  A server admin tool must not core-dump on that;
// it should print one plain line and exit non-zero.  We install our
// own message handler BEFORE constructing QApplication: for the
// platform-plugin fatal we print and std::exit() (a handler that
// returns on a fatal makes Qt abort anyway, so we must not return);
// every other message is delegated to whatever handler Qt had so
// CATCHCHALLENGER_HARDENED's direct abort() path is untouched (it
// uses abort(), not qFatal, so it never reaches here).
static QtMessageHandler g_previousMessageHandler=NULL;

static void guiMessageHandler(QtMsgType type,const QMessageLogContext &context,const QString &msg)
{
    if(type==QtFatalMsg && msg.contains(QStringLiteral("platform plugin")))
    {
        std::cerr << "CatchChallenger server-gui: no usable Qt platform plugin "
                     "(headless? run with QT_QPA_PLATFORM=offscreen). Detail: "
                  << msg.toStdString() << std::endl;
        std::exit(EXIT_FAILURE);
    }
    if(g_previousMessageHandler!=NULL)
        g_previousMessageHandler(type,context,msg);
    else
    {
        std::cerr << msg.toStdString() << std::endl;
        if(type==QtFatalMsg)
            std::abort();
    }
}

int main(int argc, char *argv[])
{
    // Wire the GUI log tee BEFORE the first std::cout — every line
    // emitted from here on lands in the textEditConsole ring as well
    // as the original stdout/stderr.  install_stdio_redirect=false
    // keeps the operator's terminal echo for printf/qDebug/Qt plugin
    // loader output; flip it on (e.g. when running with no controlling
    // tty / under testingqtserver --autostart) when you want the GUI
    // console to be the single source of truth.
    CatchChallenger::gui_log::install(/*install_stdio_redirect=*/false);

    NormalServerGlobal::displayInfo();

    // Must be armed before the QApplication ctor: the platform-plugin
    // probe/abort happens inside it.
    g_previousMessageHandler=qInstallMessageHandler(guiMessageHandler);

    QApplication a(argc, argv);
    a.setStyle(QStringLiteral("Fusion"));

    // QSettings needs a stable application id to resolve to the same
    // ~/.config/CatchChallenger/server-gui.conf across launches —
    // MainWindow::wireSettings() relies on the default-constructed
    // QSettings finding the file we just wrote to last time.
    QCoreApplication::setOrganizationName(QStringLiteral("CatchChallenger"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("catchchallenger.herman-brule.com"));
    QCoreApplication::setApplicationName(QStringLiteral("server-gui"));

    // CLI args: --autostart (testingqtserver.py), --screenshot=PATH +
    // --screenshot-delay-ms=N (visual regression), --size=WxH (testing),
    // --auto-quit-after=N (seconds) — self-terminate so the test
    // harness doesn't have to SIGTERM us; pairs with --autostart.
    QString screenshotPath;
    int screenshotDelayMs = 5000;
    int initialWidth = 1100;
    int initialHeight = 750;
    bool wantAutostart = false;
    int autoQuitAfterSecs = 0;
    {
        int i = 1;
        while (i < argc) {
            const QString arg = QString::fromLocal8Bit(argv[i]);
            if (arg.startsWith(QStringLiteral("--screenshot=")))
                screenshotPath = arg.mid(13);
            else if (arg.startsWith(QStringLiteral("--screenshot-delay-ms=")))
                screenshotDelayMs = arg.mid(22).toInt();
            else if (arg.startsWith(QStringLiteral("--size="))) {
                const QString s = arg.mid(7);
                const int x = s.indexOf('x');
                if (x > 0) {
                    initialWidth = s.left(x).toInt();
                    initialHeight = s.mid(x + 1).toInt();
                }
            } else if (arg == QStringLiteral("--autostart"))
                wantAutostart = true;
            else if (arg.startsWith(QStringLiteral("--auto-quit-after=")))
                autoQuitAfterSecs = arg.mid(18).toInt();
            ++i;
        }
    }

    if (argc < 1) {
        std::cerr << "argc<1: wrong arg count" << std::endl;
        return EXIT_FAILURE;
    }
    CatchChallenger::FacilityLibGeneral::applicationDirPath = argv[0];
    CatchChallenger::GlobalServerData::serverSettings.datapack_basePath =
        CatchChallenger::FacilityLibGeneral::getFolderFromFile(
            CatchChallenger::FacilityLibGeneral::applicationDirPath) + "/datapack/";

    // Headless when driven by --autostart or --screenshot=… (the test
    // harness / CI path): there is no operator to dismiss a modal, so a
    // blocking QMessageBox::critical here would freeze the process
    // forever (the wall-watchdog then reports a useless 60s "Start
    // hung" instead of the real "no datapack" cause). Only raise the
    // modal in genuine interactive use; headless always logs the
    // concrete reason and exits non-zero fast.
    const bool headless = wantAutostart || !screenshotPath.isEmpty();

    QFileInfo datapackFolder(QCoreApplication::applicationDirPath()
                             + QStringLiteral("/datapack/informations.xml"));
    if (!datapackFolder.isFile()) {
        if (!headless) {
            // QMessageBox parented to nullptr → uses system theme so the
            // text isn't white-on-white when the MainWindow's dark QSS
            // would otherwise leak in.
            QMessageBox::critical(nullptr, "Critical error",
                QString("No datapack found, look at file: ") + datapackFolder.absoluteFilePath());
        }
        // std::cerr (not qCritical): under the MXE GUI-subsystem build
        // the default Qt handler routes qCritical to OutputDebugString,
        // which is invisible under wine/CI — only std::cerr/std::cout
        // reach the captured console (same channel as the banner).
        std::cerr << "No datapack found, look at file: "
                  << datapackFolder.absoluteFilePath().toStdString()
                  << std::endl;
        return EXIT_FAILURE;
    }

    // Operator wants closing the window to terminate the process — no
    // background "invisible app still running" mode.  Qt's default is
    // already true; spell it out so a future copy-paste from another
    // tray-app project doesn't silently flip it.
    a.setQuitOnLastWindowClosed(true);
    MainWindow w;
    w.setHeadless(headless);
    w.resize(initialWidth, initialHeight);
    // Pure --autostart (no --screenshot) is the headless server/CI
    // path: nothing renders the window and no operator looks at it, so
    // popping a visible MainWindow is just noise (and on a real
    // display it steals focus / lingers). The Qt event loop, the
    // queued autostart() and the TCP server all run fine without
    // show(). Screenshot mode still shows: w.grab() needs the widget
    // tree laid out and painted.
    const bool showWindow = !(wantAutostart && screenshotPath.isEmpty());
    if (showWindow)
        w.show();

    if (!screenshotPath.isEmpty()) {
        QTimer::singleShot(screenshotDelayMs, &w, [&w, screenshotPath]() {
            const QPixmap shot = w.grab();
            if (!shot.save(screenshotPath))
                std::cerr << "--screenshot: failed to save " << screenshotPath.toStdString() << std::endl;
            else
                std::cerr << "--screenshot: saved " << screenshotPath.toStdString()
                          << " (" << shot.width() << "x" << shot.height() << ")" << std::endl;
            QCoreApplication::quit();
        });
    }

    // --autostart routes through MainWindow::autostart() — passed as a
    // pointer-to-member so we don't generate a closure (lambdas captured
    // &w produced a 'main::{lambda()#2}::operator()() const' link error
    // referencing autostart() before it had a body; using the method
    // pointer overload of QTimer::singleShot keeps the link clean).
    if (wantAutostart) {
        QTimer::singleShot(0, &w, &MainWindow::autostart);
    }

    // --auto-quit-after=N: schedule a clean QCoreApplication::quit()
    // N seconds after the event loop starts so testingqtserver.py and
    // friends don't have to SIGTERM the process tree. Method-pointer
    // overload of singleShot — no lambda (CLAUDE.md rule).
    if (autoQuitAfterSecs > 0) {
        std::cout << "[auto-quit-after] will quit in "
                  << autoQuitAfterSecs << "s" << std::endl;
        QTimer::singleShot(autoQuitAfterSecs * 1000,
                           qApp, &QCoreApplication::quit);
    }

    return a.exec();
}
