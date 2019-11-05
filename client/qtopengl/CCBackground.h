#ifndef CCBackground_H
#define CCBackground_H

#include <QWidget>
#include <QPixmap>
#include <QTimer>

class CCBackground : public QWidget
{
public:
    CCBackground(QWidget *parent = nullptr);
    void paintEvent(QPaintEvent *) override;
    void startAnimation();
    void stopAnimation();
private:
    QPixmap cloud,grass,sun,treeback,treefront;
    void scalePix(QPixmap &pix,unsigned int zoom);
    void grassSlot();
    void treebackSlot();
    void treefrontSlot();
    unsigned int getTargetZoom();
    void update();

    unsigned int zoom;
    int grassMove,treebackMove,treefrontMove;
    QTimer grassTimer,treebackTimer,treefrontTimer;
    QTimer updateTimer;
    bool benchmark;
    std::vector<unsigned int> results;
};

#endif // PROGRESSBARDARK_H
