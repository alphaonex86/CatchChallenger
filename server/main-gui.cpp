#include <QApplication>
#include <QMessageBox>
#include <QString>
#include "MainWindow.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

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
