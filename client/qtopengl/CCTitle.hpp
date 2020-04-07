#ifndef CCTitle_H
#define CCTitle_H

#include <QGraphicsWidget>

class CCTitle : public QGraphicsItem
{
public:
    CCTitle(QGraphicsItem *parent = nullptr);
    ~CCTitle();
    void setText(const QString &text);
    void setOutlineColor(const QColor &color);
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
    QColor outlineColor;
    int lastwidth,lastheight;
    QString text;

    QRectF m_boundingRect;
};

#endif // CCTitle_H
