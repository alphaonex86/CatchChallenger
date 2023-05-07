#ifndef CCDialogTitle_H
#define CCDialogTitle_H

#include <QGraphicsWidget>

class CCDialogTitle : public QGraphicsItem
{
public:
    CCDialogTitle(QGraphicsItem *parent = nullptr);
    ~CCDialogTitle();
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
};

#endif // CCDialogTitle_H
