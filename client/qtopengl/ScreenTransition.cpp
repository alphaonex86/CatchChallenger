#include "ScreenTransition.hpp"
#include "../qt/GameLoader.hpp"
#include "cc/QtDatapackClientLoader.hpp"
#include "background/CCMap.hpp"
#include "foreground/CharacterList.hpp"
#include "foreground/LoadingScreen.hpp"
#include "foreground/MainScreen.hpp"
#include "foreground/Multi.hpp"
#include "foreground/SubServer.hpp"
#include "foreground/OverMapLogic.hpp"
#include "above/OptionsDialog.hpp"
#include "above/DebugDialog.hpp"
#include "ConnexionManager.hpp"
#include "../../general/base/Version.hpp"
#ifndef CATCHCHALLENGER_NOAUDIO
#include "AudioGL.hpp"
#endif
#include <iostream>
#include <QGLWidget>
#include <QComboBox>
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
    {
        QGLWidget *context=new QGLWidget(QGLFormat(QGL::SampleBuffers));
        //if OpenGL is present, use it
        if(context->isValid())
        {
            setViewport(context);
            setRenderHint(QPainter::Antialiasing,true);
            setRenderHint(QPainter::TextAntialiasing,true);
            setRenderHint(QPainter::HighQualityAntialiasing,true);
            setRenderHint(QPainter::SmoothPixmapTransform,false);
            setRenderHint(QPainter::NonCosmeticDefaultPen,true);
        }
        //else use the CPU only
    }

    mousePress=nullptr;
    m_backgroundStack=nullptr;
    m_foregroundStack=nullptr;
    m_aboveStack=nullptr;
    subserver=nullptr;
    characterList=nullptr;
    connexionManager=nullptr;
    ccmap=nullptr;
    overmap=nullptr;
    m=nullptr;
    o=nullptr;
    d=nullptr;
    /*solo=nullptr;*/
    multi=nullptr;
    login=nullptr;
    setBackground(&b);
    setForeground(&l);
    if(!connect(&l,&LoadingScreen::finished,this,&ScreenTransition::loadingFinished))
        abort();

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
    imageText->setZValue(999);

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
    bool callParentClass=false;
    const QPointF &p=mapToScene(event->pos());
    if(mousePress!=nullptr)
    {
        bool temp=true;//don¡t do action if true
        static_cast<ScreenInput *>(mousePress)->mouseReleaseEventXY(p,temp,callParentClass);
        mousePress=nullptr;
    }
    bool temp=false;
    if(m_aboveStack!=nullptr)
    {
        static_cast<ScreenInput *>(m_aboveStack)->mousePressEventXY(p,temp,callParentClass);
        mousePress=m_aboveStack;
    }
    if(!temp && m_foregroundStack!=nullptr)
    {
        static_cast<ScreenInput *>(m_foregroundStack)->mousePressEventXY(p,temp,callParentClass);
        mousePress=m_foregroundStack;
    }
    if(!temp && m_backgroundStack!=nullptr)
    {
        static_cast<ScreenInput *>(m_backgroundStack)->mousePressEventXY(p,temp,callParentClass);
        mousePress=m_backgroundStack;
    }
    if(!temp || callParentClass)
        QGraphicsView::mousePressEvent(event);
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
    bool callParentClass=false;
    const QPointF &p=mapToScene(event->pos());
    mousePress=nullptr;
    bool pressValidated=false;
    if(m_aboveStack!=nullptr)
    {
        m_aboveStack->mouseReleaseEventXY(p,pressValidated,callParentClass);
        mousePress=m_aboveStack;
    }
    if(!pressValidated && m_foregroundStack!=nullptr)
    {
        m_foregroundStack->mouseReleaseEventXY(p,pressValidated,callParentClass);
        mousePress=m_foregroundStack;
    }
    if(!pressValidated && m_backgroundStack!=nullptr)
    {
        m_backgroundStack->mouseReleaseEventXY(p,pressValidated,callParentClass);
        mousePress=m_backgroundStack;
    }
    if(!pressValidated || callParentClass)
        QGraphicsView::mouseReleaseEvent(event);
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
    bool callParentClass=false;
    const QPointF &p=mapToScene(event->pos());
    mousePress=nullptr;
    bool pressValidated=false;
    if(m_aboveStack!=nullptr)
    {
        m_aboveStack->mouseMoveEventXY(p,pressValidated,callParentClass);
        mousePress=m_aboveStack;
    }
    else if(m_foregroundStack!=nullptr)
    {
        m_foregroundStack->mouseMoveEventXY(p,pressValidated,callParentClass);
        mousePress=m_foregroundStack;
    }
    if(!pressValidated)
        QGraphicsView::mouseMoveEvent(event);
}

