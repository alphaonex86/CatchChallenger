#ifndef MAP_VISUALISER_PLAYER_H
#define MAP_VISUALISER_PLAYER_H

#include "MapVisualiser.h"

#include <QSet>
#include <QString>
#include <QTimer>

class MapVisualiserPlayer : public MapVisualiser
{
    Q_OBJECT

public:
    explicit MapVisualiserPlayer(const bool &centerOnPlayer=true,const bool &debugTags=false,const bool &useCache=true,const bool &OpenGL=false);
    ~MapVisualiserPlayer();
    void keyPressEvent(QKeyEvent * event);
    void keyReleaseEvent(QKeyEvent *event);
    QString lastLocation() const;
    CatchChallenger::Direction getDirection();
    enum BlockedOn
    {
        BlockedOn_Grass,
        BlockedOn_Wather,
        BlockedOn_Cave,
        BlockedOn_RandomNumber
    };
    enum LandFight
    {
        LandFight_Water,
        LandFight_Grass,
        LandFight_Cave
    };
protected:
    //player
    Tiled::MapObject * playerMapObject;
    Tiled::Tileset * playerTileset;
    int moveStep;
    CatchChallenger::Direction direction;
    quint8 x,y;
    bool inMove;
    bool stepAlternance;
    QString mLastLocation;

    //display
    bool centerOnPlayer;

    //timer
    QTimer timer;
    QTimer moveTimer;
    QTimer lookToMove;

    //control
    QSet<int> keyPressed;
    QSet<int> keyAccepted;

    //grass
    bool haveGrassCurrentObject;
    Tiled::MapObject * grassCurrentObject;
    bool haveNextCurrentObject;
    Tiled::MapObject * nextCurrentObject;
    Tiled::Tileset * animationTileset;
protected slots:
    virtual void keyPressParse();

    void moveStepSlot();
    virtual bool haveStopTileAction();
    //have look into another direction, if the key remain pressed, apply like move
    void transformLookToMove();

    //grass
    virtual void startGrassAnimation(const CatchChallenger::Direction &direction);
    virtual void stopGrassAnimation();
    void loadGrassTile();
    //call after enter on new map
    virtual void loadPlayerFromCurrentMap();
    //call before leave the old map (and before loadPlayerFromCurrentMap())
    virtual void unloadPlayerFromCurrentMap();
    virtual void parseStop();
    virtual void parseAction();

    void setAnimationTilset(QString animationTilset);
    virtual void resetAll();
    void setSpeed(const SPEED_TYPE &speed);
    virtual bool canGoTo(const CatchChallenger::Direction &direction,CatchChallenger::Map map,COORD_TYPE x,COORD_TYPE y,const bool &checkCollision);
signals:
    void send_player_direction(const CatchChallenger::Direction &the_direction);
    void stopped_in_front_of(CatchChallenger::Map_client *map, const quint8 &x, const quint8 &y);
    void actionOn(CatchChallenger::Map_client *map, const quint8 &x, const quint8 &y);
    void blockedOn(const MapVisualiserPlayer::BlockedOn &blockOnVar);
    void fightCollision(CatchChallenger::Map_client *map, const quint8 &x, const quint8 &y);
};

#endif
