#ifndef CCMap_H
#define CCMap_H

#include "../render/MapController.hpp"
#include "../ScreenInput.hpp"

class ConnexionManager;

class CCMap : public QObject, public ScreenInput
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
    void keyPressEvent(QKeyEvent * event);
    void keyReleaseEvent(QKeyEvent *event);
private:
    MapController mapController;
    QPointF lastClickedPos;
    bool clicked;

    qreal scale;
    qreal x;
    qreal y;
signals:
    void searchPath(std::vector<Map_full> mapList);
    void pathFindingNotFound();
    void repelEffectIsOver() const;
    void teleportConditionNotRespected(const std::string &text) const;
    void send_player_direction(const CatchChallenger::Direction &the_direction);
    void stopped_in_front_of(CatchChallenger::Map_client *map, const uint8_t &x, const uint8_t &y);
    void actionOn(CatchChallenger::Map_client *map, const uint8_t &x, const uint8_t &y);
    void actionOnNothing();
    void blockedOn(const MapVisualiserPlayer::BlockedOn &blockOnVar);
    void wildFightCollision(CatchChallenger::Map_client *map, const uint8_t &x, const uint8_t &y);
    void botFightCollision(CatchChallenger::Map_client *map, const uint8_t &x, const uint8_t &y);
    void error(const std::string &error);
    void errorWithTheCurrentMap();
    void inWaitingOfMap();
    void currentMapLoaded();
    void loadOtherMapAsync(const std::string &fileName);
    void mapDisplayed(const std::string &fileName);
};

#endif // PROGRESSBARDARK_H
