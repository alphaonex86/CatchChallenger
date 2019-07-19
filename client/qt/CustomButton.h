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
    void setFont(const QFont &font);
protected:
    void enterEvent(QEvent *e) override;
    void leaveEvent(QEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
private:
    QPixmap scaledBackground,scaledBackgroundPressed,scaledBackgroundOver;
    QString background;
    bool over;
    bool pressed;
    QPainterPath *textPath;
    QFont *font;
};

#endif // PROGRESSBARDARK_H
