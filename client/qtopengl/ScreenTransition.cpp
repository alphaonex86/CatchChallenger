#include "ScreenTransition.h"
#include "../qt/GameLoader.h"
#include "../../general/base/Version.h"
#include "AudioGL.h"
#include <iostream>
#include <QGLWidget>
#ifdef Q_OS_ANDROID
#include <QtAndroidExtras>

void keep_screen_on(bool on) {
  QtAndroid::runOnAndroidThread([on]{
    QAndroidJniObject activity = QtAndroid::androidActivity();
    if (activity.isValid()) {
      QAndroidJniObject window =
          activity.callObjectMethod("getWindow", "()Landroid/view/Window;");

      if (window.isValid()) {
        const int FLAG_KEEP_SCREEN_ON = 128;
        if (on) {
          window.callMethod<void>("addFlags", "(I)V", FLAG_KEEP_SCREEN_ON);
        } else {
          window.callMethod<void>("clearFlags", "(I)V", FLAG_KEEP_SCREEN_ON);
        }
      }
    }
    QAndroidJniEnvironment env;
    if (env->ExceptionCheck()) {
      env->ExceptionClear();
    }
  });
}
#endif

ScreenTransition::ScreenTransition() :
    mScene(new QGraphicsScene(this))
{
    setViewport(new QGLWidget(QGLFormat(QGL::SampleBuffers)));
    setRenderHint(QPainter::Antialiasing,true);
    setRenderHint(QPainter::TextAntialiasing,true);
    setRenderHint(QPainter::HighQualityAntialiasing,true);
    setRenderHint(QPainter::SmoothPixmapTransform,false);
    setRenderHint(QPainter::NonCosmeticDefaultPen,true);

    mousePress=nullptr;
    m_backgroundStack=nullptr;
    m_foregroundStack=nullptr;
    m_aboveStack=nullptr;
    setBackground(&b);
    setForeground(&l);
    if(!connect(&l,&LoadingScreen::finished,this,&ScreenTransition::toMainScreen))
        abort();
    m=nullptr;
    o=nullptr;
    /*solo=nullptr;*/
    multi=nullptr;
    login=nullptr;

    timerUpdateFPS.setSingleShot(true);
    timerUpdateFPS.setInterval(1000);
    timeUpdateFPS.restart();
    frameCounter=0;
    timeRender.restart();
    waitRenderTime=40;
    timerRender.setSingleShot(true);
    if(!connect(&timerRender,&QTimer::timeout,this,&ScreenTransition::render))
        abort();
    if(!connect(&timerUpdateFPS,&QTimer::timeout,this,&ScreenTransition::updateFPS))
        abort();
    timerUpdateFPS.start();

    setScene(mScene);
    mScene->setSceneRect(QRectF(0,0,width(),height()));
    setOptimizationFlags(QGraphicsView::DontAdjustForAntialiasing | QGraphicsView::DontSavePainterState | QGraphicsView::IndirectPainting);
    setBackgroundBrush(Qt::black);
    setFrameStyle(0);
    viewport()->setAttribute(Qt::WA_StaticContents);
    viewport()->setAttribute(Qt::WA_TranslucentBackground);
    viewport()->setAttribute(Qt::WA_NoSystemBackground);
    setViewportUpdateMode(QGraphicsView::NoViewportUpdate);

    #ifdef Q_OS_ANDROID
    keep_screen_on(true);
    #endif

    imageText=new QGraphicsPixmapItem();
    mScene->addItem(imageText);
    imageText->setPos(0,0);

    render();
    setTargetFPS(100);
}

ScreenTransition::~ScreenTransition()
{
}

void ScreenTransition::resizeEvent(QResizeEvent *event)
{
    mScene->setSceneRect(QRectF(0,0,width(),height()));
    (void)event;
}

void ScreenTransition::mousePressEvent(QMouseEvent *event)
{
    /*const QPointF p=mapToScene(event->pos());
    const qreal x=p.x();
    const qreal y=p.y();*/
/*    std::cerr << "void ScreenTransition::mousePressEvent(QGraphicsSceneMouseEvent *event) "
              << std::to_string(x) << "," << std::to_string(y) << std::endl;*/
    /*if(button->boundingRect().contains(x,y))
        button->setPressed(true);*/
    const QPointF &p=mapToScene(event->pos());
    if(mousePress!=nullptr)
    {
        bool temp=true;//don¡t do action if true
        static_cast<ScreenInput *>(mousePress)->mouseReleaseEventXY(p,temp);
        mousePress=nullptr;
    }
    bool temp=false;
    if(m_aboveStack!=nullptr)
    {
        static_cast<ScreenInput *>(m_aboveStack)->mousePressEventXY(p,temp);
        mousePress=m_aboveStack;
    }
    else if(m_foregroundStack!=nullptr)
    {
        static_cast<ScreenInput *>(m_foregroundStack)->mousePressEventXY(p,temp);
        mousePress=m_foregroundStack;
    }
}

