#include <QApplication>
#include <QGraphicsEllipseItem>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QTimer>
#include "MainWindow.h"
#include "CCBackground.h"
#include "CustomButton.h"

int main( int argc, char **argv )
{
    QApplication app(argc, argv);

    QGraphicsScene* scene = new QGraphicsScene();
    MainWindow* view = new MainWindow(scene);

    /*QGraphicsPixmapItem* item = new QGraphicsPixmapItem(QPixmap(":/quit.png").scaled(1900,1000));
    scene->addItem(item);*/
    CCBackground *background=new CCBackground();
    //background->setOpacity(0.5);
    scene->addItem(background);
    //scene->setItemIndexMethod(QGraphicsScene::NoIndex);
    view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    CustomButton *button=new CustomButton(":/quit.png",background);
    //scene->addItem(button);

    view->setFrameShape(QFrame::NoFrame);
    view->show();

    return app.exec();
}
