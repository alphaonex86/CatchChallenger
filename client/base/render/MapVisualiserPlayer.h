#ifndef MAP_VISUALISER_PLAYER_H
#define MAP_VISUALISER_PLAYER_H

#include "MapVisualiser.h"
#include "../../general/base/GeneralStructures.h"

#include <QSet>
#include <QString>
#include <QTimer>
#include <QTime>

class MapVisualiserPlayer : public MapVisualiser
{
    Q_OBJECT

public:
    explicit MapVisualiserPlayer(const bool &centerOnPlayer=true,const bool &debugTags=false,const bool &useCache=true);
    ~MapVisualiserPlayer();
    void fetchFollowingMonster();
    void fetchPlayer();
    virtual bool haveMapInMemory(const std::string &mapPath);
    void keyPressEvent(QKeyEvent * event);
    void keyReleaseEvent(QKeyEvent *event);
    std::string lastLocation() const;
    std::string currentMap() const;
    MapVisualiserThread::Map_full * currentMapFull() const;
    bool currentMapIsLoaded() const;
    std::string currentMapType() const;
    std::string currentZone() const;
    std::string currentBackgroundsound() const;
    CatchChallenger::Direction getDirection();
    void updateFollowingMonster(int tiledPos = CatchChallenger::DrawSmallTiledPosition::walkLeftFoot_Bottom);
    void transitionMonster(int baseMonster);
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
    uint8_t getX();
    uint8_t getY();
    CatchChallenger::Map_client * getMapObject();

    //the datapack
    void setDatapackPath(const std::string &path, const std::string &mainDatapackCode);
    virtual void datapackParsed();
    virtual void datapackParsedMainSub();

    void setInformations(std::unordered_map<uint16_t,uint32_t> *items, std::unordered_map<uint16_t, CatchChallenger::PlayerQuest> *quests, std::vector<uint8_t> *events, std::unordered_set<uint16_t> *itemOnMap, std::unordered_map<uint16_t, CatchChallenger::PlayerPlant> *plantOnMap);
    void unblock();
protected:
    //datapack
    bool mHaveTheDatapack;
    std::string datapackPath;
    std::string datapackMapPathBase;
    std::string datapackMapPathSpec;
    //player
    Tiled::MapObject* playerMapObject;
    Tiled::Tileset* playerTileset;
    std::string playerSkinPath;
    std::string followingMonsterSkinPath;
    std::unordered_map<std::string,Tiled::Tileset *> playerTilesetCache;
    std::string lastTileset;
    std::string defaultTileset;
    std::string defaultMonsterTileset;
    int moveStep;
    CatchChallenger::Direction direction;
    uint8_t x,y;
    bool inMove;
    bool teleportedOnPush;
    bool stepAlternance;
    std::string mLastLocation;
    bool blocked;
    bool wasPathFindingUsed;

    //monster default info
    CatchChallenger::Player_public_informations followingMonsterInformation;

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
    std::unordered_map<uint16_t, CatchChallenger::PlayerQuest> *quests;
    std::unordered_set<uint16_t> *itemOnMap;
    std::unordered_map<uint16_t/*dirtOnMap*/,CatchChallenger::PlayerPlant> *plantOnMap;
    Tiled::MapObject * followingMonsterMapObject;
    Tiled::Tileset * followingMonsterTileset;
protected:
    static std::string text_slashtrainerpng;
    static std::string text_slashtrainerMonsterpng;
    static std::string text_slash;
    static std::string text_antislash;
    static std::string text_dotpng;
    static std::string text_type;
    static std::string text_zone;
    static std::string text_backgroundsound;
protected slots:
    virtual void keyPressParse();

    virtual void doMoveAnimation();
    virtual void moveStepSlot();
    virtual void finalPlayerStep();
    virtual bool nextPathStep() = 0;//true if have step
    virtual bool haveStopTileAction();
    //have look into another direction, if the key remain pressed, apply like move
    void transformLookToMove();

    //grass
    void loadGrassTile();
    //call after enter on new map
    virtual void loadPlayerFromCurrentMap();
    //call before leave the old map (and before loadPlayerFromCurrentMap())
    virtual void unloadPlayerFromCurrentMap();
    virtual void parseStop();
    virtual void parseAction();
    void stopAndSend();

    //void setAnimationTilset(std::string animationTilset);
    virtual void resetAll();
    void setSpeed(const SPEED_TYPE &speed);
    virtual bool canGoTo(const CatchChallenger::Direction &direction,CatchChallenger::CommonMap map,COORD_TYPE x,COORD_TYPE y,const bool &checkCollision);
    void mapDisplayedSlot(const std::string &fileName);
    virtual bool asyncMapLoaded(const std::string &fileName,MapVisualiserThread::Map_full * tempMapObject);
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
