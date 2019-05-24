#include "OptionsV.h"
#include "../../client/base/LanguagesSelect.h"

#include <QApplication>
#include <QDebug>
#include <QFileDialog>

int main(int argc, char *argv[])
{
    // Avoid performance issues with X11 engine when rendering objects
#ifdef Q_WS_X11
    QApplication::setGraphicsSystem(QStringLiteral("raster"));
#endif

    QApplication a(argc, argv);
    LanguagesSelect::languagesSelect=new LanguagesSelect();
    a.setOrganizationDomain(QStringLiteral("catchchallenger"));
    a.setApplicationName(QStringLiteral("Map visualiser"));
    a.setApplicationVersion(QStringLiteral("1.0"));

    OptionsV options;
    options.show();

    return a.exec();
}
