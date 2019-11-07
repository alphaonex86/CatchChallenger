#ifndef CCTitle_H
#define CCTitle_H

#include <QGraphicsWidget>

class CCTitle : public QGraphicsWidget
{
public:
    CCTitle(QWidget *parent = nullptr);
    ~CCTitle();
    void paintEvent(QPaintEvent *) override;
    void setText(const QString &text);
    void setOutlineColor(const QColor &color);
    void setFont(const QFont &font);
    QFont getFont() const;
    bool setPointSize(uint8_t size);
protected:
    void updateTextPath();
private:
    QPainterPath *textPath;
    QPixmap *cache;
    QFont *font;
    QColor outlineColor;
    int lastwidth,lastheight;
};

#endif // CCTitle_H
