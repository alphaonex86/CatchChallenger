#ifndef MAP_VISUALISER_PLAYER_WITH_FIGHT_H
#define MAP_VISUALISER_PLAYER_WITH_FIGHT_H

#include "MapVisualiserPlayer.hpp"
#include "../../general/fight/CommonFightEngine.hpp"
#include "../../general/base/lib.h"

#include <QObject>

class DLL_PUBLIC MapVisualiserPlayerWithFight : public MapVisualiserPlayer
{
    Q_OBJECT
public:
    explicit MapVisualiserPlayerWithFight(const bool &centerOnPlayer=true, const bool &debugTags=false, const bool &useCache=true, const bool &openGL=false);
    ~MapVisualiserPlayerWithFight();
    void addBeatenBotFight(const CATCHCHALLENGER_TYPE_MAPID &mapId,const CATCHCHALLENGER_TYPE_BOTID &botFightId);
    bool haveBeatBot(const CATCHCHALLENGER_TYPE_MAPID &mapId,const CATCHCHALLENGER_TYPE_BOTID &botFightId) const;
    void addRepelStep(const uint32_t &repel_step);
protected slots:
    virtual void keyPressParse();
    virtual bool haveStopTileAction();
    virtual bool canGoTo(const CatchChallenger::Direction &direction, const CATCHCHALLENGER_TYPE_MAPID &mapIndex, const COORD_TYPE &x, const COORD_TYPE &y, const bool &checkCollision);
    virtual void resetAll();
protected:
    uint32_t repel_step;
    Tiled::SharedTileset fightCollisionBot;
    //When true, canGoTo() answers walkable/not WITHOUT firing the user-facing
    //feedback signals (blockedOn / teleportConditionNotRespected). Click-to-walk
    //PROBES every nearby tile with canGoTo() to find a reachable foot tile next
    //to a sign; without this guard each probed water/lava tile pops a spurious
    //"You can't enter without the correct item" tip. Real movement steps leave
    //it false so a genuine bump still gives feedback.
    bool canGoToSilent;
signals:
    void repelEffectIsOver() const;
    void teleportConditionNotRespected(const std::string &text) const;
};

#endif
