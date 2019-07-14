#ifndef CCBackground_H
#define CCBackground_H

#include <QWidget>
#include <QPixmap>

class CCBackground : public QWidget
{
public:
    CCBackground(QWidget *parent = nullptr);
    void paintEvent(QPaintEvent *) override;
    void resizeEvent(QResizeEvent*) override;
private:
    QPixmap cloud,grass,sun,treeback,treefront;
    void scalePix(QPixmap &pix,unsigned int zoom);
    unsigned int zoom;
};

#endif // PROGRESSBARDARK_H
