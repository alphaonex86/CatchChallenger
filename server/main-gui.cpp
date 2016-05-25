#include <QApplication>
#include <QMessageBox>
#include <string>
#include "MainWindow.h"

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

    QFileInfo datapackFolder(QCoreApplication::applicationDirPath()+QLatin1Literal("/datapack/informations.xml"));
    if(!datapackFolder.isFile())
    {
        QMessageBox::critical(NULL,"Critical error",QString("No datapack found, look at file: ")+datapackFolder.absoluteFilePath());
        qDebug() << "No datapack found, look at file: " << datapackFolder.absoluteFilePath();
        return EXIT_FAILURE;
    }

    a.setQuitOnLastWindowClosed(false);
    CatchChallenger::MainWindow w;
    w.show();

    return a.exec();
}
