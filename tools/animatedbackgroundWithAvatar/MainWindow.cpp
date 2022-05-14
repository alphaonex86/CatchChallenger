#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "CustomButton.h"
#include "CCprogressbar.h"
#include "CCWidget.h"

#include <QPushButton>
#include <QGLFormat>
#include <QGLWidget>
#include <iostream>

MainWindow::MainWindow() :
    mScene(new QGraphicsScene(this))
{
    /*void QGraphicsSceneMouseEvent::mousePressedEvent(*e){
int x = e.scenePos().x();
qDebug()x;
}*/
 /*   QGraphicsView::mousePressEvent(QGraphicsSceneMouseEvent *event){
qDebug()<<event->scenePos();
}*/
    //ui->setupUi(this);

    m_CCBackground=new CCBackground();
    m_CCBackground->setPos(0,0);
    setMinimumHeight(60);
    setMinimumWidth(60);

    /*button=new CustomButton(":/quit.png");
    button->setText(tr("X"));
    if(!connect(button,&CustomButton::clicked,&QCoreApplication::quit))
        abort();
    button->setPos(0,0);*/
    //button->setMaximumSize(200,50);
    //ui->verticalLayout->addWidget(p);
    setStyleSheet("");

    /*CCprogressbar *progressbar=new CCprogressbar();
    progressbar->setValue(25);
    ui->verticalLayout->addWidget(progressbar);
    progressbar->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::MinimumExpanding);
    progressbar->setTextVisible(true);

    CCWidget *widget=new CCWidget();
    ui->verticalLayout->addWidget(widget);
    widget->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::MinimumExpanding);*/

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

    frameCounter=0;
    timeRender.restart();
    waitRenderTime=40;
    timerRender.setSingleShot(true);
    if(!connect(&timerRender,&QTimer::timeout,this,&MainWindow::render))
        abort();

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

    /*simpleText=new QGraphicsTextItem("<span style=\"color:red\">test</span>");
    mScene->addItem(simpleText);
    simpleText->setPos(0,0);
    QPixmap p(":/f.png");
    if(p.isNull())
        abort();
    image=new QGraphicsPixmapItem(p);
    mScene->addItem(image);
    image->setPos(0,0);

    imageText=new QGraphicsPixmapItem(p);
    mScene->addItem(imageText);
    imageText->setPos(0,0);
    //mScene->setSceneRect(QRectF(xPerso*TILE_SIZE,yPerso*TILE_SIZE,64,32));
    mScene->setSceneRect(QRectF(0,0,width(),height()));*/

    //mScene->addItem(m_CCBackground);
    //mScene->addItem(button);

    startRender.start(5000);
    startRender.setSingleShot(true);
    connect(&startRender,&QTimer::timeout,this,&MainWindow::startRenderSlot);

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
    /*const uint8_t x=event->x();
    const uint8_t y=event->y();
    std::cerr << "void MainWindow::mousePressEvent(QGraphicsSceneMouseEvent *event) " << x << "," << y << std::endl;*/
    /*if(button->boundingRect().contains(x,y))
        button->setPressed(true);*/
}

void MainWindow::mouseReleaseEvent(QMouseEvent *event)
{
    /*const uint8_t x=event->x();
    const uint8_t y=event->y();
    std::cerr << "void MainWindow::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) " << x << "," << y << std::endl;*/
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
    timeRender.restart();

    QGraphicsView::paintEvent(event);

    uint32_t elapsed=timeRender.elapsed();
    if(waitRenderTime<=elapsed)
        timerRender.start(1);
    else
        timerRender.start(waitRenderTime-elapsed);

    if(frameCounter<65535)
        frameCounter++;
}

void MainWindow::startRenderSlot()
{
    mScene->addItem(m_CCBackground);
}
