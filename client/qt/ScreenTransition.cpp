#include "ScreenTransition.h"
#include "MainScreen.h"
#include "GameLoader.h"
#include "../../general/base/Version.h"

ScreenTransition::ScreenTransition() :
    QWidget()
{
    m_background=nullptr;
    m_foreground=nullptr;
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
    if(m_background!=nullptr)
    {
        m_background->setMinimumSize(size());
        m_background->setMaximumSize(size());
    }
    if(m_foreground!=nullptr)
    {
        m_foreground->setMinimumSize(size());
        m_foreground->setMaximumSize(size());
    }
}

void ScreenTransition::setBackground(QWidget *widget)
{
    m_background=widget;
    if(widget!=nullptr)
    {
        widget->setMinimumSize(size());
        widget->setMaximumSize(size());
        widget->setParent(this);
    }
}

void ScreenTransition::setForeground(QWidget *widget)
{
    if(m_foreground!=nullptr)
    {
        m_foreground->setVisible(false);
        m_foreground->setParent(nullptr);
    }
    m_foreground=widget;
    if(widget!=nullptr)
    {
        widget->move(0,0);
        widget->setMinimumSize(size());
        widget->setMaximumSize(size());
        widget->setParent(this);
    }
}

void ScreenTransition::toMainScreen()
{
    MainScreen *m=new MainScreen(this);
    setForeground(m);
    setWindowTitle(tr("CatchChallenger %1").arg(CATCHCHALLENGER_VERSION));
}
