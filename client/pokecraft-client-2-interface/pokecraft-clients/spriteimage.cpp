#include "spriteimage.h"

#include <QPainter>
#include <QFile>
#include <QDebug>

#include "gamedata.h"

class spriteData
{
public:
    spriteData();

    QPixmap* getSpriteId(int id);
    QPixmap* getSprite();
    void setSprite(QPixmap* sprite);
    void setPath(QString path);
    QString getSpritePath();

    static void emptyAllSpriteData();
    static QList<spriteData*>& getData();
    static void add(spriteData* sprite);
private:
    QString mPath;
    QPixmap* mainSprite;
    QList<QPixmap*> mSprite;
    static QList<spriteData*> data;
};

spriteData::spriteData()
{
    mainSprite = 0;
}

QPixmap* spriteData::getSprite()
{
    return mainSprite;
}

QPixmap* spriteData::getSpriteId(int id)
{
    if(mSprite.length()>id)
        return mSprite.at(id);
    return 0;
}

void spriteData::setSprite(QPixmap *sprite)
{
    if(sprite == 0 || sprite->isNull())
        return;
    mainSprite = sprite;
    for(int i = 0; i<12; i++)
    {
        QRect area = QRect((i%3)*sprite->width()/3,(i/3)*sprite->height()/4,16,24);
        mSprite.append(new QPixmap(mainSprite->copy(area)));
    }
}

QString spriteData::getSpritePath()
{
    return mPath;
}

void spriteData::setPath(QString path)
{
    mPath = path;
}

QList<spriteData*> spriteData::data = QList<spriteData*>();

QList<spriteData*>& spriteData::getData()
{
    return data;
}

void spriteData::add(spriteData *sprite)
{
    data.append(sprite);
}


spriteImage::spriteImage(QGraphicsItem *parent) :
        QGraphicsItem(parent)//,QObject()
{
        mArea   = QRectF(0,0,0,0);
        mHeight = 24;
        mWidth  = 16;
        mOffset = QPointF(0,-8);
        tItem   = 0;
        spritePixmap  = 0;
        useSpriteData = false;
        setFlag(QGraphicsItem::ItemUsesExtendedStyleOption);
        //setName(name);
}

spriteImage::~spriteImage()
{
    if(tItem!=0 && !useSpriteData)
        delete tItem;
    tItem = 0;
    this->setParentItem(0);
}
// -------------- QGraphicsItem
void spriteImage::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
        if(tItem!=0)
            painter->drawPixmap(boundingRect().toRect(),*tItem);
}

QRectF spriteImage::boundingRect() const
{
        QRectF tArea = QRectF(mArea);
        tArea.moveTopLeft(mOffset);
        return tArea;
}

// -------------- setter
void spriteImage::setArea(QRectF area)
{
        mArea = area;
        moveBy(0,0);
        if(tItem!=0)
        {
            delete tItem;
            tItem = 0;
        }
        if(!pixmap().isNull())
        {
            tItem = new QPixmap(pixmap().copy(mArea.toRect()));
        }
        useSpriteData=false;
}

void spriteImage::setArea(QPointF point1, QPointF point2)
{
        setArea(QRectF( point1, point2));
}

void spriteImage::setSpriteHeight(quint8 height)
{
        if(height < 1)
                return;
        mHeight = height;
}

void spriteImage::setSpriteWidth(quint8 width)
{
        if(width < 1)
                return;
        mWidth = width;
}

bool spriteImage::setSprite(int x, int y)
{
        if(mHeight != 0 && mWidth != 0)
        {
                setArea(QRectF(QPoint(x*mWidth,y*mHeight),QSize(mWidth,mHeight)));
                return true;
        }
        qWarning() << "can't change sprite";
        return false;
}

bool spriteImage::setSprite(int id)
{
        if(mHeight != 0 && mWidth != 0 && spritePixmap!=0)
        {
            tItem = spritePixmap->getSpriteId(id);
            if(tItem == 0)
            {
                qWarning()<< "No sprite for id :"<<id;
                return false;
            }
            useSpriteData= true;
            mArea = tItem->rect();
            return true;
        }
        return false;
}

void spriteImage::setOffset(int x, int y)
{
        mOffset = QPointF(x,y);
}

void spriteImage::setOffset(QPointF point)
{
        mOffset = point;
}

/*void spriteImage::setPixmap( QPixmap* pixmap )
{
        if(pixmap != 0 && !pixmap->isNull())
                mItem = pixmap;
        else
                mItem = 0;
}
*/
void spriteImage::setPixmap(QString path)
{
        if(QFile::exists(path))
        {
                //qWarning() << "loading " << path <<" as skin";
                //mItem = new QPixmap();
                foreach(spriteData* sprite,spriteData::getData())
                {
                    if(sprite->getSpritePath()==path)
                    {
                        spritePixmap = sprite;
                        return;
                    }
                }
                spriteData *old = spritePixmap;
                spritePixmap = new spriteData();
                QPixmap temp = QPixmap();

                if(!temp.load(path))
                {
                        spritePixmap = old;
                        qWarning() << "Fail of loading "<< path;
                        return;
                }
                else if(parentItem() == 0)
                {
                        setParentItem(gameData::instance()->getGraphicsItem());
                        gameData::destroyInstanceAtTheLastCall();
                }
                spritePixmap->setSprite(new QPixmap(temp));
                spritePixmap->setPath(path);
                qWarning()<<"new sprite of type "<<path;
                spriteData::add(spritePixmap);
        }
}


// -------------- getter
QPixmap spriteImage::pixmap()
{
        if( spritePixmap !=0 && !spritePixmap->getSprite()->isNull())
                return *spritePixmap->getSprite();
        return QPixmap();
}

// -------------- other
bool spriteImage::isNull()
{
        if(spritePixmap != 0)
                return (mHeight == 0 || mWidth == 0 || spritePixmap->getSprite()->isNull());
        return true;
}

void spriteImage::refresh()
{
    setTransformations(transformations());
}
