#ifndef MAP_VISUALISER_PLAYER_H
#define MAP_VISUALISER_PLAYER_H

#include "MapVisualiser.h"
#include "../../general/base/GeneralStructures.h"

#include <QSet>
#include <QString>
#include <QTimer>

class MapVisualiserPlayer : public MapVisualiser
{
    Q_OBJECT

public:
    explicit MapVisualiserPlayer(const bool &centerOnPlayer=true,const bool &debugTags=false,const bool &useCache=true,const bool &OpenGL=false);
    ~MapVisualiserPlayer();
    virtual bool haveMapInMemory(const QString &mapPath);
    void keyPressEvent(QKeyEvent * event);
    void keyReleaseEvent(QKeyEvent *event);
    QString lastLocation() const;
    QString currentMap() const;
    MapVisualiserThread::Map_full * currentMapFull() const;
    bool currentMapIsLoaded() const;
    QString currentMapType() const;
    QString currentZone() const;
    QString currentBackgroundsound() const;
    CatchChallenger::Direction getDirection();
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
    void setDatapackPath(const QString &path, const QString &mainDatapackCode);
    virtual void datapackParsed();
    virtual void datapackParsedMainSub();

    void setInformations(std::unordered_map<uint16_t,uint32_t> *items, std::unordered_map<uint16_t, CatchChallenger::PlayerQuest> *quests, std::vector<uint8_t> *events, std::unordered_set<uint8_t> *itemOnMap, std::unordered_map<uint8_t/*dirtOnMap*/,CatchChallenger::PlayerPlant> *plantOnMap);
    void unblock();
protected:
    //datapack
    bool mHaveTheDatapack;
    QString datapackPath;
    QString datapackMapPathBase;
    QString datapackMapPathSpec;
    //player
    Tiled::MapObject * playerMapObject;
    Tiled::Tileset * playerTileset;
    QString playerSkinPath;
    QHash<QString,Tiled::Tileset *> playerTilesetCache;
    QString lastTileset;
    QString defaultTileset;
    int moveStep;
    CatchChallenger::Direction direction;
    uint8_t x,y;
    bool inMove;
    bool teleportedOnPush;
    bool stepAlternance;
    QString mLastLocation;
    bool blocked;

    //display
    bool centerOnPlayer;

    //timer
    QTimer timer;
    QTimer moveTimer;
    QTimer lookToMove;
    QTimer moveAnimationTimer;

    //control
    QSet<int> keyPressed;
    QSet<int> keyAccepted;

    //grass
    bool haveGrassCurrentObject;
    Tiled::MapObject * grassCurrentObject;
    bool haveNextCurrentObject;
    Tiled::MapObject * nextCurrentObject;
    Tiled::Tileset * animationTileset;
    uint32_t currentPlayerSpeed;
    bool animationDisplayed;

    std::vector<uint8_t> *events;
    std::unordered_map<uint16_t,uint32_t> *items;
    std::unordered_map<uint16_t, CatchChallenger::PlayerQuest> *quests;
    std::unordered_set<uint8_t> *itemOnMap;
    std::unordered_map<uint8_t/*dirtOnMap*/,CatchChallenger::PlayerPlant> *plantOnMap;
protected:
    static QString text_slashtrainerpng;
    static QString text_slash;
    static QString text_antislash;
    static QString text_dotpng;
    static QString text_type;
    static QString text_zone;
    static QString text_backgroundsound;
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

    //void setAnimationTilset(QString animationTilset);
    virtual void resetAll();
    void setSpeed(const SPEED_TYPE &speed);
    virtual bool canGoTo(const CatchChallenger::Direction &direction,CatchChallenger::CommonMap map,COORD_TYPE x,COORD_TYPE y,const bool &checkCollision);
    void mapDisplayedSlot(const QString &fileName);
    virtual bool asyncMapLoaded(const QString &fileName,MapVisualiserThread::Map_full * tempMapObject);
signals:
    void send_player_direction(const CatchChallenger::Direction &the_direction);
    void stopped_in_front_of(CatchChallenger::Map_client *map, const uint8_t &x, const uint8_t &y);
    void actionOn(CatchChallenger::Map_client *map, const uint8_t &x, const uint8_t &y);
    void actionOnNothing();
    void blockedOn(const MapVisualiserPlayer::BlockedOn &blockOnVar);
    void wildFightCollision(CatchChallenger::Map_client *map, const uint8_t &x, const uint8_t &y);
    void botFightCollision(CatchChallenger::Map_client *map, const uint8_t &x, const uint8_t &y);
    void error(const QString &error);
    void errorWithTheCurrentMap();
    void inWaitingOfMap();
    void currentMapLoaded();
};

#endif
