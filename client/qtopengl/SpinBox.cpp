#include "SpinBox.hpp"

SpinBox::SpinBox(QGraphicsItem *parent) :
    QGraphicsProxyWidget(parent,Qt::Widget)
{
    m_spinBox=new QSpinBox();
    setWidget(m_spinBox);
}

SpinBox::~SpinBox()
{
}

void SpinBox::setFixedWidth(int w)
{
    m_spinBox->setFixedWidth(w);
}

void SpinBox::setFixedHeight(int h)
{
    m_spinBox->setFixedWidth(h);
}

void SpinBox::setFixedSize(int w, int h)
{
    m_spinBox->setFixedSize(w,h);
}

void SpinBox::setFixedSize(const QSize &s)
{
    m_spinBox->setFixedSize(s);
}

void SpinBox::setValue(int val)
{
    m_spinBox->setValue(val);
}

int SpinBox::value() const
{
    return m_spinBox->value();
}

int SpinBox::minimum() const
{
    return m_spinBox->minimum();
}

void SpinBox::setMinimum(int min)
{
    m_spinBox->setMinimum(min);
}

int SpinBox::maximum() const
{
    return m_spinBox->maximum();
}

void SpinBox::setMaximum(int max)
{
    m_spinBox->setMaximum(max);
}