void ScreenTransition::setBackground(ScreenInput *widget)
{
    if(m_backgroundStack!=nullptr)
        mScene->removeItem(m_backgroundStack);
    m_backgroundStack=widget;
    if(widget!=nullptr)
    {
        mScene->addItem(m_backgroundStack);
        m_backgroundStack->setZValue(-1);
    }
}

void ScreenTransition::setForeground(ScreenInput *widget)
{
    if(mousePress!=nullptr)
    {
        bool temp=true;//don¡t do action if true
        static_cast<ScreenInput *>(mousePress)->mouseReleaseEventXY(QPointF(0.0,0.0),temp,temp);
        mousePress=nullptr;
    }
    if(ccmap!=nullptr)
        ccmap->keyPressReset();
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
        static_cast<ScreenInput *>(mousePress)->mouseReleaseEventXY(QPointF(0.0,0.0),temp,temp);
        mousePress=nullptr;
    }
    if(ccmap!=nullptr)
        ccmap->keyPressReset();
    if(m_aboveStack!=nullptr)
        mScene->removeItem(m_aboveStack);
    m_aboveStack=widget;
    if(widget!=nullptr)
        mScene->addItem(m_aboveStack);
}

void ScreenTransition::loadingFinished()
{
    if(connexionManager==nullptr)
        toMainScreen();
    else if(connexionManager->client==nullptr)
        toMainScreen();
    else if(!connexionManager->client->getIsLogged())
        toMainScreen();
    else if(!connexionManager->client->getCaracterSelected())
        toSubServer();
    else
        toInGame();
}

void ScreenTransition::toSubServer()
{
    if(subserver==nullptr)
    {
        subserver=new SubServer();
        if(!connect(subserver,&SubServer::connectToSubServer,this,&ScreenTransition::connectToSubServer))
            abort();
        if(!connect(subserver,&SubServer::backMulti,this,&ScreenTransition::toMainScreen))
            abort();
    }
    setForeground(subserver);
}

void ScreenTransition::toInGame()
{
    if(characterList==nullptr)
    {
        characterList=new CharacterList();
        if(!connect(characterList,&CharacterList::backSubServer,this,&ScreenTransition::toSubServer))
            abort();
        if(!connect(characterList,&CharacterList::setAbove,this,&ScreenTransition::setAbove))
            abort();
        if(!connect(characterList,&CharacterList::selectCharacter,this,&ScreenTransition::selectCharacter))
            abort();
    }
    setForeground(characterList);
}

void ScreenTransition::toMainScreen()
{
    QtDatapackClientLoader::datapackLoader=new QtDatapackClientLoader();
    #ifndef CATCHCHALLENGER_NOAUDIO
    if(Audio::audio==nullptr)
    {
        AudioGL* a=new AudioGL();
        Audio::audio=a;
        if(!connect(QCoreApplication::instance(),&QCoreApplication::aboutToQuit,a,&AudioGL::stopCurrentAmbiance,Qt::DirectConnection))
            abort();
    }
    #endif
    if(m==nullptr)
    {
        m=new MainScreen();
        if(!connect(m,&MainScreen::goToOptions,this,&ScreenTransition::openOptions))
            abort();
        if(!connect(m,&MainScreen::goToDebug,this,&ScreenTransition::openDebug))
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
        if(!connect(o,&OptionsDialog::removeAbove,this,&ScreenTransition::removeAbove))
            abort();
    }
    setAbove(o);
}

void ScreenTransition::openDebug()
{
    if(d==nullptr)
    {
        d=new DebugDialog();
        if(!connect(d,&DebugDialog::removeAbove,this,&ScreenTransition::removeAbove))
            abort();
    }
    setAbove(d);
}

void ScreenTransition::removeAbove()
{
    setAbove(nullptr);
}

void ScreenTransition::openSolo()
{
    if(m!=nullptr)
        m->setError(std::string());
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
    if(m!=nullptr)
        m->setError(std::string());
    if(multi==nullptr)
    {
        multi=new Multi();
        if(!connect(multi,&Multi::backMain,this,&ScreenTransition::backMain))
            abort();
        if(!connect(multi,&Multi::connectToServer,this,&ScreenTransition::connectToServer))
            abort();
        if(!connect(multi,&Multi::setAbove,this,&ScreenTransition::setAbove))
            abort();
    }
    setForeground(multi);
}

