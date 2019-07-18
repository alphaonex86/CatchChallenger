#ifndef CUSTOMBUTTON_H
#define CUSTOMBUTTON_H

#include <QGraphicsItem>
#include <QPaintEvent>

class CustomButton : public QGraphicsItem
{
public:
    CustomButton(QString pix,QGraphicsItem *item=nullptr);
    ~CustomButton();
    void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *);
    void setText(const QString &text);
    void setFont(const QFont &font);
    QRectF boundingRect() const;
protected:
/*    void enterEvent(QEvent *e) override;
    void leaveEvent(QEvent *e) override;*/
    void mousePressEvent(QMouseEvent *e);
    void mouseReleaseEvent(QMouseEvent *e);
private:
    QPixmap scaledBackground,scaledBackgroundPressed,scaledBackgroundOver;
    QString background;
    bool over;
    bool pressed;
    QPainterPath *textPath;
    QFont *font;
    QString text;
};

#endif // CUSTOMBUTTON_H
