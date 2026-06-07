#ifndef CATCHCHALLENGER_MAPCONTROLLERMP_H
#define CATCHCHALLENGER_MAPCONTROLLERMP_H

#include "MapVisualiserPlayerWithFight.hpp"
#include "PathFinding.hpp"

#include <string>
#include <vector>
#include <QTimer>
#include <QResizeEvent>
#include "../../general/base/lib.h"

namespace CatchChallenger {
class Api_protocol_Qt;
}

class DLL_PUBLIC MapControllerMP : public MapVisualiserPlayerWithFight
{
    Q_OBJECT
public:
    explicit MapControllerMP(const bool &centerOnPlayer=true, const bool &debugTags=false, const bool &useCache=true, const bool &openGL=false);
    ~MapControllerMP();

    virtual void resetAll();
    virtual void connectAllSignals(CatchChallenger::Api_protocol_Qt *client);
    void setScale(float scaleSize);
    void updateScale();
    void resizeEvent(QResizeEvent *event);

    //the other player
    struct OtherPlayer
    {
        std::string lastTileset;
        Tiled::MapObject * playerMapObject;
        Tiled::SharedTileset playerTileset;
        int moveStep;
        CatchChallenger::Direction direction;
        COORD_TYPE x,y;
        bool inMove;
        bool stepAlternance;
        CATCHCHALLENGER_TYPE_MAPID current_map;
        std::unordered_set<CATCHCHALLENGER_TYPE_MAPID> mapUsed;
        CatchChallenger::Player_public_informations informations;
        Tiled::MapObject * labelMapObject;
        Tiled::SharedTileset labelTileset;
        uint32_t playerSpeed;
        bool animationDisplayed;
        //monster
        std::vector<CatchChallenger::Direction> pendingMonsterMoves;
        Tiled::MapObject * monsterMapObject;
        Tiled::SharedTileset monsterTileset;
        CATCHCHALLENGER_TYPE_MAPID current_monster_map;
        COORD_TYPE monster_x,monster_y;

        //presumed map
        CATCHCHALLENGER_TYPE_MAPID presumed_map;
        COORD_TYPE presumed_x,presumed_y;
        CatchChallenger::Direction presumed_direction;
        //pointer to allow copy of OtherPlayer
        QTimer *oneStepMore;
        QTimer *moveAnimationTimer;
    };
    const std::unordered_map<uint8_t,OtherPlayer> &getOtherPlayerList() const;
public slots:
    //map move Qt
    void insert_player(const SIMPLIFIED_PLAYER_ID_FOR_MAP &simplifiedIndex, const CatchChallenger::Player_public_informations &player,const CATCHCHALLENGER_TYPE_MAPID &mapId,const COORD_TYPE &x,const COORD_TYPE &y,const CatchChallenger::Direction &direction);
    void move_player(const SIMPLIFIED_PLAYER_ID_FOR_MAP &simplifiedIndex, const std::vector<std::pair<uint8_t,CatchChallenger::Direction> > &movement);
    void remove_player(const SIMPLIFIED_PLAYER_ID_FOR_MAP &simplifiedIndex);
    void reinsert_player(const SIMPLIFIED_PLAYER_ID_FOR_MAP &simplifiedIndex, const COORD_TYPE &x, const COORD_TYPE &y, const CatchChallenger::Direction &direction);
    void full_reinsert_player(const SIMPLIFIED_PLAYER_ID_FOR_MAP &simplifiedIndex, const CATCHCHALLENGER_TYPE_MAPID &mapId, const COORD_TYPE &x, const COORD_TYPE &y, const CatchChallenger::Direction &direction);
    void dropAllPlayerOnTheMap();
    //map move
    bool insert_player_final(const SIMPLIFIED_PLAYER_ID_FOR_MAP &simplifiedIndex, const CatchChallenger::Player_public_informations &player,const CATCHCHALLENGER_TYPE_MAPID &mapId,const COORD_TYPE &x,const COORD_TYPE &y,const CatchChallenger::Direction &direction,bool inReplayMode);
    bool move_player_final(const SIMPLIFIED_PLAYER_ID_FOR_MAP &simplifiedIndex, const std::vector<std::pair<uint8_t,CatchChallenger::Direction> > &movement, bool inReplayMode);
    bool move_otherMonster(OtherPlayer &otherPlayer, const bool &haveMoved,
                           const COORD_TYPE &previous_different_x, const COORD_TYPE &previous_different_y, const CATCHCHALLENGER_TYPE_MAPID &previous_different_map_index,
                           CatchChallenger::Direction &previous_different_move, const std::vector<CatchChallenger::Direction> &lastMovedDirection);
    bool remove_player_final(const SIMPLIFIED_PLAYER_ID_FOR_MAP &simplifiedIndex, bool inReplayMode);
    bool reinsert_player_final(const SIMPLIFIED_PLAYER_ID_FOR_MAP &simplifiedIndex, const COORD_TYPE &x, const COORD_TYPE &y, const CatchChallenger::Direction &direction, bool inReplayMode);
    bool full_reinsert_player_final(const SIMPLIFIED_PLAYER_ID_FOR_MAP &simplifiedIndex, const CATCHCHALLENGER_TYPE_MAPID &mapId, const COORD_TYPE &x, const COORD_TYPE &y, const CatchChallenger::Direction &direction, bool inReplayMode);
    bool dropAllPlayerOnTheMap_final(bool inReplayMode);

