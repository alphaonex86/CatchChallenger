#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "CustomButton.h"
#include "CCprogressbar.h"
#include "CCWidget.h"

#include <QPushButton>
#include <QGLFormat>
#include <QGLWidget>

MainWindow::MainWindow() :
    mScene(new QGraphicsScene(this))
{
    //ui->setupUi(this);

    m_CCBackground=new CCBackground();
    m_CCBackground->setPos(0,0);
    setMinimumHeight(60);
    setMinimumWidth(60);

    /*p=new CustomButton(":/quit.png",m_CCBackground);
    p->setText(tr("Normal"));
    if(!connect(p,&CustomButton::clicked,&QCoreApplication::quit))
        abort();
    //ui->verticalLayout->addWidget(p);
    setStyleSheet("");*/

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
    setRenderHint(QPainter::Antialiasing,false);
    setRenderHint(QPainter::TextAntialiasing,false);
    setRenderHint(QPainter::HighQualityAntialiasing,false);
    setRenderHint(QPainter::SmoothPixmapTransform,false);
    setRenderHint(QPainter::NonCosmeticDefaultPen,true);
    show();

    timerUpdateFPS.setSingleShot(true);
    timerUpdateFPS.setInterval(1000);
    timeUpdateFPS.restart();
    frameCounter=0;
    timeRender.restart();
    waitRenderTime=40;
    timerRender.setSingleShot(true);
    if(!connect(&timerRender,&QTimer::timeout,this,&MainWindow::render))
        abort();
    if(!connect(&timerUpdateFPS,&QTimer::timeout,this,&MainWindow::updateFPS))
        abort();
    timerUpdateFPS.start();

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

    simpleText=new QGraphicsTextItem("<span style=\"color:red\">test</span>");
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
    mScene->setSceneRect(QRectF(0,0,width(),height()));

    mScene->addItem(m_CCBackground);

    render();

    setTargetFPS(100);
}

MainWindow::~MainWindow()
{
    //delete ui;
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    mScene->setSceneRect(QRectF(0,0,width(),height()));
    (void)event;
    /*m_CCBackground->setMinimumSize(size());
    m_CCBackground->setMaximumSize(size());

    QString text;
    switch (rand()%4) {
    case 0:
        text="f";
        break;
    case 1:
        text="quit";
        break;
    default:
        text="фхцчшщ";
        break;
    }*/
    /*const int w=rand()%173+50;
    const int h=rand()%73+20;
    p->setMinimumWidth(w);
    p->setMinimumHeight(h);
    p->setMaximumWidth(w);
    p->setMaximumHeight(h);*/
    //p->setText(text);
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

void MainWindow::updateFPS()
{
    const unsigned int FPS=(int)(((float)frameCounter)*1000)/timeUpdateFPS.elapsed();
    emit newFPSvalue(FPS);
    QImage pix(50,40,QImage::Format_ARGB32_Premultiplied);
    pix.fill(Qt::transparent);
    QPainter p(&pix);
    p.setPen(QPen(Qt::green));
    p.setFont(QFont("Times", 12, QFont::Bold));
    p.drawText(pix.rect(), Qt::AlignCenter,QString::number(FPS));
    imageText->setPixmap(QPixmap::fromImage(pix));

    frameCounter=0;
    timeUpdateFPS.restart();
    timerUpdateFPS.start();
}

void MainWindow::setTargetFPS(int targetFPS)
{
    if(targetFPS==0)
        waitRenderTime=0;
    else
    {
        waitRenderTime=static_cast<uint8_t>(static_cast<float>(1000.0)/(float)targetFPS);
        if(waitRenderTime<1)
            waitRenderTime=1;
    }
}

