#ifndef SPRITEIMAGE_H
#define SPRITEIMAGE_H

#include <QGraphicsItem>
//#include <QObject>

class spriteData;

class spriteImage : public QGraphicsItem//, public QObject
{
    //Q_OBJECT
public:
    explicit spriteImage(QGraphicsItem *parent = 0);
    ~spriteImage();
    // ----- QGraphicsItem
    QRectF boundingRect() const;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
    // ----- setter
    void setArea(QRectF area);
    void setArea(QPointF point1, QPointF point2);
    void setSpriteHeight(quint8 height);
    void setSpriteWidth(quint8 width);
    bool setSprite(int x, int y);
    bool setSprite(int id);
    void setOffset(int x, int y);
    void setOffset(QPointF point);
    void setPixmap(QPixmap* pixmap);
    void setPixmap(QString path);
    // ----- getter
    QPixmap pixmap();
    // ----- other
    bool isNull();
    void refresh();
private:
    QPixmap*    tItem;
    QRectF      mArea;
    QPointF     mOffset;
    quint8      mHeight;
    quint8      mWidth;
    spriteData* spritePixmap;
    bool        useSpriteData;
};

#endif // SPRITEIMAGE_H
