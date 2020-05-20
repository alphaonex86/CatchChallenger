#ifndef CCDialogTitle_H
#define CCDialogTitle_H

#include <QGraphicsWidget>
#include <QPen>
#include <QBrush>

class CustomText : public QGraphicsItem
{
public:
    CustomText(QLinearGradient pen_gradient, QLinearGradient brush_gradient, QGraphicsItem *parent = nullptr);
    ~CustomText();
    void setText(const QString &text);
    void setFont(const QFont &font);
    QFont getFont() const;
    bool setPixelSize(uint8_t size);

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *) override;
    QRectF boundingRect() const override;
protected:
    void updateTextPath();
private:
    QPainterPath *textPath;
    QPixmap *cache;
    QFont *font;
    int lastwidth,lastheight;
    QString text;

    QRectF m_boundingRect;
    QLinearGradient m_pen_gradient;
    QLinearGradient m_brush_gradient;
};

#endif // CCDialogTitle_H
