#ifndef MAP_VISUALISER_PLAYER_WITH_FIGHT_H
#define MAP_VISUALISER_PLAYER_WITH_FIGHT_H

#include "MapVisualiserPlayer.hpp"
#include "../cc/ClientFightEngine.hpp"

#include <QObject>

class MapVisualiserPlayerWithFight : public MapVisualiserPlayer
{
    Q_OBJECT
public:
    explicit MapVisualiserPlayerWithFight(const bool &debugTags=false);
    ~MapVisualiserPlayerWithFight();
    void setBotsAlreadyBeaten(const char * const botAlreadyBeaten);
    void addBeatenBotFight(const uint16_t &botFightId);
    bool haveBeatBot(const uint16_t &botFightId) const;
    void addRepelStep(const uint32_t &repel_step);
protected slots:
    virtual void keyPressParse();
    virtual bool haveStopTileAction();
    virtual bool canGoTo(const CatchChallenger::Direction &direction,CatchChallenger::CommonMap map,COORD_TYPE x,COORD_TYPE y,const bool &checkCollision);
    virtual void resetAll();
protected:
    char * botAlreadyBeaten;
    uint32_t repel_step;
    Tiled::Tileset *fightCollisionBot;
signals:
    void repelEffectIsOver() const;
    void teleportConditionNotRespected(const std::string &text) const;
};

#endif
