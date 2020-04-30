#ifndef CCTextLineInput_H
#define CCTextLineInput_H

#include <QGraphicsItem>
#include <QPaintEvent>
#include <QResizeEvent>

class CCTextLineInput : public QGraphicsItem
{
public:
    CCTextLineInput(QGraphicsItem *parent = nullptr);
    virtual ~CCTextLineInput();
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
    QRectF boundingRect() const override;

    QString m_text;
    unsigned int m_textCursor=0;
private:
    QPixmap backgroundLeft,backgroundMiddle,backgroundRight;
    QPixmap cache;
};

#endif // CCTextLineInput_H
