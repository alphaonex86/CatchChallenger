#ifndef CCBackground_H
#define CCBackground_H

#include <QWidget>
#include <QPixmap>
#include <QTimer>
#include <chrono>
#include "../ScreenInput.hpp"

class CCBackground : public QObject, public ScreenInput
{
    Q_OBJECT
public:
    CCBackground();
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *widget) override;
    void startAnimation();
    void stopAnimation();
private:
    QPixmap cloud,grass,sun,treeback,treefront;
    void scalePix(QPixmap &pix,unsigned int zoom);
    void grassSlot();
    void treebackSlot();
    void treefrontSlot();
    void cloudSlot();
    unsigned int getTargetZoom(QWidget *widget);
    void updateSlot();
    QRectF boundingRect() const override;

    unsigned int zoom;
    int grassMove,treebackMove,treefrontMove,cloudMove;
    QTimer grassTimer,treebackTimer,treefrontTimer,cloudTimer;
    QTimer updateTimer;
    std::vector<unsigned int> results;
};

#endif // PROGRESSBARDARK_H
