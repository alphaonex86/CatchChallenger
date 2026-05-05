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
#include "../../general/base/FacilityLibGeneral.hpp"

int main(int argc, char *argv[])
{
    NormalServerGlobal::displayInfo();

    QApplication a(argc, argv);
    if(argc<1)
    {
        std::cerr << "argc<1: wrong arg count" << std::endl;
        return EXIT_FAILURE;
    }
    CatchChallenger::FacilityLibGeneral::applicationDirPath=argv[0];
    CatchChallenger::GlobalServerData::serverSettings.datapack_basePath=CatchChallenger::FacilityLibGeneral::getFolderFromFile(CatchChallenger::FacilityLibGeneral::applicationDirPath)+"/datapack/";

    QFileInfo datapackFolder(QCoreApplication::applicationDirPath()+QStringLiteral("/datapack/informations.xml"));
    if(!datapackFolder.isFile())
    {
        QMessageBox::critical(NULL,"Critical error",QString("No datapack found, look at file: ")+datapackFolder.absoluteFilePath());
        qDebug() << "No datapack found, look at file: " << datapackFolder.absoluteFilePath();
        return EXIT_FAILURE;
    }

    a.setQuitOnLastWindowClosed(false);
    CatchChallenger::MainWindow w;
    w.show();

    // --autostart: programmatic equivalent of clicking "Start server" on
    // the first event-loop tick, so testingqtserver.py can boot the
    // server headlessly and see the "Listen *:<port>" success marker
    // in stdout. Without this flag the binary just opens the window
    // and waits for the operator to click.
    int i=1;
    while(i<argc)
    {
        if(argv[i]!=nullptr && std::strcmp(argv[i],"--autostart")==0)
        {
            QTimer::singleShot(0,&w,[&w](){
                std::cout << "[autostart] clicking Start" << std::endl;
                std::cout.flush();
                w.autostart();
            });
            break;
        }
        ++i;
    }

    return a.exec();
}
