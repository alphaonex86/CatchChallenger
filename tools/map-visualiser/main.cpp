#include "OptionsV.h"
#include "../../client/qt/LanguagesSelect.h"
#include "../../client/qt/QtDatapackClientLoader.h"

#include <QApplication>
#include <QDebug>
#include <QFileDialog>

int main(int argc, char *argv[])
{
    // Avoid performance issues with X11 engine when rendering objects
#ifdef Q_WS_X11
    QApplication::setGraphicsSystem(QStringLiteral("raster"));
#endif
    qRegisterMetaType<std::string>("std::string");

    QApplication a(argc, argv);
    a.setQuitOnLastWindowClosed(false);
    LanguagesSelect::languagesSelect=new LanguagesSelect();
    a.setOrganizationDomain(QStringLiteral("catchchallenger"));
    a.setApplicationName(QStringLiteral("Map visualiser"));
    a.setApplicationVersion(QStringLiteral("1.0"));
    QtDatapackClientLoader::datapackLoader=new QtDatapackClientLoader();

    OptionsV options;
    options.show();

    const int r=a.exec();
    delete QtDatapackClientLoader::datapackLoader;
    return r;
}
