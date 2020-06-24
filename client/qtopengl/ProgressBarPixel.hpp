#ifndef ProgressBarPixel_H
#define ProgressBarPixel_H

#include <QGraphicsWidget>

class ProgressBarPixel : public QGraphicsItem
{
public:
    ProgressBarPixel(QGraphicsItem *parent = nullptr,const bool onlygreen=false);
    virtual ~ProgressBarPixel();

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
    QPainterPath *textPath;
    QFont *font;
    QString oldText;
    QPixmap *cache;
    bool onlygreen;

    QRectF m_boundingRect;
    int m_value,m_min,m_max;
};

#endif // ProgressBarPixel_H
