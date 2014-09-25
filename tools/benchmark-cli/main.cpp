#include "MainBenchmark.h"
#include <QCoreApplication>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    a.setApplicationName("benchmark-gui");
    a.setOrganizationName("CatchChallenger");
    MainBenchmark benchMark;
    Q_UNUSED(benchMark);

    return a.exec();
}
