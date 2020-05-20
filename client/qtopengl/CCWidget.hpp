#ifndef CCWidget_H
#define CCWidget_H

#include <QGraphicsWidget>

class ImagesStrechMiddle : public QGraphicsItem
{
public:
    //46,":/CC/images/interface/message.png"
    ImagesStrechMiddle(unsigned int borderSize,QString path,QGraphicsItem *parent = nullptr);
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    unsigned int currentBorderSize() const;
    QRectF boundingRect() const override;
    void setWidth(const int &w);
    void setHeight(const int &h);
    void setSize(const int &w,const int &h);
    int width() const;
    int height() const;
private:
    QPixmap tr,tm,tl,mr,mm,ml,br,bm,bl;
    int oldratio;
    unsigned int imageBorderSize() const;
    int m_w,m_h;
    unsigned int m_borderSize;
    QString m_path;
};

#endif // CCWidget_H
