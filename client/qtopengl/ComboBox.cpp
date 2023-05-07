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

void ComboBox::setVisible(bool visible)
{
    m_ComboBox->setVisible(visible);
}

int ComboBox::currentIndex() const
{
    return m_ComboBox->currentIndex();
}

void ComboBox::setItemData(int index, const QVariant &value, int role)
{
    m_ComboBox->setItemData(index,value,role);
}

QVariant ComboBox::itemData(int index, int role) const
{
    return m_ComboBox->itemData(index,role);
}

void ComboBox::setItemText(int index, const QString &text)
{
    m_ComboBox->setItemText(index,text);
}

void ComboBox::clear()
{
    m_ComboBox->clear();
}

int	ComboBox::count() const
{
    return m_ComboBox->count();
}

void ComboBox::setFixedWidth(int w)
{
    m_ComboBox->setFixedWidth(w);
}
