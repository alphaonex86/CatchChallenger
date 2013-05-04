#ifndef CATCHCHALLENGER_PROTOCOL_H
#define CATCHCHALLENGER_PROTOCOL_H

#include <QObject>
#include <QString>
#include <QCoreApplication>
#include <QString>
#include <QByteArray>
#include <QList>
#include <QPair>
#include <QDir>
#include <QFileInfo>
#include <QDateTime>
#include <QCryptographicHash>

#include "../../general/base/DebugClass.h"
#include "../../general/base/GeneralStructures.h"
#include "../../general/base/GeneralVariable.h"
#include "../../general/base/ProtocolParsing.h"
#include "../../general/base/MoveOnTheMap.h"

namespace CatchChallenger {
class Api_protocol : public ProtocolParsingInput, public MoveOnTheMap
{
    Q_OBJECT
public:
    explicit Api_protocol(ConnectedSocket *socket,bool tolerantMode=false);
    ~Api_protocol();

    //protocol command
    bool tryLogin(const QString &login,const QString &pass);
    bool sendProtocol();

    //get the stored data
    Player_private_and_public_informations get_player_informations();
    QString getPseudo();
    quint16 getId();
    quint64 getTXSize();

    virtual void sendDatapackContent() = 0;
    virtual void tryDisconnect() = 0;
    QString get_datapack_base_name();

    //plant
    bool getHavePlantAction();
    bool getHaveShopAction();

    //to reset all
    void resetAll();

    //to manipulate the monsters
    Player_private_and_public_informations player_informations;

    void startReadData();
private:
    //status for the query
    bool is_logged;
    bool have_send_protocol;
    bool have_receive_protocol;
    bool tolerantMode;

    //to send trame
    quint8 lastQueryNumber;
protected:
    //have message without reply
    virtual void parseMessage(const quint8 &mainCodeType,const QByteArray &data);
    virtual void parseMessage(const quint8 &mainCodeType,const quint16 &subCodeType,const QByteArray &data);
    //have query with reply
    virtual void parseQuery(const quint8 &mainCodeType,const quint8 &queryNumber,const QByteArray &data);
    virtual void parseQuery(const quint8 &mainCodeType,const quint16 &subCodeType,const quint8 &queryNumber,const QByteArray &data);
    //send reply
    virtual void parseReplyData(const quint8 &mainCodeType,const quint8 &queryNumber,const QByteArray &data);
    virtual void parseReplyData(const quint8 &mainCodeType,const quint16 &subCodeType,const quint8 &queryNumber,const QByteArray &data);

    //error
    void parseError(const QString &userMessage, const QString &errorString);

    //general data
    virtual void defineMaxPlayers(const quint16 &maxPlayers) = 0;

    //stored local player info
    quint16 max_player;
    quint32 number_of_map;

    //to send trame
    ProtocolParsingOutput *output;
    quint8 queryNumber();

    //datapack
    QString datapack_base_name;

    //plant
    bool havePlantAction;
    bool haveShopAction;

    //teleport list query id
    QList<quint8> teleportList;

    //trade
    QList<quint32> tradeRequestId;
    bool isInTrade;
    //battle
    QList<quint32> battleRequestId;
    bool isInBattle;
signals:
    void newError(const QString &error,const QString &detailedError);

    //protocol/connection info
    void disconnected(const QString &reason);
    void notLogged(const QString &reason);
    void logged();
    void protocol_is_good();

    //general info
    void number_of_player(const quint16 &number,const quint16 &max_player);
    void random_seeds(const QByteArray &data);

    //map move
    void insert_player(const CatchChallenger::Player_public_informations &player,const quint32 &mapId,const quint16 &x,const quint16 &y,const CatchChallenger::Direction &direction);
    void move_player(const quint16 &id,const QList<QPair<quint8,CatchChallenger::Direction> > &movement);
    void remove_player(const quint16 &id);
    void reinsert_player(const quint16 &id,const quint8 &x,const quint8 &y,const CatchChallenger::Direction &direction);
    void reinsert_player(const quint16 &id,const quint32 &mapId,const quint8 &x,const quint8 y,const CatchChallenger::Direction &direction);
    void dropAllPlayerOnTheMap();
    void teleportTo(const quint32 &mapId,const quint16 &x,const quint16 &y,const CatchChallenger::Direction &direction);

