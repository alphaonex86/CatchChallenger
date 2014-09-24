#include "MainBenchmark.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setApplicationName("benchmark-gui");
    a.setOrganizationName("CatchChallenger");
    a.setQuitOnLastWindowClosed(false);
    MainBenchmark benchMark;
    Q_UNUSED(benchMark);

    return a.exec();
}
