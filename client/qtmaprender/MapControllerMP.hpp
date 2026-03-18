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
    void setScale(const float &scaleSize);
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
    virtual bool asyncMapLoaded(const CATCHCHALLENGER_TYPE_MAPID &mapIndex,Map_full * tempMapObject);

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
    //std::unordered_map<std::string,SIMPLIFIED_PLAYER_ID_FOR_MAP> mapUsedByOtherPlayer;

    //datapack
    std::vector<std::string> skinFolderList;

    //the delayed action
    struct DelayedInsert
    {
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
    double scaleSize;
    bool isTeleported;
    static QFont playerpseudofont;
    static QPixmap *imgForPseudoAdmin;
    static QPixmap *imgForPseudoDev;
    static QPixmap *imgForPseudoPremium;
    std::vector<PathResolved> pathList;
public:
    void eventOnMap(CatchChallenger::MapEvent event, Map_full * tempMapObject, COORD_TYPE x, COORD_TYPE y);
private slots:
    void moveOtherPlayerStepSlot();
    void moveOtherPlayerStepSlotWithPlayer(OtherPlayer &otherPlayer);
    void finalOtherPlayerStep(OtherPlayer &otherPlayer);
    void doMoveOtherAnimation();
    /// \warning all ObjectGroupItem destroyed into removeMap()
    virtual void destroyMap(Map_full *map);
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
    void searchPath(std::vector<Map_full> mapList);
    void pathFindingNotFound();
    void pathFindingInternalError();
};

#endif // CATCHCHALLENGER_MAPCONTROLLERMP_H
