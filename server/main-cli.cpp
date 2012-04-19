#include <QtGui/QApplication>
#include "ProcessControler.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    ProcessControler w;
    return a.exec();
}