void ScreenTransition::mouseReleaseEvent(QMouseEvent *event)
{
    /*const QPointF &p=mapToScene(event->pos());
    const qreal x=p.x();
    const qreal y=p.y();*/
    /*std::cerr << "void ScreenTransition::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) "
              << std::to_string(x) << "," << std::to_string(y) << std::endl;*/
    /*if(button->boundingRect().contains(x,y))
        button->emitclicked();
    button->setPressed(false);*/
    const QPointF &p=mapToScene(event->pos());
    mousePress=nullptr;
    bool pressValidated=false;
    if(m_aboveStack!=nullptr)
    {
        m_aboveStack->mouseReleaseEventXY(p,pressValidated);
        mousePress=m_aboveStack;
    }
    if(m_foregroundStack!=nullptr)
    {
        m_foregroundStack->mouseReleaseEventXY(p,pressValidated);
        mousePress=m_foregroundStack;
    }
}

void ScreenTransition::mouseMoveEvent(QMouseEvent *event)
{
    /*const QPointF &p=mapToScene(event->pos());
    const qreal x=p.x();
    const qreal y=p.y();*/
    /*std::cerr << "void ScreenTransition::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) "
              << std::to_string(x) << "," << std::to_string(y) << std::endl;*/
    /*if(button->boundingRect().contains(x,y))
        button->emitclicked();
    button->setPressed(false);*/
    const QPointF &p=mapToScene(event->pos());
    mousePress=nullptr;
    bool pressValidated=false;
    if(m_aboveStack!=nullptr)
    {
        m_aboveStack->mouseMoveEventXY(p,pressValidated);
        mousePress=m_aboveStack;
    }
    if(m_foregroundStack!=nullptr)
    {
        m_foregroundStack->mouseMoveEventXY(p,pressValidated);
        mousePress=m_foregroundStack;
    }
}

void ScreenTransition::setBackground(ScreenInput *widget)
{
    if(m_backgroundStack!=nullptr)
        mScene->removeItem(m_backgroundStack);
    m_backgroundStack=widget;
    if(widget!=nullptr)
        mScene->addItem(m_backgroundStack);
}

void ScreenTransition::setForeground(ScreenInput *widget)
{
    if(mousePress!=nullptr)
    {
        bool temp=true;//don¡t do action if true
        static_cast<ScreenInput *>(mousePress)->mouseReleaseEventXY(QPointF(0.0,0.0),temp);
        mousePress=nullptr;
    }
    if(m_foregroundStack!=nullptr)
        mScene->removeItem(m_foregroundStack);
    m_foregroundStack=widget;
    if(widget!=nullptr)
        mScene->addItem(m_foregroundStack);
}

void ScreenTransition::setAbove(ScreenInput *widget)
{
    if(mousePress!=nullptr)
    {
        bool temp=true;//don¡t do action if true
        static_cast<ScreenInput *>(mousePress)->mouseReleaseEventXY(QPointF(0.0,0.0),temp);
        mousePress=nullptr;
    }
    if(m_aboveStack!=nullptr)
        mScene->removeItem(m_aboveStack);
    m_aboveStack=widget;
    if(widget!=nullptr)
        mScene->addItem(m_aboveStack);
}

void ScreenTransition::toMainScreen()
{
    if(Audio::audio==nullptr)
        Audio::audio=new AudioGL();
    if(m==nullptr)
    {
        m=new MainScreen();
        if(!connect(m,&MainScreen::goToOptions,this,&ScreenTransition::openOptions))
            abort();
        if(!connect(m,&MainScreen::goToSolo,this,&ScreenTransition::openSolo))
            abort();
        if(!connect(m,&MainScreen::goToMulti,this,&ScreenTransition::openMulti))
            abort();
    }
    setForeground(m);
    setWindowTitle(tr("CatchChallenger %1").arg(QString::fromStdString(CatchChallenger::Version::str)));
}

void ScreenTransition::openOptions()
{
    if(o==nullptr)
    {
        o=new OptionsDialog();
        if(!connect(o,&OptionsDialog::quitOption,this,&ScreenTransition::closeOptions))
            abort();
    }
    setAbove(o);
}

void ScreenTransition::closeOptions()
{
    setAbove(nullptr);
}

void ScreenTransition::openSolo()
{
    /*if(solo==nullptr)
    {
        solo=new Solo(this);
        if(!connect(solo,&Solo::backMain,this,&ScreenTransition::backMain))
            abort();
    }
    setForeground(solo);*/
}

