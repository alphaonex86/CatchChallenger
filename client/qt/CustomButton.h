#ifndef CustomButton_H
#define CustomButton_H

#include <QPushButton>

class CustomButton : public QPushButton
{
public:
    CustomButton(QString pix, QWidget *parent = nullptr);
    ~CustomButton();
    void paintEvent(QPaintEvent *) override;
    void setText(const QString &text);
    void setOutlineColor(const QColor &color);
    void setFont(const QFont &font);
    QFont getFont() const;
    bool updateTextPercent(uint8_t percent);
    bool setPointSize(uint8_t size);
protected:
    void mousePressEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
    void updateTextPath();
private:
    QPixmap *cache;
    QString background;
    bool pressed;
    QPainterPath *textPath;
    QFont *font;
    QColor outlineColor;
    uint8_t percent;
};

#endif // PROGRESSBARDARK_H
