#ifndef MULTIITEM_H
#define MULTIITEM_H

#include <QGraphicsItem>
#include "ConnexionInfo.hpp"

class MultiItem : public QGraphicsItem
{
public:
    MultiItem(const ConnexionInfo &connexionInfo,QGraphicsItem *parent = nullptr);
    ~MultiItem();
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    QRectF boundingRect() const override;
    void setWidth(const int &w);
    void setHeight(const int &h);
    void setSize(const int &w,const int &h);
    int width() const;
    int height() const;
    void setSelected(bool selected);
    bool isSelected() const;
    void mousePressEventXY(const QPointF &p,bool &pressValidated);
    void mouseReleaseEventXY(const QPointF &p, bool &pressValidated);
    const ConnexionInfo &connexionInfo() const;
private:
    QRectF m_boundingRect;
    ConnexionInfo m_connexionInfo;
    QPixmap *cache;
    int lastwidth,lastheight;
    bool m_selected;
    bool pressed;
};

#endif // MULTIITEM_H
