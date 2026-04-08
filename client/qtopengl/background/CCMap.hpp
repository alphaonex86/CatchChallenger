#ifndef CCMap_H
#define CCMap_H

#include "../../libqtcatchchallenger/maprender/MapController.hpp"
#include "../ScreenInput.hpp"

class ConnexionManager;

class CCMap : public ScreenInput
{
    Q_OBJECT
public:
    CCMap();
    QRectF boundingRect() const;
    void setVar(ConnexionManager *connexionManager);
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr);
    void paintChildItems(QList<QGraphicsItem *> childItems, qreal parentX, qreal parentY, QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr);
    void mousePressEventXY(const QPointF &p, bool &pressValidated,bool &callParentClass) override;
    void mouseReleaseEventXY(const QPointF &p,bool &pressValidated,bool &callParentClass) override;
    void keyPressReset();
    void keyPressEvent(QKeyEvent * event, bool &eventTriggerGeneral);
    void keyReleaseEvent(QKeyEvent *event, bool &eventTriggerGeneral);
public:
    MapController mapController;
private:
    QPointF lastClickedPos;
    bool clicked;

    qreal scale;
    qreal x;
    qreal y;
private slots:
    void onMapControllerStoppedInFrontOf(CatchChallenger::Map_client *map, const COORD_TYPE &x, const COORD_TYPE &y);
    void onMapControllerActionOn(CatchChallenger::Map_client *map, const COORD_TYPE &x, const COORD_TYPE &y);
    void onMapControllerWildFightCollision(CatchChallenger::Map_client *map, const COORD_TYPE &x, const COORD_TYPE &y);
    void onMapControllerBotFightCollision(CatchChallenger::Map_client *map, const COORD_TYPE &x, const COORD_TYPE &y);
signals:
    void pathFindingNotFound();
    void repelEffectIsOver() const;
    void teleportConditionNotRespected(const std::string &text) const;
    void send_player_direction(const CatchChallenger::Direction &the_direction);
    void stopped_in_front_of(const CATCHCHALLENGER_TYPE_MAPID &mapIndex, const COORD_TYPE &x, const COORD_TYPE &y);
    void actionOn(const CATCHCHALLENGER_TYPE_MAPID &mapIndex, const COORD_TYPE &x, const COORD_TYPE &y);
    void actionOnNothing();
    void blockedOn(const MapVisualiserPlayer::BlockedOn &blockOnVar);
    void wildFightCollision(const CATCHCHALLENGER_TYPE_MAPID &mapIndex, const COORD_TYPE &x, const COORD_TYPE &y);
    void botFightCollision(const CATCHCHALLENGER_TYPE_MAPID &mapIndex, const COORD_TYPE &x, const COORD_TYPE &y);
    void error(const std::string &error);
    void errorWithTheCurrentMap();
    void currentMapLoaded();
};

#endif // PROGRESSBARDARK_H
