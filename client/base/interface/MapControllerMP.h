#ifndef CATCHCHALLENGER_MAPCONTROLLERMP_H
#define CATCHCHALLENGER_MAPCONTROLLERMP_H

#include "../../fight/interface/MapVisualiserPlayerWithFight.h"
#include "../Api_protocol.h"
#include "PathFinding.h"

#include <QString>
#include <QList>
#include <QStringList>
#include <QHash>
#include <QTimer>

class MapControllerMP : public MapVisualiserPlayerWithFight
{
    Q_OBJECT
public:
    explicit MapControllerMP(const bool &centerOnPlayer=true, const bool &debugTags=false, const bool &useCache=true, const bool &OpenGL=false);
    ~MapControllerMP();

    virtual void resetAll();
    virtual void connectAllSignals();
    void setScale(const float &scaleSize);
public slots:
    //map move
    void insert_player(const CatchChallenger::Player_public_informations &player,const quint32 &mapId,const quint16 &x,const quint16 &y,const CatchChallenger::Direction &direction);
    void move_player(const quint16 &id,const QList<QPair<quint8,CatchChallenger::Direction> > &movement);
    void remove_player(const quint16 &id);
    void reinsert_player(const quint16 &id,const quint8 &x,const quint8 &y,const CatchChallenger::Direction &direction);
    void full_reinsert_player(const quint16 &id,const quint32 &mapId,const quint8 &x,const quint8 &y,const CatchChallenger::Direction &direction);
    void dropAllPlayerOnTheMap();
    void teleportTo(const quint32 &mapId,const quint16 &x,const quint16 &y,const CatchChallenger::Direction &direction);
    virtual bool asyncMapLoaded(const QString &fileName,MapVisualiserThread::Map_full * tempMapObject);

    //player info
    void have_current_player_info(const CatchChallenger::Player_private_and_public_informations &informations);

    //the datapack
    virtual void datapackParsed();
    virtual void reinject_signals();
    virtual void reinject_signals_on_valid_map();
private:
    //the other player
    struct OtherPlayer
    {
        Tiled::MapObject * playerMapObject;
        Tiled::Tileset * playerTileset;
        int moveStep;
        CatchChallenger::Direction direction;
        quint8 x,y;
        bool inMove;
        bool stepAlternance;
        QString current_map;
        QSet<QString> mapUsed;
        CatchChallenger::Player_public_informations informations;
        Tiled::MapObject * labelMapObject;
        Tiled::Tileset * labelTileset;

        //presumed map
        MapVisualiserThread::Map_full *presumed_map;
        quint8 presumed_x,presumed_y;
        CatchChallenger::Direction presumed_direction;
        QTimer *oneStepMore;
    };
    QList<PathFinding *> pathFindingList;
    QHash<quint16,OtherPlayer> otherPlayerList;
    QHash<QTimer *,quint16> otherPlayerListByTimer;
    QHash<QString,quint16> mapUsedByOtherPlayer;

    //datapack
    QStringList skinFolderList;

    //the delayed action
    struct DelayedInsert
    {
        CatchChallenger::Player_public_informations player;
        quint32 mapId;
        quint16 x;
        quint16 y;
        CatchChallenger::Direction direction;
    };
    struct DelayedMove
    {
        quint16 id;
        QList<QPair<quint8,CatchChallenger::Direction> > movement;
    };
    struct DelayedReinsertSingle
    {
        quint16 id;
        quint8 x;
        quint8 y;
        CatchChallenger::Direction direction;
    };
    struct DelayedReinsertFull
    {
        quint16 id;
        quint32 mapId;
        quint8 x;
        quint8 y;
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
        quint16 remove;
        DelayedReinsertSingle reinsert_single;
        DelayedReinsertFull reinsert_full;
    };
    QList<DelayedMultiplex> delayedActions;

    struct DelayedTeleportTo
    {
        quint32 mapId;
        quint16 x;
        quint16 y;
        CatchChallenger::Direction direction;
    };
    QList<DelayedTeleportTo> delayedTeleportTo;
    float scaleSize;
    bool isTeleported;
    static QFont playerpseudofont;
    static QPixmap *imgForPseudoAdmin;
    static QPixmap *imgForPseudoDev;
    static QPixmap *imgForPseudoPremium;
protected:
    //current player
    CatchChallenger::Player_private_and_public_informations player_informations;
    bool player_informations_is_set;
private slots:
    bool loadPlayerMap(const QString &fileName,const quint8 &x,const quint8 &y);
    void moveOtherPlayerStepSlot();
    /// \warning all ObjectGroupItem destroyed into removeMap()
    virtual void destroyMap(MapVisualiserThread::Map_full *map);
    void eventOnMap(CatchChallenger::MapEvent event,MapVisualiserThread::Map_full * tempMapObject,quint8 x,quint8 y);
    //virtual QSet<QString> loadMap(MapVisualiserThread::Map_full *map,const bool &display);
protected slots:
    virtual void finalPlayerStep();
    //call after enter on new map
    virtual void loadOtherPlayerFromMap(OtherPlayer otherPlayer, const bool &display=true);
    //call before leave the old map (and before loadPlayerFromCurrentMap())
    virtual void unloadOtherPlayerFromMap(OtherPlayer otherPlayer);
    void pathFindingResult(QList<QPair<CatchChallenger::Direction,quint8>> path);
    virtual void keyPressParse();
signals:
    void searchPath(QList<MapVisualiserThread::Map_full> mapList);
};

#endif // CATCHCHALLENGER_MAPCONTROLLERMP_H
