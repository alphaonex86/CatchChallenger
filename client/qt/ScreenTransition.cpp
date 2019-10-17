#include "ScreenTransition.h"
#include "GameLoader.h"
#include "../../general/base/Version.h"
#ifdef Q_OS_ANDROID
#include <QtAndroidExtras>
#endif

#ifdef Q_OS_ANDROID
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
    QWidget()
{
    m_backgroundStack=new QStackedWidget(this);
    m_foregroundStack=new QStackedWidget(this);
    m_aboveStack=new QStackedWidget(this);
    m_aboveStack->addWidget(new QWidget());
    m_aboveStack->setVisible(false);
    setBackground(&b);
    setForeground(&l);
    if(!connect(&l,&LoadingScreen::finished,this,&ScreenTransition::toMainScreen))
        abort();
    m=nullptr;
    o=nullptr;
    solo=nullptr;
    multi=nullptr;
    login=nullptr;

    #ifdef Q_OS_ANDROID
    keep_screen_on(true);
    #endif
}

ScreenTransition::~ScreenTransition()
{
}

void ScreenTransition::paintEvent(QPaintEvent *)
{
}

void ScreenTransition::resizeEvent(QResizeEvent *)
{
    if(m_backgroundStack!=nullptr)
    {
        m_backgroundStack->setMinimumSize(size());
        m_backgroundStack->setMaximumSize(size());
    }
    if(m_foregroundStack!=nullptr)
    {
        m_foregroundStack->setMinimumSize(size());
        m_foregroundStack->setMaximumSize(size());
    }
    if(m_aboveStack!=nullptr)
    {
        m_aboveStack->setMinimumSize(size());
        m_aboveStack->setMaximumSize(size());
    }
}

void ScreenTransition::setBackground(QWidget *widget)
{
    if(widget!=nullptr)
    {
        if(m_backgroundStack->indexOf(widget)<0)
            m_backgroundStack->addWidget(widget);
        m_backgroundStack->setCurrentWidget(widget);
    }
}

void ScreenTransition::setForeground(QWidget *widget)
{
    if(widget!=nullptr)
    {
        if(m_foregroundStack->indexOf(widget)<0)
            m_foregroundStack->addWidget(widget);
        m_foregroundStack->setCurrentWidget(widget);
    }
}

void ScreenTransition::setAbove(QWidget *widget)
{
    if(widget!=nullptr)
    {
        if(m_aboveStack->indexOf(widget)<0)
            m_aboveStack->addWidget(widget);
        m_aboveStack->setCurrentWidget(widget);
        m_aboveStack->setVisible(true);
    }
    else
    {
        m_aboveStack->setVisible(false);
        m_aboveStack->setCurrentIndex(0);
    }
}

void ScreenTransition::toMainScreen()
{
    if(m==nullptr)
    {
        m=new MainScreen(this);
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
        o=new OptionsDialog(this);
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
    if(solo==nullptr)
    {
        solo=new Solo(this);
        if(!connect(solo,&Solo::backMain,this,&ScreenTransition::backMain))
            abort();
    }
    setForeground(solo);
}

void ScreenTransition::openMulti()
{
    if(multi==nullptr)
    {
        multi=new Multi(this);
        if(!connect(multi,&Multi::backMain,this,&ScreenTransition::backMain))
            abort();
        if(!connect(multi,&Multi::setAbove,this,&ScreenTransition::setAbove))
            abort();
        if(!connect(multi,&Multi::connectToServer,this,&ScreenTransition::connectToServer))
            abort();
    }
    setForeground(multi);
}

void ScreenTransition::connectToServer(Multi::ConnexionInfo connexionInfo,QString login,QString pass)
{
    Q_UNUSED(connexionInfo);
    Q_UNUSED(login);
    Q_UNUSED(pass);
    setForeground(&l);
    baseWindow=new CatchChallenger::BaseWindow();
    connexionManager=new ConnexionManager(baseWindow,&l);
    connexionManager->connectToServer(connexionInfo,login,pass);
    if(!connect(connexionManager,&ConnexionManager::logged,this,&ScreenTransition::logged))
        abort();
    if(!connect(connexionManager,&ConnexionManager::errorString,this,&ScreenTransition::errorString))
        abort();
    if(!connect(connexionManager,&ConnexionManager::disconnectedFromServer,this,&ScreenTransition::disconnectedFromServer))
        abort();
    l.progression(0,100);
}

/*void ScreenTransition::errorString(std::string error)
{
    setForeground(m);
    m->setError(error);
}*/

void ScreenTransition::backMain()
{
    setForeground(m);
}

void ScreenTransition::logged(const std::vector<std::vector<CatchChallenger::CharacterEntry> > &characterEntryList)
{
    Q_UNUSED(characterEntryList);
    setBackground(nullptr);
    setForeground(baseWindow);
    //baseWindow->updateConnectingStatus();
}

void ScreenTransition::disconnectedFromServer()
{
    setBackground(&b);
    setForeground(m);
    setAbove(nullptr);
}

void ScreenTransition::errorString(std::string error)
{
    setBackground(&b);
    setForeground(m);
    setAbove(nullptr);
    m->setError(error);
}

