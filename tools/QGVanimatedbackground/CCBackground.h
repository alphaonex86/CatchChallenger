#ifndef CCBACKGROUND_H
#define CCBACKGROUND_H

#include <QGraphicsItem>
#include <QPaintEvent>
#include <QTimer>

class CCBackground : public QObject, public QGraphicsItem
{
public:
    CCBackground(QGraphicsItem *parent = nullptr);
    void paint(QPainter *paint, const QStyleOptionGraphicsItem *, QWidget *widget);
    QRectF boundingRect() const;
private:
    QPixmap cloud,grass,sun,treeback,treefront;
    void scalePix(QPixmap &pix,unsigned int zoom);
    void grassSlot();
    void treebackSlot();
    void treefrontSlot();
    unsigned int getTargetZoom(const QWidget * const widget);
    void update();

    unsigned int zoom;
    int grassMove,treebackMove,treefrontMove;
    QTimer grassTimer,treebackTimer,treefrontTimer;
    QTimer updateTimer;
    bool benchmark;
    std::vector<unsigned int> results;
};

#endif // CCBACKGROUND_H
