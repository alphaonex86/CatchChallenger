#include <QCoreApplication>
#include "ProcessControler.h"

int main(int argc, char *argv[])
{
    NormalServer::displayInfo();
    
    if(argc<1)
    {
        std::cerr << "argc<1: wrong arg count" << std::endl;
        return EXIT_FAILURE;
    }
    CatchChallenger::FacilityLibGeneral::applicationDirPath=argv[0];
    
    QCoreApplication a(argc, argv);

    QFileInfo datapackFolder(QCoreApplication::applicationDirPath()+QLatin1Literal("/datapack/informations.xml"));
    if(!datapackFolder.isFile())
    {
        qDebug() << "No datapack found into: " << datapackFolder.absoluteFilePath();
        return EXIT_FAILURE;
    }

    ProcessControler w;
    Q_UNUSED(w);
    return a.exec();
}
