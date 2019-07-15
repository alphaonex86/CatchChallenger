#ifndef CustomButton_H
#define CustomButton_H

#include <QPushButton>

class CustomButton : public QPushButton
{
public:
    CustomButton(QPixmap pix,QWidget *parent = nullptr);
    void paintEvent(QPaintEvent *) override;
    void setText(const QString &text);
protected:
    void enterEvent(QEvent *e) override;
    void leaveEvent(QEvent *e) override;
private:
    QPixmap background;
    bool over;
    QPainterPath textPath;
};

#endif // PROGRESSBARDARK_H
