#include <QApplication>
#include "MainWindow.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QFileInfo datapackFolder(QCoreApplication::applicationDirPath()+QLatin1Literal("/datapack/informations.xml"));
    if(!datapackFolder.isFile())
    {
        qDebug() << "No datapack found into: " << datapackFolder.absoluteFilePath();
        return EXIT_FAILURE;
    }

    a.setQuitOnLastWindowClosed(false);
    CatchChallenger::MainWindow w;
    w.show();

    return a.exec();
}
