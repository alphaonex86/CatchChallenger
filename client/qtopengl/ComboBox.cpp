#include "ComboBox.hpp"

ComboBox::ComboBox(QGraphicsItem *parent) :
    QGraphicsProxyWidget(parent,Qt::Widget)
{
    m_ComboBox=new QComboBox();
    setWidget(m_ComboBox);
    if(!connect(m_ComboBox,static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),this,static_cast<void(ComboBox::*)(int)>(&ComboBox::currentIndexChanged)))
        abort();
}

ComboBox::~ComboBox()
{
}

void ComboBox::addItem(const QString &text, const QVariant &userData)
{
    m_ComboBox->addItem(text,userData);
}

void ComboBox::setCurrentIndex(int index)
{
    m_ComboBox->setCurrentIndex(index);
}

int ComboBox::currentIndex() const
{
    return m_ComboBox->currentIndex();
}
