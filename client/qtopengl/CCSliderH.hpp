#ifndef CCSliderH_H
#define CCSliderH_H

#include <QGraphicsItem>
#include <QRectF>
#include <QPixmap>

class CCSliderH : public QObject, public QGraphicsItem
{
    Q_OBJECT
public:
    CCSliderH(QGraphicsItem *parent = nullptr);
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    QRectF boundingRect() const override;
    void setWidth(const int &w);
    int width() const;

    //slider
    void setMaximum(const unsigned int &maximum);
    unsigned int maximum();
    void setValue(const unsigned int &value);
    unsigned int value();

    //event
    void mousePressEventXY(const QPointF &p, bool &pressValidated);
    void mouseReleaseEventXY(const QPointF &p, bool &previousPressValidated);
    void mouseMoveEventXY(const QPointF &p, bool &previousPressValidated);
private:
    QPixmap l,m,r;
    unsigned int m_width;

    //slider
    unsigned int m_maximum;
    unsigned int m_value;
    unsigned int m_value_temp;
    bool m_pressed;
signals:
    void sliderMoved(int value);
    void sliderPressed();
    void sliderReleased();
    void valueChanged(int value);
};

#endif // CCTitle_H
