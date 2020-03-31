#ifndef CCGRAPHICSTEXTITEM_H
#define CCGRAPHICSTEXTITEM_H

#include <QGraphicsTextItem>

class CCGraphicsTextItem : public QGraphicsTextItem
{
public:
    explicit CCGraphicsTextItem(QGraphicsItem *parent = nullptr);
    explicit CCGraphicsTextItem(const QString &text, QGraphicsItem *parent = nullptr);
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
};

#endif // CCGRAPHICSTEXTITEM_H
