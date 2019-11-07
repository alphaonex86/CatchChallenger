#ifndef CustomButton_H
#define CustomButton_H

#include <QGraphicsWidget>

class CustomButton : public QGraphicsWidget
{
    Q_OBJECT
public:
    CustomButton(QString pix);
    ~CustomButton();
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    void setText(const QString &text);
    void setFont(const QFont &font);
protected:
    /*void enterEvent(QEvent *e) override;
    void leaveEvent(QEvent *e) override;*/
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
private:
    QPixmap scaledBackground,scaledBackgroundPressed,scaledBackgroundOver;
    QString background;
    bool over;
    bool pressed;
    QString text;
    QFont *font;
signals:
    void clicked();
};

#endif // PROGRESSBARDARK_H
