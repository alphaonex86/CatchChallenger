#include "LineEdit.hpp"

LineEdit::LineEdit(QGraphicsItem *parent) :
    QGraphicsProxyWidget(parent,Qt::Widget)
{
    m_lineEdit=new QLineEdit();
    setWidget(m_lineEdit);
    if(!connect(m_lineEdit,&QLineEdit::textChanged,this,&LineEdit::textChanged))
        abort();
    if(!connect(m_lineEdit,&QLineEdit::returnPressed,this,&LineEdit::returnPressed))
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

void LineEdit::setEchoMode(QLineEdit::EchoMode e)
{
    m_lineEdit->setEchoMode(e);
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

void LineEdit::setMaxLength(int a)
{
    m_lineEdit->setMaxLength(a);
}

void LineEdit::setVisible(bool visible)
{
    m_lineEdit->setVisible(visible);
}

void LineEdit::clear()
{
    m_lineEdit->clear();
}

void LineEdit::keyPressEvent(QKeyEvent * event)
{
    QGraphicsProxyWidget::keyPressEvent(event);
}

void LineEdit::keyReleaseEvent(QKeyEvent *event)
{
    QGraphicsProxyWidget::keyReleaseEvent(event);
}

bool LineEdit::hasFocus() const
{
    return m_lineEdit->hasFocus();
}

void LineEdit::clearFocus()
{
    m_lineEdit->clearFocus();
}

