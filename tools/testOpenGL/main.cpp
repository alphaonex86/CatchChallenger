#include "../../client/qt/LanguagesSelect.h"
#include "../../client/qt/QtDatapackClientLoader.h"
#include "MapControllerV.h"

#include <QApplication>
#include <QDebug>
#include <QFileDialog>
#include <QStandardPaths>

int main(int argc, char *argv[])
{
    // Avoid performance issues with X11 engine when rendering objects
    qRegisterMetaType<std::string>("std::string");

    QApplication a(argc, argv);
    LanguagesSelect::languagesSelect=new LanguagesSelect();
    a.setOrganizationDomain(QStringLiteral("catchchallenger"));
    a.setApplicationName(QStringLiteral("TestOpenGL"));
    a.setApplicationVersion(QStringLiteral("1.0"));
    QtDatapackClientLoader::datapackLoader=new QtDatapackClientLoader();
    QString path=QStandardPaths::writableLocation(QStandardPaths::DataLocation);
    QDir().mkpath(path);
    QFileInfoList files=QDir(":/datapack/").entryInfoList();
    {
        int index=0;
        while(index<files.size())
        {
            QString source=files.at(index).absoluteFilePath();
            QString destination=path+"/"+files.at(index).fileName();
            QFile::copy(source,destination);
            index++;
        }
    }

    MapControllerV *mapController=new MapControllerV(true,false,true,true);

    //mapController->setShowFPS(ui->showFPS->isChecked());
    mapController->setScale(4);
    mapController->setTargetFPS(25);

    mapController->current_map=path.toStdString()+"/city.tmx";
    mapController->viewMap(path+"/city.tmx");

    const int r=a.exec();
    delete QtDatapackClientLoader::datapackLoader;
    return r;
}
