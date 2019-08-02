#include "ScreenTransition.h"
#include "GameLoader.h"

ScreenTransition::ScreenTransition() :
    QWidget()
{
    m_background=nullptr;
    m_foreground=nullptr;
    setBackground(&b);
    setForeground(&l);
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
    m_foreground=widget;
    if(widget!=nullptr)
    {
        widget->setMinimumSize(size());
        widget->setMaximumSize(size());
        widget->setParent(this);
    }
}
