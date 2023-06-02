#ifndef CustomButton_H
#define CustomButton_H

#include <QGraphicsItem>
#include <QPointF>

class CustomButton : public QObject, public QGraphicsItem
{
    Q_OBJECT
public:
    CustomButton(QString pix, QGraphicsItem *parent = nullptr);
    ~CustomButton();
    void setImage(QString pix);
    void setText(const QString &text);
    void setOutlineColor(const QColor &color);
    void setFont(const QFont &font);
    QFont getFont() const;
    bool updateTextPercent(uint8_t percent);
    bool setPixelSize(uint8_t size);

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) override;
    QRectF boundingRect() const override;
    void setPressed(const bool &pressed);
    void setPos(qreal ax, qreal ay);
    void setSize(uint16_t w,uint16_t h);
    void setEnabled(bool enabled);
    void setCheckable(bool checkable);
    bool isChecked() const;
    int width();
    int height();

    //event
    void mousePressEventXY(const QPointF &p, bool &pressValidated);
    void mouseReleaseEventXY(const QPointF &p, bool &previousPressValidated);
signals:
    void clicked();
protected:
    void updateTextPath();
private:
    QPixmap *cache;
    QString background;
    bool pressed;
    QPainterPath *textPath;
    QFont *font;
    QColor outlineColor;
    uint8_t percent;
    QString m_text;
    bool m_checkable;
    bool m_checked;

    QRectF m_boundingRect;
};

#endif // PROGRESSBARDARK_H
