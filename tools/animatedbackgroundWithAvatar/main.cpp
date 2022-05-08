#include "MainWindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QFont font("Comic Sans MS");
    font.setStyleHint(QFont::Monospace);
    font.setBold(true);
    //font.setPixelSize(20);
    QApplication::setFont(font);

    MainWindow w;
    w.show();
    w.resize(1920,1080);
    //w.resize(420,360);

    return a.exec();
}
