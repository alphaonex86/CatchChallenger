#ifndef CCprogressbar_H
#define CCprogressbar_H

#include <QGraphicsWidget>

class CCprogressbar : public QGraphicsWidget
{
public:
    CCprogressbar(QWidget *parent = nullptr);
    virtual ~CCprogressbar();
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
private:
    QPixmap backgroundLeft,backgroundMiddle,backgroundRight;
    QPixmap barLeft,barMiddle,barRight;
    QPainterPath *textPath;
    QFont *font;
    QString oldText;
};

#endif // CCprogressbar_H
