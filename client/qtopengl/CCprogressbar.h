#ifndef CCprogressbar_H
#define CCprogressbar_H

#include <QGraphicsWidget>

class CCprogressbar : public QGraphicsItem
{
public:
    CCprogressbar(QGraphicsItem *parent = nullptr);
    virtual ~CCprogressbar();

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    QRectF boundingRect() const override;
    void setPos(qreal ax, qreal ay);
    void setSize(qreal width, qreal height);

    void setMaximum(const int &value);
    void setMinimum(const int &value);
    void setValue(const int &value);
    int maximum();
    int minimum();
    int value();
private:
    QPixmap backgroundLeft,backgroundMiddle,backgroundRight;
    QPixmap barLeft,barMiddle,barRight;
    QPainterPath *textPath;
    QFont *font;
    QString oldText;
    QPixmap *cache;

    QRectF m_boundingRect;
    int m_value,m_min,m_max;
};

#endif // CCprogressbar_H
