#ifndef MAP_VISUALISER_PLAYER_H
#define MAP_VISUALISER_PLAYER_H

#include "MapVisualiser.hpp"
#include "../../../general/base/GeneralStructures.hpp"

#include <QSet>
#include <QString>
#include <QTimer>
#include <QTime>

class MapVisualiserPlayer : public MapVisualiser
{
    Q_OBJECT
    friend class MapVisualiserPlayerWithFight;
public:
    explicit MapVisualiserPlayer(const bool &centerOnPlayer=true,const bool &debugTags=false,
                                 const bool &useCache=true, const bool &openGL=false);
    ~MapVisualiserPlayer();
    virtual bool haveMapInMemory(const std::string &mapPath);
    void keyPressEvent(QKeyEvent * event);
    void keyReleaseEvent(QKeyEvent *event);
    std::string lastLocation() const;
    std::string currentMap() const;
    Map_full * currentMapFull() const;
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
            std::string map;
            uint8_t x,y;
        };
        StartPoint startPoint;
        std::vector<std::pair<CatchChallenger::Orientation,uint8_t> > path;
    };
    uint8_t getX();
    uint8_t getY();
    CatchChallenger::Map_client * getMapObject();

    //the datapack
    void setDatapackPath(const std::string &path, const std::string &mainDatapackCode);
    virtual void datapackParsed();
    virtual void datapackParsedMainSub();

    void setInformations(std::unordered_map<uint16_t,uint32_t> *items, std::unordered_map<CATCHCHALLENGER_TYPE_QUEST, CatchChallenger::PlayerQuest> *quests,
                         std::vector<uint8_t> *events, std::unordered_set<uint16_t> *itemOnMap,
                         std::unordered_map<uint16_t, CatchChallenger::PlayerPlant> *plantOnMap);
    void unblock();
    virtual bool teleportTo(const uint32_t &mapId,const uint16_t &x,const uint16_t &y,const CatchChallenger::Direction &direction);
private:
    //player
    Tiled::MapObject * playerMapObject;
    Tiled::Tileset * playerTileset;
    std::string playerSkinPath;
    int moveStep;
    CatchChallenger::Direction direction;
    uint8_t x,y;
    bool inMove;
    bool teleportedOnPush;
    bool stepAlternance;
    std::string mLastLocation;
    bool blocked;
    bool wasPathFindingUsed;
    //monster
    std::vector<CatchChallenger::Direction> pendingMonsterMoves;
    Tiled::MapObject * monsterMapObject;
    Tiled::Tileset * monsterTileset;
    std::string current_monster_map;
    uint8_t monster_x,monster_y;

protected:
    //datapack
    bool mHaveTheDatapack;
    std::string datapackPath;
    std::string datapackMapPathBase;
    std::string datapackMapPathSpec;
    //cache
    std::unordered_map<std::string,Tiled::Tileset *> playerTilesetCache;
    std::unordered_map<std::string,Tiled::Tileset *> monsterTilesetCache;
    std::string lastTileset;
    std::string defaultTileset;

    //display
    bool centerOnPlayer;

    //timer
    QTimer timer;
    QTimer moveTimer;
    QTimer lookToMove;
    QTimer moveAnimationTimer;
    //QTime
    QTime lastAction;//to prevent flood

    //control
    std::unordered_set<int> keyPressed;
    std::unordered_set<int> keyAccepted;
    bool clip;

    //grass
    bool haveGrassCurrentObject;
    Tiled::MapObject * grassCurrentObject;
    bool haveNextCurrentObject;
    Tiled::MapObject * nextCurrentObject;
    Tiled::Tileset * animationTileset;
    uint16_t currentPlayerSpeed;
    bool animationDisplayed;

    std::vector<uint8_t> *events;
    std::unordered_map<uint16_t,uint32_t> *items;
    std::unordered_map<CATCHCHALLENGER_TYPE_QUEST, CatchChallenger::PlayerQuest> *quests;
    std::unordered_set<uint16_t> *itemOnMap;
    std::unordered_map<uint16_t/*dirtOnMap*/,CatchChallenger::PlayerPlant> *plantOnMap;
protected:
    //current player
    CatchChallenger::Player_private_and_public_informations player_informations;
    bool player_informations_is_set;
protected slots:
    virtual void keyPressParse();

    virtual void doMoveAnimation();
    virtual void moveStepSlot();
    virtual void finalPlayerStep(bool parseKey=true);
    virtual bool finalPlayerStepTeleported(bool &isTeleported);
    virtual bool nextPathStep() = 0;//true if have step
    void pathFindingResultInternal(std::vector<PathResolved> &pathList, const std::string &current_map, const uint8_t &x, const uint8_t &y,
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
    void stopAndSend();

    //void setAnimationTilset(std::string animationTilset);
    virtual void resetAll();
    void setSpeed(const SPEED_TYPE &speed);
    virtual bool canGoTo(const CatchChallenger::Direction &direction,CatchChallenger::CommonMap map,COORD_TYPE x,COORD_TYPE y,const bool &checkCollision);
    void mapDisplayedSlot(const std::string &fileName);
    virtual bool asyncMapLoaded(const std::string &fileName,Map_full * tempMapObject);

    void resetMonsterTile();
    virtual bool loadPlayerMap(const std::string &fileName,const uint8_t &x,const uint8_t &y);
    virtual bool insert_player_internal(const CatchChallenger::Player_public_informations &player, const uint32_t &mapId,
                                     const uint16_t &x, const uint16_t &y, const CatchChallenger::Direction &direction,
                                     const std::vector<std::string> &skinFolderList);
    const Tiled::MapObject * getPlayerMapObject() const;
    std::pair<uint8_t,uint8_t> getPos() const;
    bool isInMove() const;
    CatchChallenger::Direction getDirection() const;
    void stopMove();
signals:
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
};

#endif