void ScreenTransition::openMulti()
{
/*    if(multi==nullptr)
    {
        multi=new Multi(this);
        if(!connect(multi,&Multi::backMain,this,&ScreenTransition::backMain))
            abort();
        if(!connect(multi,&Multi::setAbove,this,&ScreenTransition::setAbove))
            abort();
        if(!connect(multi,&Multi::connectToServer,this,&ScreenTransition::connectToServer))
            abort();
    }
    setForeground(multi);*/
}

void ScreenTransition::connectToServer(Multi::ConnexionInfo connexionInfo,QString login,QString pass)
{
    Q_UNUSED(connexionInfo);
    Q_UNUSED(login);
    Q_UNUSED(pass);
    setForeground(&l);
    //baseWindow=new CatchChallenger::BaseWindow();
    connexionManager=new ConnexionManager(baseWindow,&l);
    connexionManager->connectToServer(connexionInfo,login,pass);
    if(!connect(connexionManager,&ConnexionManager::logged,this,&ScreenTransition::logged))
        abort();
    if(!connect(connexionManager,&ConnexionManager::errorString,this,&ScreenTransition::errorString))
        abort();
    if(!connect(connexionManager,&ConnexionManager::disconnectedFromServer,this,&ScreenTransition::disconnectedFromServer))
        abort();
    /*if(!connect(baseWindow,&CatchChallenger::BaseWindow::toLoading,this,&ScreenTransition::toLoading))
        abort();
    if(!connect(baseWindow,&CatchChallenger::BaseWindow::goToServerList,this,&ScreenTransition::goToServerList))
        abort();
    if(!connect(baseWindow,&CatchChallenger::BaseWindow::goToMap,this,&ScreenTransition::goToMap))
        abort();*/
    l.progression(0,100);
}

/*void ScreenTransition::errorString(std::string error)
{
    setForeground(m);
    m->setError(error);
}*/

void ScreenTransition::toLoading(QString text)
{
    if(m_foregroundStack!=&l)
        l.progression(0,100);
    l.setText(text);
    setBackground(&b);
    setForeground(&l);
}

void ScreenTransition::backMain()
{
    setForeground(m);
}

void ScreenTransition::logged(const std::vector<std::vector<CatchChallenger::CharacterEntry> > &characterEntryList)
{
    Q_UNUSED(characterEntryList);
    setBackground(nullptr);
    //setForeground(baseWindow);
    //baseWindow->updateConnectingStatus();
}

void ScreenTransition::disconnectedFromServer()
{
    //baseWindow->resetAll();
    setBackground(&b);
    //setForeground(m);
    setAbove(nullptr);
}

void ScreenTransition::errorString(std::string error)
{
    setBackground(&b);
    //setForeground(m);
    setAbove(nullptr);
    if(m!=nullptr)
        m->setError(error);
}

void ScreenTransition::goToServerList()
{
    setBackground(&b);
    //setForeground(baseWindow);
    setAbove(nullptr);
}

void ScreenTransition::goToMap()
{
    setBackground(&b);
    //setForeground(baseWindow);
    setAbove(nullptr);
}

void ScreenTransition::render()
{
    mScene->setSceneRect(QRectF(0,0,width(),height()));
    //mScene->update();
    viewport()->update();
}

void ScreenTransition::paintEvent(QPaintEvent * event)
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

void ScreenTransition::updateFPS()
{
    const unsigned int FPS=(int)(((float)frameCounter)*1000)/timeUpdateFPS.elapsed();
    emit newFPSvalue(FPS);
    QImage pix(50,40,QImage::Format_ARGB32_Premultiplied);
    pix.fill(Qt::transparent);
    QPainter p(&pix);
    p.setFont(QFont("Times", 12, QFont::Bold));

    const int offset=1;
    p.setPen(QPen(Qt::black));
    p.drawText(pix.rect().x()+offset,pix.rect().y()+offset,pix.rect().width(),pix.rect().height(),Qt::AlignCenter,QString::number(FPS));
    p.drawText(pix.rect().x()-offset,pix.rect().y()+offset,pix.rect().width(),pix.rect().height(),Qt::AlignCenter,QString::number(FPS));
    p.drawText(pix.rect().x()-offset,pix.rect().y()-offset,pix.rect().width(),pix.rect().height(),Qt::AlignCenter,QString::number(FPS));
    p.drawText(pix.rect().x()+offset,pix.rect().y()-offset,pix.rect().width(),pix.rect().height(),Qt::AlignCenter,QString::number(FPS));

    p.setPen(QPen(Qt::white));
    p.drawText(pix.rect(), Qt::AlignCenter,QString::number(FPS));
    imageText->setPixmap(QPixmap::fromImage(pix));

    frameCounter=0;
    timeUpdateFPS.restart();
    timerUpdateFPS.start();
}

void ScreenTransition::setTargetFPS(int targetFPS)
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

