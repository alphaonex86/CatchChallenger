#include "MainWindow.h"
#include <QGLWidget>

MainWindow::MainWindow(QWidget *parent) :
    QGraphicsView(parent)
{
    setViewport(new QGLWidget(QGLFormat(QGL::SampleBuffers | QGL::DoubleBuffer)));
    if(!QObject::connect(&timer,&QTimer::timeout,this,&MainWindow::update))
        abort();
    timer.start(20);
    setMinimumHeight(120);
    setMinimumWidth(120);
    /* no performance change
    setCacheMode(CacheBackground);
    setOptimizationFlag(IndirectPainting);
    setOptimizationFlag(DontSavePainterState);
    setOptimizationFlag(DontAdjustForAntialiasing);*/
}

MainWindow::MainWindow(QGraphicsScene *scene, QWidget *parent) :
    QGraphicsView(scene,parent)
{
    if(!QObject::connect(&timer,&QTimer::timeout,this,&MainWindow::update))
        abort();
    timer.start(20);
}

MainWindow::~MainWindow()
{
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    scene()->setSceneRect(0,0,width(),height());
    QGraphicsView::resizeEvent(event);
}

void MainWindow::update()
{
    //QGraphicsView::update();
    scene()->update();
}