    bool teleportTo(const CATCHCHALLENGER_TYPE_MAPID &mapIndex,const COORD_TYPE &x,const COORD_TYPE &y,const CatchChallenger::Direction &direction);
    virtual bool asyncMapLoaded(const CATCHCHALLENGER_TYPE_MAPID &mapIndex,QMap_client * tempMapObject);

    //initial position from QthaveCharacter (after character selection)
    void loadCurrentPlayer(const CATCHCHALLENGER_TYPE_MAPID &mapId,const COORD_TYPE &x,const COORD_TYPE &y,const CatchChallenger::Direction &direction);

    //player info
    void have_current_player_info(const CatchChallenger::Player_private_and_public_informations &informations);

    //the datapack
    virtual void datapackParsed();
    virtual void datapackParsedMainSub();
    virtual void reinject_signals();
    virtual void reinject_signals_on_valid_map();
private:
    PathFinding pathFinding;
    std::unordered_map<SIMPLIFIED_PLAYER_ID_FOR_MAP,OtherPlayer> otherPlayerList;
    std::unordered_map<QTimer *,SIMPLIFIED_PLAYER_ID_FOR_MAP> otherPlayerListByTimer,otherPlayerListByAnimationTimer;
    std::unordered_map<CATCHCHALLENGER_TYPE_MAPID,SIMPLIFIED_PLAYER_ID_FOR_MAP> mapUsedByOtherPlayer;//other player count on each map

    //datapack
    std::vector<std::string> skinFolderList;

    //the delayed action
    struct DelayedInsert
    {
        SIMPLIFIED_PLAYER_ID_FOR_MAP id;
        CatchChallenger::Player_public_informations player;
        CATCHCHALLENGER_TYPE_MAPID mapId;
        COORD_TYPE x;
        COORD_TYPE y;
        CatchChallenger::Direction direction;
    };
    struct DelayedMove
    {
        SIMPLIFIED_PLAYER_ID_FOR_MAP id;
        std::vector<std::pair<uint8_t,CatchChallenger::Direction> > movement;
    };
    struct DelayedReinsertSingle
    {
        SIMPLIFIED_PLAYER_ID_FOR_MAP id;
        COORD_TYPE x;
        COORD_TYPE y;
        CatchChallenger::Direction direction;
    };
    struct DelayedReinsertFull
    {
        SIMPLIFIED_PLAYER_ID_FOR_MAP id;
        CATCHCHALLENGER_TYPE_MAPID mapId;
        COORD_TYPE x;
        COORD_TYPE y;
        CatchChallenger::Direction direction;
    };
    enum DelayedType
    {
        DelayedType_Insert,
        DelayedType_Move,
        DelayedType_Remove,
        DelayedType_Reinsert_single,
        DelayedType_Reinsert_full,
        DelayedType_Drop_all
    };
    struct DelayedMultiplex
    {
        DelayedType type;
        DelayedInsert insert;
        DelayedMove move;
        SIMPLIFIED_PLAYER_ID_FOR_MAP remove;
        DelayedReinsertSingle reinsert_single;
        DelayedReinsertFull reinsert_full;
    };
    std::vector<DelayedMultiplex> delayedActions;

