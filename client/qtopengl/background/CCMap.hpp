#ifndef CCMap_H
#define CCMap_H

#include "../ScreenInput.hpp"
#include "../render/MapController.hpp"

class ConnexionManager;

class CCMap : public MapController, public ScreenInput
{
    Q_OBJECT
public:
    CCMap();
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *o, QWidget *widget) override;
    void setVar(ConnexionManager *connexionManager);
};

#endif // PROGRESSBARDARK_H
