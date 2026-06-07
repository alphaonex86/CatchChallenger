#ifndef MAP_VISUALISER_PLAYER_H
#define MAP_VISUALISER_PLAYER_H

#include "MapVisualiser.hpp"
#include "../../general/base/GeneralStructures.hpp"
#include "../libqtcatchchallenger/Api_protocol_Qt.hpp"
#include "../../general/base/lib.h"

#include <QSet>
#include <QString>
#include <QTimer>
#include <QTime>
#include <QElapsedTimer>

class DLL_PUBLIC MapVisualiserPlayer : public MapVisualiser
{
    Q_OBJECT
    friend class MapVisualiserPlayerWithFight;
public:
    explicit MapVisualiserPlayer(const bool &centerOnPlayer=true,const bool &debugTags=false,
                                 const bool &useCache=true, const bool &openGL=false);
    ~MapVisualiserPlayer();
    std::pair<COORD_TYPE,COORD_TYPE> getPos() const;
    const Tiled::MapObject * getPlayerMapObject() const;
    bool isInMove() const;
    CatchChallenger::Direction getDirection() const;
    virtual bool haveMapInMemory(const CATCHCHALLENGER_TYPE_MAPID &mapIndex);
    void keyPressEvent(QKeyEvent * event);
    void keyReleaseEvent(QKeyEvent *event);
    //CATCHCHALLENGER_TYPE_MAPID lastLocation() const;
    CATCHCHALLENGER_TYPE_MAPID currentMap() const;
    QMap_client * currentMapFull() const;
    bool currentMapIsLoaded() const;
    std::string currentMapType() const;
    std::string currentZone() const;
    std::string currentBackgroundsound() const;
    CatchChallenger::Direction getDirection();
    void updatePlayerMonsterTile(const uint16_t &monster);
    void setClip(const bool &clip);
    enum BlockedOn
    {
        BlockedOn_ZoneItem,
        BlockedOn_ZoneFight,
        BlockedOn_RandomNumber,
        BlockedOn_Fight
    };
    enum LandFight
    {
        LandFight_Water,
        LandFight_Grass,
        LandFight_Cave
    };
    struct PathResolved
    {
        struct StartPoint
        {
            CATCHCHALLENGER_TYPE_MAPID map;
            COORD_TYPE x,y;
        };
        StartPoint startPoint;
        std::vector<std::pair<CatchChallenger::Orientation,uint8_t> > path;
    };
    COORD_TYPE getX();
    COORD_TYPE getY();
    CatchChallenger::Map_client * getMapObject();

    //the datapack
    void setDatapackPath(const std::string &path, const std::string &mainDatapackCode);
    virtual void datapackParsed();
    virtual void datapackParsedMainSub();

    void unblock();
    virtual bool teleportTo(const CATCHCHALLENGER_TYPE_MAPID &mapId,const COORD_TYPE &x,const COORD_TYPE &y,const CatchChallenger::Direction &direction);
    //center the QGraphicsView on the integer player tile (x,y). The current map is at scene origin (0,0), tile pixel size = CLIENT_BASE_TILE_SIZE
    void centerOnPlayerTile();
private:
    //player
    Tiled::MapObject * playerMapObject;
    Tiled::SharedTileset playerTileset;
    std::string playerSkinPath;
    int moveStep;
    CatchChallenger::Direction direction;
    uint8_t x,y;
    bool inMove;
    bool teleportedOnPush;
    bool stepAlternance;
    //std::string mLastLocation;last displayed info?
    bool blocked;
    bool wasPathFindingUsed;
    //monster
    std::vector<CatchChallenger::Direction> pendingMonsterMoves;
    Tiled::MapObject * monsterMapObject;
    Tiled::SharedTileset monsterTileset;
    CATCHCHALLENGER_TYPE_MAPID current_monster_map;
    COORD_TYPE monster_x,monster_y;

protected:
    //true while a tile-step animation is in progress (read-only for subclasses;
    //used by the --test-keyboard walk to wait between synthesised arrow presses)
    bool playerIsMoving() const { return inMove; }
    CatchChallenger::Api_protocol_Qt * client;
    //datapack
    bool mHaveTheDatapack;
    std::string datapackPath;
    std::string datapackMapPathBase;
    std::string datapackMapPathSpec;
    //cache
    std::unordered_map<std::string,Tiled::SharedTileset> playerTilesetCache;
    std::unordered_map<std::string,Tiled::SharedTileset> monsterTilesetCache;
    std::string lastTileset;
    std::string defaultTileset;