    struct DelayedTeleportTo
    {
        CATCHCHALLENGER_TYPE_MAPID mapId;
        COORD_TYPE x;
        COORD_TYPE y;
        CatchChallenger::Direction direction;
    };
    std::vector<DelayedTeleportTo> delayedTeleportTo;

    //initial player position to load after datapack and player info are ready
    bool pendingInitialPlayerLoad;
    DelayedTeleportTo initialPlayerPosition;

    double scaleSize;
    float requestedScaleSize;
    bool isTeleported;
    bool chatHelloSent;
    static QFont playerpseudofont;
    static QPixmap *imgForPseudoAdmin;
    static QPixmap *imgForPseudoDev;
    static QPixmap *imgForPseudoPremium;
    std::vector<PathResolved> pathList;
public:
    void eventOnMap(CatchChallenger::MapEvent event, const CATCHCHALLENGER_TYPE_MAPID &mapIndex, COORD_TYPE x, COORD_TYPE y);
private:
    //--test-clicksign self-test state (see runClickSignSelfTest())
    bool signSelfTestStarted;
    CATCHCHALLENGER_TYPE_MAPID signTestMap;
    uint8_t signTestX,signTestY;
    QTimer signSelfTestTimeoutTimer;
    //true if the player can stand on (x,y): some orthogonal neighbour can move
    //into it (water/lava/collisions -> not standable). Used to retarget a click
    //on a non-standable Sign/NPC tile to its nearest standable neighbour.
    bool tileStandable(const CATCHCHALLENGER_TYPE_MAPID &mapIndex,const int &x,const int &y,const int &mw,const int &mh);
    //For a click on a non-standable tile (Sign/NPC/wall), find the orthogonal
    //neighbour the player can actually WALK to (reachable on foot, BFS from the
    //player) that is nearest the player, so the click leads up to the right side
    //of the sign. Returns false when the sign has no foot-reachable neighbour.
    bool reachableClickNeighbor(const CATCHCHALLENGER_TYPE_MAPID &mapIndex,const int &cx,const int &cy,const int &px,const int &py,int &outX,int &outY);
    //scene-pixel centre of tile (tx,ty) (current map at scene origin, 16px tiles)
    QPointF tileCenterScenePos(const int &tileX,const int &tileY) const;
    //--test-clicksign zoom check: map the sign tile's centre to a viewport pixel
    //through the current ZOOM and back to a tile (PreparedLayer's qCeil maths) —
    //outX/outY must equal the sign tile or the on-device tap hits the wrong tile.
    bool resolveTileAtZoom(const int &tileX,const int &tileY,int &outX,int &outY);
    //--test-clicksign: inject a REAL left-click at the viewport pixel of tile
    //(tx,ty), travelling the full device path (pixel->zoomed view->scene->tile).
    void postClickAtTile(const int &tileX,const int &tileY);
    //--test-clickdoor state: the door round-trip is a state machine advanced on
    //each current-map display (spawn -> teleport to dest -> teleport back).
    int doorTestPhase;
    CATCHCHALLENGER_TYPE_MAPID doorOrigMap,doorDestMap;
    QTimer doorTestTimeoutTimer;
    //BFS the tiles the player can WALK to on mapIndex (silent canGoTo probe).
    void computeReachableSet(const CATCHCHALLENGER_TYPE_MAPID &mapIndex,const int &px,const int &py,std::vector<bool> &reach,int &mw,int &mh);
    //Find a teleporter (door) on mapIndex usable from the player: source tile
    //itself reachable (a DOOR -> click ON it) else a reachable orthogonal
    //neighbour (a PUSH wall -> click NEXT to it and step in). requireDest!=65535
    //restricts to teleporters landing on that map (used to find the return door).
    //Returns the source tile, destination map, push flag and (push) the neighbour.
    bool pickReachableTeleporter(const CATCHCHALLENGER_TYPE_MAPID &mapIndex,const int &px,const int &py,const CATCHCHALLENGER_TYPE_MAPID &requireDest,uint8_t &srcX,uint8_t &srcY,CATCHCHALLENGER_TYPE_MAPID &destMap,bool &isPush,int &neighX,int &neighY);
    //Drive a click to a teleporter: ON the source for a door, or NEXT TO it (and
    //queue a push onto the source on arrival) for a push wall.
    void clickTeleporter(const uint8_t &srcX,const uint8_t &srcY,const bool &isPush,const int &neighX,const int &neighY);
protected:
    //once-on-map hook: launches the --test-clicksign self-test when requested
    virtual void afterMapDisplayed() override;
private slots:
    //--test-clicksign: click the nearest sign, then verify (via actionOn) that
    //the player walked up to it, faced it and opened it like Enter was pressed
    void runClickSignSelfTest();
    void signSelfTestActionOn(CatchChallenger::Map_client *map, const CATCHCHALLENGER_TYPE_MAPID &mapIndex, const COORD_TYPE &x, const COORD_TYPE &y);
    void signSelfTestTimeout();
    //--test-clicksign: a walk step was refused (e.g. item-gated water/lava) ->
    //report "you can't enter ..." as a FAIL immediately instead of timing out
    void signSelfTestBlockedOn(const MapVisualiserPlayer::BlockedOn &blockOnVar);
    //--test-clickdoor: round-trip state machine — click a door to the other map,
    //then click next to the return teleport (push) to come back on the orig map
    void runClickDoorSelfTest();
    void doorTestTimeout();
    //--autosolo=DX,DY: click the tile at player+(DX,DY) and report the outcome
    void runAutosoloClick();
    void autosoloClickActionOn(CatchChallenger::Map_client *map, const CATCHCHALLENGER_TYPE_MAPID &mapIndex, const COORD_TYPE &x, const COORD_TYPE &y);
    void moveOtherPlayerStepSlot();
    void moveOtherPlayerStepSlotWithPlayer(OtherPlayer &otherPlayer);
    void finalOtherPlayerStep(OtherPlayer &otherPlayer);
    void doMoveOtherAnimation();
    /// \warning all ObjectGroupItem destroyed into removeMap()
    virtual void destroyMap(const CATCHCHALLENGER_TYPE_MAPID &mapIndex);
    CatchChallenger::Direction moveFromPath();
    //virtual std::unordered_set<std::string> loadMap(Map_full *map,const bool &display);
    void updateOtherPlayerMonsterTile(OtherPlayer &tempPlayer,const CATCHCHALLENGER_TYPE_MONSTER &monster);
    void resetOtherMonsterTile(OtherPlayer &tempPlayer);
    void loadOtherMonsterFromCurrentMap(const OtherPlayer &tempPlayer);
    void unloadOtherMonsterFromCurrentMap(const OtherPlayer &tempPlayer);
protected slots:
    bool loadPlayerMap(const CATCHCHALLENGER_TYPE_MAPID &mapIndex,const COORD_TYPE &x,const COORD_TYPE &y);
    virtual void finalPlayerStep(bool parseKey=true);
    //call after enter on new map
    virtual void loadOtherPlayerFromMap(const OtherPlayer &otherPlayer, const bool &display=true);
    //call before leave the old map (and before loadPlayerFromCurrentMap())
    virtual void unloadOtherPlayerFromMap(const OtherPlayer &otherPlayer);
    void pathFindingResult(const CATCHCHALLENGER_TYPE_MAPID &mapIndex, const COORD_TYPE &x, const COORD_TYPE &y, const std::vector<std::pair<CatchChallenger::Orientation, uint8_t> > &path, const PathFinding::PathFinding_status &status);
    bool nextPathStep();//true if have step
    virtual void keyPressParse();
signals:
    void searchPath(std::vector<QMap_client> mapList);
    void pathFindingNotFound();
    void pathFindingInternalError();
};

#endif // CATCHCHALLENGER_MAPCONTROLLERMP_H
