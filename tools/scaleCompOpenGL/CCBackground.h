#ifndef CCBackground_H
#define CCBackground_H

#include <QWidget>
#include <QPixmap>
#include <QTimer>
#include <chrono>
#include <QGraphicsItem>

class CCBackground : public QObject, public QGraphicsItem
{
    Q_OBJECT
public:
    CCBackground(QGraphicsItem *parent = nullptr);
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
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
    unsigned int getTargetZoom(QWidget *widget);
    void updateSlot();
    QRectF boundingRect() const override;

    unsigned int zoom;
    int grassMove,treebackMove,treefrontMove;
    QTimer grassTimer,treebackTimer,treefrontTimer;
    QTimer updateTimer;
    bool benchmark;
    QPixmap ddd;
    QPixmap miniBase;
    std::vector<unsigned int> results;
};

#endif // PROGRESSBARDARK_H