    //display
    bool centerOnPlayer;

    //Unload our player/monster sprites from the map before the base class
    //destroys the Tiled::Map. ObjectGroup::~ObjectGroup() qDeleteAll's its
    //objects; if ours is still in the group we end up with a dangling pointer
    //that crashes at ~MapVisualiserPlayer.
    virtual void destroyMap(const CATCHCHALLENGER_TYPE_MAPID &mapIndex) override;

    //timer
    QTimer timer;
    QTimer moveTimer;
    QTimer lookToMove;
    QTimer moveAnimationTimer;
    //QTime
    QElapsedTimer lastAction;//to prevent flood

    //control
    std::unordered_set<int> keyPressed;
    std::unordered_set<int> keyAccepted;
    bool clip;

    //Click-to-interact: when the player clicks a tile (e.g. a Sign/NPC, which
    //sits on a collision tile), pathfinding routes the player ADJACENT to it.
    //We remember the clicked tile here so that, once the path completes, the
    //player turns to FACE that tile and parseAction() opens it like Enter was
    //pressed. Set by MapControllerMP::eventOnMap() on a simple click; consumed
    //by faceClickInteractTargetIfAdjacent(); cleared on manual move / reset.
    bool clickInteractTargetValid;
    CATCHCHALLENGER_TYPE_MAPID clickInteractTargetMap;
    uint8_t clickInteractTargetX,clickInteractTargetY;

    //"click NEXT to a teleport then push in": when the click leads up to a tile
    //adjacent to a teleporter we want to ENTER (not just face — a door/push wall),
    //the player takes one more step ONTO the teleporter source on arrival, which
    //triggers finalPlayerStepTeleported(). Set by clickTeleporter(); consumed by
    //pushTeleportIfAdjacent() in the same arrival hook as the sign-facing.
    bool clickInteractPushValid;
    CATCHCHALLENGER_TYPE_MAPID clickInteractPushMap;
    uint8_t clickInteractPushX,clickInteractPushY;

    //grass
    bool haveGrassCurrentObject;
    Tiled::MapObject * grassCurrentObject;
    bool haveNextCurrentObject;
    Tiled::MapObject * nextCurrentObject;
    Tiled::SharedTileset animationTileset;
    bool animationDisplayed;

    /*all of this variable is now into CatchChallenger::Api_protocol_Qt * client;
    std::vector<uint8_t> *events;
    std::unordered_map<uint16_t,uint32_t> *items;
    std::unordered_map<CATCHCHALLENGER_TYPE_QUEST, CatchChallenger::PlayerQuest> *quests;
    std::map<CATCHCHALLENGER_TYPE_MAPID,CatchChallenger::Player_private_and_public_informations_Map> mapData;
protected:
    //current player
    CatchChallenger::Player_private_and_public_informations player_informations;*/
    bool player_informations_is_set;
protected slots:
    virtual void keyPressParse();

