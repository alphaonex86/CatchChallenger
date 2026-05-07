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
#include "MainWindow.hpp"
#include "../base/NormalServerGlobal.hpp"
#include "../base/GlobalServerData.hpp"
#include "../../general/base/FacilityLibGeneral.hpp"

int main(int argc, char *argv[])
{
    NormalServerGlobal::displayInfo();

    QApplication a(argc, argv);
    a.setStyle(QStringLiteral("Fusion"));

    // QSettings needs a stable application id to resolve to the same
    // ~/.config/CatchChallenger/server-gui.conf across launches —
    // MainWindow::wireSettings() relies on the default-constructed
    // QSettings finding the file we just wrote to last time.
    QCoreApplication::setOrganizationName(QStringLiteral("CatchChallenger"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("catchchallenger.first-world.info"));
    QCoreApplication::setApplicationName(QStringLiteral("server-gui"));

    // CLI args: --autostart (testingqtserver.py), --screenshot=PATH +
    // --screenshot-delay-ms=N (visual regression), --size=WxH (testing).
    QString screenshotPath;
    int screenshotDelayMs = 5000;
    int initialWidth = 1100;
    int initialHeight = 750;
    bool wantAutostart = false;
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

    QFileInfo datapackFolder(QCoreApplication::applicationDirPath()
                             + QStringLiteral("/datapack/informations.xml"));
    if (!datapackFolder.isFile()) {
        // QMessageBox parented to nullptr → uses system theme so the
        // text isn't white-on-white when the MainWindow's dark QSS
        // would otherwise leak in.
        QMessageBox::critical(nullptr, "Critical error",
            QString("No datapack found, look at file: ") + datapackFolder.absoluteFilePath());
        qDebug() << "No datapack found, look at file: " << datapackFolder.absoluteFilePath();
        return EXIT_FAILURE;
    }

    a.setQuitOnLastWindowClosed(false);
    MainWindow w;
    w.resize(initialWidth, initialHeight);
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

    return a.exec();
}
