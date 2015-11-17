#include <QApplication>
#include "MainWindow.h"

int main(int argc, char *argv[])
{
    if(argc<1)
    {
        std::cerr << "argc<1: wrong arg count" << std::endl;
        return EXIT_FAILURE;
    }
    CatchChallenger::FacilityLibGeneral::applicationDirPath=argv[0];
    
    QApplication a(argc, argv);
    a.setQuitOnLastWindowClosed(false);
    CatchChallenger::MainWindow w;
    w.show();

    return a.exec();
}
