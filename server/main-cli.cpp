#include <QCoreApplication>
#include "ProcessControler.h"

int main(int argc, char *argv[])
{
    NormalServerGlobal::displayInfo();

    if(argc<1)
    {
        std::cerr << "argc<1: wrong arg count" << std::endl;
        return EXIT_FAILURE;
    }
    CatchChallenger::FacilityLibGeneral::applicationDirPath=argv[0];
    CatchChallenger::GlobalServerData::serverSettings.datapack_basePath=CatchChallenger::FacilityLibGeneral::getFolderFromFile(CatchChallenger::FacilityLibGeneral::applicationDirPath)+"/datapack/";

    QCoreApplication a(argc, argv);

    QFileInfo datapackFolder(QCoreApplication::applicationDirPath()+QStringLiteral("/datapack/informations.xml"));
    if(!datapackFolder.isFile())
    {
        qDebug() << "No datapack found into: " << datapackFolder.absoluteFilePath();
        return EXIT_FAILURE;
    }

    ProcessControler w;
    Q_UNUSED(w);
    return a.exec();
}
