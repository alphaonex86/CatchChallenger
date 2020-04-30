#include "LineEdit.hpp"

LineEdit::LineEdit(QGraphicsItem *parent) :
    QGraphicsProxyWidget(parent,Qt::Widget)
{
    m_lineEdit=new QLineEdit();
    setWidget(m_lineEdit);
    if(!connect(m_lineEdit,&QLineEdit::textChanged,this,&LineEdit::textChanged))
        abort();
}

LineEdit::~LineEdit()
{
}

void LineEdit::setText(const QString &text)
{
    m_lineEdit->setText(text);
}

void LineEdit::setStyleSheet(const QString& styleSheet)
{
    m_lineEdit->setStyleSheet(styleSheet);
}

void LineEdit::setMinimumSize(int minw, int minh)
{
    m_lineEdit->setMinimumSize(minw,minh);
}

void LineEdit::setMaximumSize(int maxw, int maxh)
{
    m_lineEdit->setMaximumSize(maxw,maxh);
}

void LineEdit::setFixedWidth(int w)
{
    m_lineEdit->setFixedWidth(w);
}

void LineEdit::setFixedHeight(int h)
{
    m_lineEdit->setFixedWidth(h);
}

void LineEdit::setFixedSize(int w, int h)
{
    m_lineEdit->setFixedSize(w,h);
}

void LineEdit::setFixedSize(const QSize &s)
{
    m_lineEdit->setFixedSize(s);
}

void LineEdit::setPlaceholderText(const QString &t)
{
    m_lineEdit->setPlaceholderText(t);
}

int LineEdit::width() const
{
    return m_lineEdit->width();
}

int LineEdit::height() const
{
    return m_lineEdit->height();
}

QString LineEdit::text() const
{
    return m_lineEdit->text();
}
