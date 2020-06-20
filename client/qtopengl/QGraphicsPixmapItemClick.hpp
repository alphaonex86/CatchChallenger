#ifndef QGRAPHICSPIXMAPITEMCLICK_H
#define QGRAPHICSPIXMAPITEMCLICK_H

#include <QGraphicsPixmapItem>
#include <QObject>

class QGraphicsPixmapItemClick : public QObject, public QGraphicsPixmapItem
{
    Q_OBJECT
public:
    QGraphicsPixmapItemClick(const QPixmap &pixmap, QGraphicsItem *parent = nullptr);
    void mousePressEventXY(const QPointF &p,bool &pressValidated);
    void mouseReleaseEventXY(const QPointF &p, bool &previousPressValidated);
private:
    void setPressed(const bool &pressed);
private:
    bool pressed;
signals:
    void clicked();
};

#endif // QGRAPHICSPIXMAPITEMCLICK_H
