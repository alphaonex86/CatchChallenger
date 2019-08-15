#include "ScreenTransition.h"
#include "MainScreen.h"
#include "GameLoader.h"
#include "../../general/base/Version.h"

ScreenTransition::ScreenTransition() :
    QWidget()
{
    m_backgroundStack=new QStackedWidget(this);
    m_foregroundStack=new QStackedWidget(this);
    setBackground(&b);
    setForeground(&l);
    connect(&l,&LoadingScreen::finished,this,&ScreenTransition::toMainScreen);
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
}

void ScreenTransition::setBackground(QWidget *widget)
{
    if(widget!=nullptr)
    {
        m_backgroundStack->addWidget(widget);
        m_backgroundStack->setCurrentWidget(widget);
    }
}

void ScreenTransition::setForeground(QWidget *widget)
{
    if(widget!=nullptr)
    {
        m_foregroundStack->addWidget(widget);
        m_foregroundStack->setCurrentWidget(widget);
    }
}

void ScreenTransition::toMainScreen()
{
    MainScreen *m=new MainScreen(this);
    setForeground(m);
    setWindowTitle(tr("CatchChallenger %1").arg(CATCHCHALLENGER_VERSION));
}
