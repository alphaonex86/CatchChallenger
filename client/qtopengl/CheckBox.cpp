#include "CheckBox.hpp"

CheckBox::CheckBox(QGraphicsItem *parent) :
    QGraphicsProxyWidget(parent,Qt::Widget)
{
    m_checkBox=new QCheckBox();
    setWidget(m_checkBox);
}

CheckBox::~CheckBox()
{
}

bool CheckBox::isChecked() const
{
    return m_checkBox->isChecked();
}

void CheckBox::setChecked(bool c)
{
    m_checkBox->setChecked(c);
}
