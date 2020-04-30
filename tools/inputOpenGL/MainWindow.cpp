#include "MainWindow.h"
#include "ui_MainWindow.h"

#include <QGLFormat>
#include <QGLWidget>
#include <iostream>

MainWindow::MainWindow() :
    mScene(new QGraphicsScene(this))
{
    input=new CCTextLineInput();
    input->setPos(0,0);
    setMinimumHeight(60);
    setMinimumWidth(60);
    mScene->addItem(input);

    //showFullScreen();
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    setViewport(new QGLWidget(QGLFormat(QGL::DoubleBuffer)));
    /*setRenderHint(QPainter::Antialiasing,false);
    setRenderHint(QPainter::TextAntialiasing,false);
    setRenderHint(QPainter::HighQualityAntialiasing,false);
    setRenderHint(QPainter::SmoothPixmapTransform,false);
    setRenderHint(QPainter::NonCosmeticDefaultPen,true);*/
    show();

    setScene(mScene);
/*    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setDragMode(QGraphicsView::ScrollHandDrag);*/
    setOptimizationFlags(QGraphicsView::DontAdjustForAntialiasing | QGraphicsView::DontSavePainterState | QGraphicsView::IndirectPainting);
    setBackgroundBrush(Qt::black);
    setFrameStyle(0);

    viewport()->setAttribute(Qt::WA_StaticContents);
    viewport()->setAttribute(Qt::WA_TranslucentBackground);
    viewport()->setAttribute(Qt::WA_NoSystemBackground);
    setViewportUpdateMode(QGraphicsView::NoViewportUpdate);

    render();
}

MainWindow::~MainWindow()
{
    //delete ui;
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    mScene->setSceneRect(QRectF(0,0,width(),height()));
    (void)event;
}

void MainWindow::mousePressEvent(QMouseEvent *event)
{
    const uint8_t x=event->x();
    const uint8_t y=event->y();
    std::cerr << "void MainWindow::mousePressEvent(QGraphicsSceneMouseEvent *event) " << x << "," << y << std::endl;
    /*if(button->boundingRect().contains(x,y))
        button->setPressed(true);*/
}

void MainWindow::mouseReleaseEvent(QMouseEvent *event)
{
    const uint8_t x=event->x();
    const uint8_t y=event->y();
    std::cerr << "void MainWindow::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) " << x << "," << y << std::endl;
    /*if(button->boundingRect().contains(x,y))
        button->emitclicked();
    button->setPressed(false);*/
}

void MainWindow::render()
{
    mScene->setSceneRect(QRectF(0,0,width(),height()));
    //mScene->update();
    viewport()->update();
}

void MainWindow::paintEvent(QPaintEvent * event)
{
    mScene->setSceneRect(QRectF(0,0,width(),height()));
    QGraphicsView::paintEvent(event);
}
