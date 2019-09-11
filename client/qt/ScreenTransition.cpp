#include "ScreenTransition.h"
#include "GameLoader.h"
#include "../../general/base/Version.h"

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
    connect(&l,&LoadingScreen::finished,this,&ScreenTransition::toMainScreen);
    m=nullptr;
    o=nullptr;
    solo=nullptr;
    multi=nullptr;
    login=nullptr;
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
        connect(m,&MainScreen::goToOptions,this,&ScreenTransition::openOptions);
        connect(m,&MainScreen::goToSolo,this,&ScreenTransition::openSolo);
        connect(m,&MainScreen::goToMulti,this,&ScreenTransition::openMulti);
    }
    setForeground(m);
    setWindowTitle(tr("CatchChallenger %1").arg(QString::fromStdString(CatchChallenger::Version::str)));
}

void ScreenTransition::openOptions()
{
    if(o==nullptr)
    {
        o=new OptionsDialog(this);
        connect(o,&OptionsDialog::quitOption,this,&ScreenTransition::closeOptions);
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
        connect(solo,&Solo::backMain,this,&ScreenTransition::backMain);
    }
    setForeground(solo);
}

void ScreenTransition::openMulti()
{
    if(multi==nullptr)
    {
        multi=new Multi(this);
        connect(multi,&Multi::backMain,this,&ScreenTransition::backMain);
        connect(multi,&Multi::setAbove,this,&ScreenTransition::setAbove);
    }
    setForeground(multi);
}

void ScreenTransition::backMain()
{
    setForeground(m);
}

