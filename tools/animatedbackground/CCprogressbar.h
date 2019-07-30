#ifndef CCprogressbar_H
#define CCprogressbar_H

#include <QProgressBar>
#include <QPaintEvent>
#include <QResizeEvent>

class CCprogressbar : public QProgressBar
{
public:
    CCprogressbar(QWidget *parent = nullptr);
    virtual ~CCprogressbar();
    void paintEvent(QPaintEvent *) override;
    void resizeEvent(QResizeEvent *event) override;
private:
    QPixmap backgroundLeft,backgroundMiddle,backgroundRight;
    QPixmap barLeft,barMiddle,barRight;
    QPainterPath *textPath;
    QFont *font;
    QString oldText;
};

#endif // CCprogressbar_H
