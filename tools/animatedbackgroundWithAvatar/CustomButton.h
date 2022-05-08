#ifndef CustomButton_H
#define CustomButton_H

#include <QGraphicsItem>

class CustomButton : public QObject, public QGraphicsItem
{
    Q_OBJECT
public:
    CustomButton(QString pix,QGraphicsItem *parent = nullptr);
    ~CustomButton();
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    QRectF boundingRect() const override;
    void setText(const QString &text);
    void setFont(const QFont &font);
    void emitclicked();
    void setPressed(const bool &pressed);
    void setPos(qreal ax, qreal ay);
protected:
    /*void enterEvent(QEvent *e) override;
    void leaveEvent(QEvent *e) override;*/
    /*void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;*/

    /*QRectF outlineRect() const;
    QRectF boundingRect() const;
    QPainterPath shape() const;*/
private:
    QPixmap scaledBackground,scaledBackgroundPressed,scaledBackgroundOver;
    QString background;
    bool over;
    bool pressed;
    QString text;
    QFont *font;
    QRectF m_boundingRect;
signals:
    void clicked();
};

#endif // PROGRESSBARDARK_H
