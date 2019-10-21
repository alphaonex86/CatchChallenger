#ifndef CCBackground_H
#define CCBackground_H

#include <QWidget>
#include <QPixmap>
#include <QTimer>
#include <chrono>

class CCBackground : public QWidget
{
public:
    CCBackground(QWidget *parent = nullptr);
    void paintEvent(QPaintEvent *) override;
    void startAnimation();
    void stopAnimation();

    static unsigned int timeToDraw;
    static unsigned int timeDraw;
    static std::chrono::time_point<std::chrono::steady_clock> timeFromStart;
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
    QPixmap ddd;
    std::vector<unsigned int> results;
};

#endif // PROGRESSBARDARK_H