void ScreenTransition::connectToServer(ConnexionInfo connexionInfo,QString login,QString pass)
{
    Q_UNUSED(connexionInfo);
    Q_UNUSED(login);
    Q_UNUSED(pass);
    setForeground(&l);
    //baseWindow=new CatchChallenger::BaseWindow();
    if(connexionManager!=nullptr)
        delete connexionManager;
    connexionManager=new ConnexionManager(&l);
    connexionManager->connectToServer(connexionInfo,login,pass);
    if(!connect(connexionManager,&ConnexionManager::logged,this,&ScreenTransition::logged))
        abort();
    if(!connect(connexionManager,&ConnexionManager::errorString,this,&ScreenTransition::errorString))
        abort();
    if(!connect(connexionManager,&ConnexionManager::disconnectedFromServer,this,&ScreenTransition::disconnectedFromServer))
        abort();
    if(!connect(connexionManager,&ConnexionManager::goToMap,this,&ScreenTransition::goToMap))
        abort();
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

void ScreenTransition::backSubServer()
{
    setForeground(subserver);
}

void ScreenTransition::logged(const std::vector<std::vector<CatchChallenger::CharacterEntry> > &characterEntryList)
{
    this->characterEntryList=characterEntryList;
    if(subserver==nullptr)
    {
        subserver=new SubServer();
        if(!connect(subserver,&SubServer::backMulti,this,&ScreenTransition::backMain))
            abort();
        if(!connect(subserver,&SubServer::connectToSubServer,this,&ScreenTransition::connectToSubServer))
            abort();
    }
    subserver->logged(connexionManager->client->getServerOrdenedList(),connexionManager);
    setForeground(subserver);
}

void ScreenTransition::connectToSubServer(const int indexSubServer)
{
    if(characterList==nullptr)
    {
        characterList=new CharacterList();
        if(!connect(characterList,&CharacterList::backSubServer,this,&ScreenTransition::backSubServer))
            abort();
        if(!connect(characterList,&CharacterList::selectCharacter,this,&ScreenTransition::selectCharacter))
            abort();
    }
    characterList->connectToSubServer(indexSubServer,connexionManager,characterEntryList);
    setForeground(characterList);

    //todo: optimise by prevent create each time
    if(ccmap!=nullptr)
    {
        delete ccmap;
        ccmap=nullptr;
    }
    if(ccmap==nullptr)
    {
        ccmap=new CCMap();
        /*if(!connect(ccmap,&CCMap::backSubServer,this,&ScreenTransition::backSubServer))
            abort();
        if(!connect(ccmap,&CCMap::selectCharacter,this,&ScreenTransition::selectCharacter))
            abort();*/
        if(!connect(ccmap,&CCMap::error,this,&ScreenTransition::errorString))
            abort();
    }
    ccmap->setVar(connexionManager);

    if(overmap!=nullptr)
    {
        delete overmap;
        overmap=nullptr;
    }
    if(overmap==nullptr)
    {
        overmap=new OverMapLogic();
        if(!connect(overmap,&OverMap::error,this,&ScreenTransition::errorString))
            abort();
    }
    overmap->resetAll();
    overmap->connectAllSignals();
    overmap->setVar(connexionManager);
}

void ScreenTransition::selectCharacter(const int indexSubServer,const int indexCharacter)
{
    l.reset();
    l.progression(0,100);
    l.setText("Selecting your character...");
    setAbove(nullptr);
    setForeground(&l);
    connexionManager->selectCharacter(indexSubServer,indexCharacter);
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
    std::cerr << "ScreenTransition::errorString(" << error << ")" << std::endl;
    setBackground(&b);
    setForeground(m);
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
    setBackground(ccmap);
    setForeground(overmap);
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

void ScreenTransition::keyPressEvent(QKeyEvent * event)
{
    event->setAccepted(false);
    if(m_aboveStack!=nullptr)
        static_cast<ScreenInput *>(m_aboveStack)->keyPressEvent(event);
    if(!event->isAccepted() && m_foregroundStack!=nullptr)
        static_cast<ScreenInput *>(m_foregroundStack)->keyPressEvent(event);
    if(!event->isAccepted() && m_backgroundStack!=nullptr)
        static_cast<ScreenInput *>(m_backgroundStack)->keyPressEvent(event);
    if(!event->isAccepted())
        QGraphicsView::keyPressEvent(event);
}

void ScreenTransition::keyReleaseEvent(QKeyEvent *event)
{
    event->setAccepted(false);
    if(m_aboveStack!=nullptr)
        static_cast<ScreenInput *>(m_aboveStack)->keyReleaseEvent(event);
    if(!event->isAccepted() && m_foregroundStack!=nullptr)
        static_cast<ScreenInput *>(m_foregroundStack)->keyReleaseEvent(event);
    if(!event->isAccepted() && m_backgroundStack!=nullptr)
        static_cast<ScreenInput *>(m_backgroundStack)->keyReleaseEvent(event);
    if(!event->isAccepted())
        QGraphicsView::keyReleaseEvent(event);
}
