#ifndef MAP_VISUALISER_PLAYER_WITH_FIGHT_H
#define MAP_VISUALISER_PLAYER_WITH_FIGHT_H

#include "../../base/render/MapVisualiserPlayer.h"

class MapVisualiserPlayerWithFight : public MapVisualiserPlayer
{
    Q_OBJECT
public:
    explicit MapVisualiserPlayerWithFight(const bool &centerOnPlayer=true, const bool &debugTags=false, const bool &useCache=true, const bool &OpenGL=false);
    void addBotAlreadyBeaten(const QList<quint32> &botAlreadyBeaten);
protected slots:
    virtual void keyPressParse();
    virtual bool haveStopTileAction();
    virtual bool canGoTo(const CatchChallenger::Direction &direction,CatchChallenger::Map map,COORD_TYPE x,COORD_TYPE y,const bool &checkCollision);
    virtual void resetAll();
protected:
    QList<quint32> botAlreadyBeaten;
};

#endif
