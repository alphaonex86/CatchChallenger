#ifndef CCDialogTitle_H
#define CCDialogTitle_H

#include <QGraphicsWidget>

class CCDialogTitle : public QGraphicsItem
{
public:
    CCDialogTitle(QGraphicsItem *parent = nullptr);
    ~CCDialogTitle();
    void setText(const QString &text);
    void setOutlineColor(const QColor &color);
    void setFont(const QFont &font);
    QFont getFont() const;
    bool setPointSize(uint8_t size);

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *) override;
    QRectF boundingRect() const override;
    void setPressed(const bool &pressed);
    void setPos(qreal ax, qreal ay);
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

#endif // CCDialogTitle_H
