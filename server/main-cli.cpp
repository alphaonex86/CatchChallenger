#include <QCoreApplication>
#include "ProcessControler.h"

int main(int argc, char *argv[])
{
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
