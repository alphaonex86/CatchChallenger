#include "MultiItem.hpp"
#include <QPainter>
#include <QObject>
#include <QDateTime>
#include "../qt/GameLoader.hpp"

MultiItem::MultiItem(const ConnexionInfo &connexionInfo, QGraphicsItem *parent) :
    QGraphicsItem(parent),
    m_connexionInfo(connexionInfo)
{
    cache=nullptr;
    lastwidth=0;
    lastheight=0;
    m_selected=false;
    pressed=false;
}

MultiItem::~MultiItem()
{
    if(cache!=nullptr)
        delete cache;
    cache=nullptr;
}

void MultiItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    if(cache!=nullptr && !cache->isNull() && cache->width()==m_boundingRect.width() && cache->height()==m_boundingRect.height())
    {
        painter->drawPixmap(m_boundingRect.x(),m_boundingRect.y(),m_boundingRect.width(),m_boundingRect.height(),*cache);
        return;
    }
    if(m_boundingRect.isEmpty())
        return;
    if(cache!=nullptr)
        delete cache;
    cache=new QPixmap();

    QImage image(m_boundingRect.width(),m_boundingRect.height(),QImage::Format_ARGB32);
    //image.fill(Qt::transparent);
    if(isSelected())
        image.fill(QColor(100,150,135,150));
    else
        image.fill(QColor(100,150,100,50));
    QPainter paint;
    if(image.isNull())
        abort();
    paint.begin(&image);
    if(lastwidth!=m_boundingRect.width() || lastheight!=m_boundingRect.height())
    {
        lastwidth=m_boundingRect.width();
        lastheight=m_boundingRect.height();
    }
    if(!paint.isActive())
        return;

    QFont font;
    font.setFamily("Comic Sans MS");
    font.setPixelSize(25);
    font.setStyleHint(QFont::Monospace);
    font.setBold(true);
    font.setStyleStrategy(QFont::ForceOutline);

    QString connexionInfoLabel;
    if(!m_connexionInfo.name.isEmpty())
        connexionInfoLabel=m_connexionInfo.name;
    else
    {
        #ifdef NOTCPSOCKET
        connexionInfoLabel=m_connexionInfo.ws;
        #else
            #ifdef NOWEBSOCKET
            connexionInfoLabel=m_connexionInfo.host;
            #else
            if(!m_connexionInfo.host.isEmpty())
                connexionInfoLabel=m_connexionInfo.host;
            else
                connexionInfoLabel=m_connexionInfo.ws;
            #endif
        #endif
        if(connexionInfoLabel.size()>32)
            connexionInfoLabel=connexionInfoLabel.left(15)+"..."+connexionInfoLabel.right(15);
    }
    if(m_connexionInfo.connexionCounter>0)
    {
        const QPixmap &p=*GameLoader::gameLoader->getImage(":/CC/images/interface/top.png");
        paint.drawPixmap(5,image.width()/2-p.width()/2,p.width(),p.height(),p);
    }
    const QString &text=connexionInfoLabel;
    QPainterPath tempPath;
    tempPath.addText(0, 0, font, text);
    QRectF rect=tempPath.boundingRect();
    QPainterPath textPath;
    int newHeight=m_boundingRect.height();
    const int p=font.pointSize();
    const int tempHeight=newHeight/2+p/2;
    const Qt::Alignment a=Qt::AlignCenter;//alignment();
    if(a.testFlag(Qt::AlignLeft))
        textPath.addText(0, tempHeight-1, font, text);
    else if(a.testFlag(Qt::AlignRight))
        textPath.addText(m_boundingRect.width()-rect.width(), tempHeight-1, font, text);
    else
        textPath.addText(m_boundingRect.width()/2-rect.width()/2, tempHeight-1, font, text);


    {
        paint.setRenderHint(QPainter::Antialiasing);
        /*qreal penWidth=2.0;
        if(font.pointSize()<=12)
            penWidth=0.7;
        else if(font.pointSize()<=18)
            penWidth=1;
        else if(font.pointSize()<=24)
            penWidth=1.5;
        paint.setPen(QPen(outlineColor, penWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));*/
        paint.setBrush(Qt::white);
        paint.drawPath(textPath);
    }

    *cache=QPixmap::fromImage(image);

    painter->drawPixmap(m_boundingRect.x(),m_boundingRect.y(),m_boundingRect.width(),m_boundingRect.height(),*cache);
}

QRectF MultiItem::boundingRect() const
{
    return m_boundingRect;
}

void MultiItem::setWidth(const int &w)
{
    this->m_boundingRect.setWidth(w);
}

void MultiItem::setHeight(const int &h)
{
    this->m_boundingRect.setHeight(h);
}

void MultiItem::setSize(const int &w,const int &h)
{
    this->m_boundingRect.setSize(QSizeF(w,h));
}

int MultiItem::width() const
{
    return this->m_boundingRect.width();
}

int MultiItem::height() const
{
    return this->m_boundingRect.height();
}

void MultiItem::setSelected(bool selected)
{
    m_selected=selected;
    if(cache!=nullptr)
        delete cache;
    cache=nullptr;
}

bool MultiItem::isSelected() const
{
    return m_selected;
}

void MultiItem::mousePressEventXY(const QPointF &p,bool &pressValidated)
{
    if(this->pressed)
        return;
    if(pressValidated)
        return;
    if(!isVisible())
        return;
    if(!isEnabled())
        return;
    const QRectF &b=boundingRect();
    const QRectF &t=mapRectToScene(b);
    if(t.contains(p))
    {
        pressValidated=true;
        this->pressed=true;
    }
}

void MultiItem::mouseReleaseEventXY(const QPointF &p, bool &previousPressValidated)
{
    if(previousPressValidated)
    {
        this->pressed=false;
        return;
    }
    const QRectF &b=boundingRect();
    const QRectF &t=mapRectToScene(b);
    if(!this->pressed)
        return;
    if(!isEnabled())
        return;
    if(!previousPressValidated && isVisible())
    {
        if(t.contains(p))
        {
            setSelected(true);
            previousPressValidated=true;
        }
    }
    this->pressed=false;
}

const ConnexionInfo &MultiItem::connexionInfo() const
{
    return m_connexionInfo;
}
