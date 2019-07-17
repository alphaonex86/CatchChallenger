#include <QApplication>
#include <QGraphicsEllipseItem>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QTimer>
#include "MainWindow.h"
#include "CCBackground.h"

int main( int argc, char **argv )
{
    QApplication app(argc, argv);

    QGraphicsScene* scene = new QGraphicsScene();
    MainWindow* view = new MainWindow(scene);

    /*QGraphicsPixmapItem* item = new QGraphicsPixmapItem(QPixmap(":/quit.png").scaled(1900,1000));
    scene->addItem(item);*/
    CCBackground *background=new CCBackground();
    scene->addItem(background);

    view->setFrameShape(QFrame::NoFrame);
    view->show();

    return app.exec();
}