    virtual void doMoveAnimation();
    virtual void moveStepSlot();
    virtual void finalPlayerStep(bool parseKey=true);
    virtual bool finalPlayerStepTeleported(bool &isTeleported);
    virtual bool nextPathStep() = 0;//true if have step
    void pathFindingResultInternal(std::vector<PathResolved> &pathList, const CATCHCHALLENGER_TYPE_MAPID &current_map, const COORD_TYPE &x, const COORD_TYPE &y,
                                   const std::vector<std::pair<CatchChallenger::Orientation, uint8_t> > &path);
    bool nextPathStepInternal(std::vector<PathResolved> &pathList, const CatchChallenger::Direction &direction);//true if have step
    virtual bool haveStopTileAction();
    //have look into another direction, if the key remain pressed, apply like move
    void transformLookToMove();
    void forcePlayerTileset(QString path);// for /tools/map-visualiser/

    //grass
    void loadGrassTile();
    //call after enter on new map
    virtual void loadPlayerFromCurrentMap();
    virtual void loadMonsterFromCurrentMap();
    //call before leave the old map (and before loadPlayerFromCurrentMap())
    virtual void unloadPlayerFromCurrentMap();
    virtual void unloadMonsterFromCurrentMap();
    virtual void parseStop();
    virtual void parseAction();
    //If a click-to-interact target was recorded and the player ended up
    //orthogonally adjacent to it (the usual case for a Sign/NPC on a collision
    //tile), turn to face it (sprite + send_player_direction) so the following
    //parseAction() acts on that tile. Always consumes the pending target.
    void faceClickInteractTargetIfAdjacent();
    //If a "push into a teleport" target was recorded and the player ended up
    //orthogonally adjacent to it, take one more step ONTO the teleporter source
    //so finalPlayerStepTeleported() fires. Returns true if a push step was taken
    //(so the caller skips the sign-facing/parseAction path). Consumes the target.
    bool pushTeleportIfAdjacent();
    void stopAndSend();

    //void setAnimationTilset(std::string animationTilset);
    virtual void resetAll();
    virtual bool canGoTo(const CatchChallenger::Direction &direction,const CATCHCHALLENGER_TYPE_MAPID &mapIndex,const COORD_TYPE &x,const COORD_TYPE &y,const bool &checkCollision);
    void mapDisplayedSlot(const CATCHCHALLENGER_TYPE_MAPID &mapIndex);
    //called from mapDisplayedSlot() once the current map is shown; default is a
    //no-op, MapControllerMP overrides it to run the --test-clicksign self-test
    virtual void afterMapDisplayed();
    virtual bool asyncMapLoaded(const CATCHCHALLENGER_TYPE_MAPID &mapIndex,QMap_client * tempMapObject);

    void resetMonsterTile();
    virtual bool loadPlayerMap(const CATCHCHALLENGER_TYPE_MAPID &mapIndex,const uint8_t &x,const uint8_t &y);
    virtual bool insert_player_internal(const CatchChallenger::Player_public_informations &player, const uint32_t &mapId,
                                     const uint16_t &x, const uint16_t &y, const CatchChallenger::Direction &direction,
                                     const std::vector<std::string> &skinFolderList);
    void stopMove();
signals:
    void send_player_direction(const CatchChallenger::Direction &the_direction);
    //Escape pressed on the map: ask the client to close any open Sign/NPC dialog.
    void escapePressed();
    void stopped_in_front_of(CatchChallenger::Map_client *map, const CATCHCHALLENGER_TYPE_MAPID &mapIndex, const COORD_TYPE &x, const COORD_TYPE &y);
    void actionOn(CatchChallenger::Map_client *map, const CATCHCHALLENGER_TYPE_MAPID &mapIndex, const COORD_TYPE &x, const COORD_TYPE &y);
    void actionOnNothing();
    void blockedOn(const MapVisualiserPlayer::BlockedOn &blockOnVar);
    void wildFightCollision(CatchChallenger::Map_client *map, const COORD_TYPE &x, const COORD_TYPE &y);
    void botFightCollision(CatchChallenger::Map_client *map, const COORD_TYPE &x, const COORD_TYPE &y);
    void error(const std::string &error);
    void errorWithTheCurrentMap();
    void inWaitingOfMap();
    void currentMapLoaded();
};

#endif
