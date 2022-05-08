#include "CustomButton.h"
#include "CCBackground.h"
#include <QPainter>
#include <QEvent>
#include <QMouseEvent>
#include <iostream>
#include <QApplication>

CustomButton::CustomButton(QString pix, QGraphicsItem *parent) :
    QGraphicsItem(parent)
{
    font=new QFont();
    font->setFamily("Comic Sans MS");
    font->setPointSize(35);
    font->setStyleHint(QFont::Monospace);
    font->setBold(true);
    font->setStyleStrategy(QFont::ForceOutline);
    //setMaximumSize(QSize(223,92));

    pressed=false;
    over=false;
    background=pix;
    m_boundingRect=QRectF(0.0,0.0,223.0,92.0);
}

CustomButton::~CustomButton()
{
    if(font!=nullptr)
    {
        delete font;
        font=nullptr;
    }
}

QRectF CustomButton::boundingRect() const
{
    return m_boundingRect;
}

void CustomButton::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    return;
    Q_UNUSED(option);
    auto end = std::chrono::steady_clock::now();
    if(pressed) {
            if(scaledBackgroundPressed.width()!=m_boundingRect.width() || scaledBackgroundPressed.height()!=m_boundingRect.height())
            {
                QPixmap temp(background);
                scaledBackgroundPressed=temp.copy(0,temp.height()/3,temp.width(),temp.height()/3);
                scaledBackgroundPressed=scaledBackgroundPressed.scaled(m_boundingRect.width(),m_boundingRect.height(),Qt::IgnoreAspectRatio,Qt::SmoothTransformation);
            }
            painter->drawPixmap(m_boundingRect.x(),m_boundingRect.y(),m_boundingRect.width(),m_boundingRect.height(),scaledBackgroundPressed);
    } else if(over) {
        if(scaledBackgroundOver.width()!=m_boundingRect.width() || scaledBackgroundOver.height()!=m_boundingRect.height())
        {
            QPixmap temp(background);
            scaledBackgroundOver=temp.copy(0,temp.height()/3*2,temp.width(),temp.height()/3);
            scaledBackgroundOver=scaledBackgroundOver.scaled(m_boundingRect.width(),m_boundingRect.height(),Qt::IgnoreAspectRatio,Qt::SmoothTransformation);
        }
        painter->drawPixmap(m_boundingRect.x(),m_boundingRect.y(),m_boundingRect.width(),m_boundingRect.height(),scaledBackgroundOver);
    } else {
        if(scaledBackground.width()!=m_boundingRect.width() || scaledBackground.height()!=m_boundingRect.height())
        {
            QPixmap temp(background);
            scaledBackground=temp.copy(0,0,temp.width(),temp.height()/3);
            scaledBackground=scaledBackground.scaled(m_boundingRect.width(),m_boundingRect.height(),Qt::IgnoreAspectRatio,Qt::SmoothTransformation);
        }
        painter->drawPixmap(m_boundingRect.x(),m_boundingRect.y(),m_boundingRect.width(),m_boundingRect.height(),scaledBackground);
    }

    QPainterPath tempPath;
    tempPath.addText(0, 0, *font, text);
    QRectF rect=tempPath.boundingRect();
    QPainterPath textPath;
    const int h=m_boundingRect.height();
    const int newHeight=(h*80/100);
    const int p=font->pointSize();
    const int tempHeight=newHeight/2+p/2;
    textPath.addText(m_boundingRect.x()+m_boundingRect.width()/2-rect.width()/2, tempHeight+m_boundingRect.y(), *font, text);

    painter->setRenderHint(QPainter::Antialiasing);
    painter->setPen(QPen(QColor(217,145,0)/*penColor*/, 2/*penWidth*/, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter->setBrush(Qt::white);
    painter->drawPath(textPath);
}

void CustomButton::setText(const QString &text)
{
    this->text=text;
}

void CustomButton::setFont(const QFont &font)
{
    *this->font=font;
}

void CustomButton::emitclicked()
{
    emit clicked();
}

void CustomButton::setPressed(const bool &pressed)
{
    this->pressed=pressed;
}

void CustomButton::setPos(qreal ax, qreal ay)
{
    m_boundingRect.setX(ax);
    m_boundingRect.setY(ay);
}

/*void CustomButton::enterEvent(QEvent *e)
{
    over=true;
    QPushButton::enterEvent(e);
}
void CustomButton::leaveEvent(QEvent *e)
{
    over=false;
    pressed=false;
    QPushButton::leaveEvent(e);
}*/

/*void CustomButton::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    std::cerr << "Item Mouse Pressed" << std::endl;
    pressed=true;
    QGraphicsItem::mousePressEvent(event);
}

void CustomButton::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    pressed=false;
    QGraphicsItem::mouseReleaseEvent(event);
 //   emit clicked();
}*/

/*QRectF CustomButton::outlineRect() const
{
    const int iPadding = 8;
    QFontMetricsF metrics = QFontMetricsF(QApplication::font());
    QRectF rect = metrics.boundingRect("dfgdfg");
    rect.adjust(-iPadding, -iPadding, +iPadding, +iPadding);
    rect.translate(-rect.center());
    return rect;
}

QRectF CustomButton::boundingRect() const
{
    const int iMargin = 1;
    return outlineRect().adjusted(-iMargin, -iMargin, +iMargin, +iMargin);

}

QPainterPath CustomButton::shape() const
{
    QRectF rect = outlineRect();

    QPainterPath path;
    path.addRect(rect);

    return path;
}*/