    //plant
    void insert_plant(const quint32 &mapId,const quint16 &x,const quint16 &y,const quint8 &plant_id,const quint16 &seconds_to_mature);
    void remove_plant(const quint32 &mapId,const quint16 &x,const quint16 &y);
    void seed_planted(const bool &ok);
    void plant_collected(const CatchChallenger::Plant_collect &stat);
    //crafting
    void recipeUsed(const RecipeUsage &recipeUsage);
    //inventory
    void objectUsed(const ObjectUsage &objectUsage);

    //chat
    void new_chat_text(const CatchChallenger::Chat_type &chat_type,const QString &text,const QString &pseudo,const CatchChallenger::Player_type &type);
    void new_system_text(const CatchChallenger::Chat_type &chat_type,const QString &text);

    //player info
    void have_current_player_info(const CatchChallenger::Player_private_and_public_informations &informations);
    void have_inventory(const QHash<quint32,quint32> &items);
    void add_to_inventory(const QHash<quint32,quint32> &items);
    void remove_to_inventory(const QHash<quint32,quint32> &items);

    //datapack
    void haveTheDatapack();
    void newFile(const QString &fileName,const QByteArray &data,const quint64 &mtime);
    void removeFile(const QString &fileName);

    //shop
    void haveShopList(const QList<ItemToSellOrBuy> &items);
    void haveBuyObject(const BuyStat &stat,const quint32 &newPrice);
    void haveSellObject(const SoldStat &stat,const quint32 &newPrice);

    //trade
    void tradeRequested(const QString &pseudo,const quint8 &skinInt);
    void tradeAcceptedByOther(const QString &pseudo,const quint8 &skinInt);
    void tradeCanceledByOther();
    void tradeFinishedByOther();
    void tradeValidatedByTheServer();
    void tradeAddTradeCash(const quint64 &cash);
    void tradeAddTradeObject(const quint32 &item,const quint32 &quantity);
    void tradeAddTradeMonster(const CatchChallenger::PlayerMonster &monster);

    //battle
    void battleRequested(const QString &pseudo,const quint8 &skinInt);
    void battleAcceptedByOther(const QString &pseudo,const quint8 &skinId,const QList<quint8> &stat,const PublicPlayerMonster &publicPlayerMonster);
    void battleCanceledByOther();
public slots:
    void send_player_direction(const CatchChallenger::Direction &the_direction);
    void send_player_move(const quint8 &moved_unit,const CatchChallenger::Direction &direction);
    void sendChatText(const CatchChallenger::Chat_type &chatType,const QString &text);
    void sendPM(const QString &text,const QString &pseudo);
    void teleportDone();

    //plant, can do action only if the previous is finish
    void useSeed(const quint8 &plant_id);
    void collectMaturePlant();
    //crafting
    void useRecipe(const quint32 &recipeId);
    void addRecipe(const quint32 &recipeId);

    //trade
    void tradeRefused();
    void tradeAccepted();
    void tradeCanceled();
    void tradeFinish();
    void addTradeCash(const quint64 &cash);
    void addObject(const quint32 &item,const quint32 &quantity);
    void addMonster(const quint32 &monsterId);

    //battle
    void battleRefused();
    void battleAccepted();

    //inventory
    void destroyObject(const quint32 &object,const quint32 &quantity=1);
    void useObject(const quint32 &object);

    //shop
    void getShopList(const quint32 &shopId);
    void buyObject(const quint32 &shopId,const quint32 &objectId,const quint32 &quantity,const quint32 &price);
    void sellObject(const quint32 &shopId,const quint32 &objectId,const quint32 &quantity,const quint32 &price);

    //fight
    void tryEscape();
    void useSkill(const quint32 &skill);

    //lean
    void learnSkill(const quint32 &monsterId,const quint32 &skill);

    //quest
    void startQuest(const quint32 &questId);
    void finishQuest(const quint32 &questId);
    void cancelQuest(const quint32 &questId);
    void nextQuestStep(const quint32 &questId);
};
}

#endif // CATCHCHALLENGER_PROTOCOL_H
