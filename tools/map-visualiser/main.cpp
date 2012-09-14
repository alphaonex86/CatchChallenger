#include "Options.h"

#include <QApplication>
#include <QDebug>
#include <QFileDialog>

int main(int argc, char *argv[])
{
    // Avoid performance issues with X11 engine when rendering objects
#ifdef Q_WS_X11
    QApplication::setGraphicsSystem(QLatin1String("raster"));
#endif

    QApplication a(argc, argv);

    a.setOrganizationDomain(QLatin1String("pokecraft"));
    a.setApplicationName(QLatin1String("Map visualiser"));
    a.setApplicationVersion(QLatin1String("1.0"));

    Options options;
    options.show();

    return a.exec();
}
