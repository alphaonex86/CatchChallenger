#ifndef CATCHCHALLENGER_CLIENTNETWORKREAD_H
#define CATCHCHALLENGER_CLIENTNETWORKREAD_H

#include <QObject>
#include <QTcpSocket>
#include <QByteArray>
#include <QStringList>
#include <QRegularExpression>

#include "BroadCastWithoutSender.h"
#include "ServerStructures.h"
#include "../../general/base/DebugClass.h"
#include "../../general/base/ProtocolParsing.h"
#include "../../general/base/GeneralVariable.h"
#include "../VariableServer.h"

namespace CatchChallenger {
class ClientNetworkRead : public ProtocolParsingInput
{
    Q_OBJECT
public:
    explicit ClientNetworkRead(Player_internal_informations *player_informations,ConnectedSocket * socket);
    void stopRead();

    struct TeleportationPoint
    {
        Map *map;
        COORD_TYPE x;
        COORD_TYPE y;
        Orientation orientation;
    };
public slots:
    void fake_receive_data(const QByteArray &data);
    void purgeReadBuffer();
    //normal slots
    void askIfIsReadyToStop();

    void teleportTo(Map *map,const /*COORD_TYPE*/quint8 &x,const /*COORD_TYPE*/quint8 &y,const Orientation &orientation);
    void sendTradeRequest(const QByteArray &data);
    void sendBattleRequest(const QByteArray &data);
private:
    void parseInputBeforeLogin(const quint8 &mainCodeType,const quint16 &subCodeType,const quint8 &queryNumber,const QByteArray & inputData);
    //have message without reply
    void parseMessage(const quint8 &mainCodeType,const QByteArray &data);
    void parseFullMessage(const quint8 &mainCodeType,const quint16 &subCodeType,const QByteArray &data);
    //have query with reply
    void parseQuery(const quint8 &mainCodeType,const quint8 &queryNumber,const QByteArray &data);
    void parseFullQuery(const quint8 &mainCodeType,const quint16 &subCodeType,const quint8 &queryNumber,const QByteArray &data);
    //send reply
    void parseReplyData(const quint8 &mainCodeType,const quint8 &queryNumber,const QByteArray &data);
    void parseFullReplyData(const quint8 &mainCodeType,const quint16 &subCodeType,const quint8 &queryNumber,const QByteArray &data);

    void parseError(const QString &errorString);
    void receiveSystemText(const QString &text);
signals:
    //normal signals
    void sendFullPacket(const quint8 &mainCodeType,const quint16 &subCodeType,const QByteArray &data=QByteArray()) const;
    void sendPacket(const quint8 &mainCodeType,const QByteArray &data=QByteArray()) const;
    void sendQuery(const quint8 &mainIdent,const quint16 &subIdent,const quint8 &queryNumber,const QByteArray &data) const;
    //send reply
    void postReply(const quint8 &queryNumber,const QByteArray &data) const;
    //normal signals
    void isReadyToStop() const;
    //packet parsed (heavy)
    void askLogin(const quint8 &query_id,const QString &login,const QByteArray &hash) const;
    void datapackList(const quint8 &query_id,const QStringList &files,const QList<quint64> &timestamps) const;
    //packet parsed (map management)
    void moveThePlayer(const quint8 &previousMovedUnit,const Direction &direction) const;
    void teleportValidatedTo(Map *map,const /*COORD_TYPE*/quint8 &x,const /*COORD_TYPE*/quint8 &y,const Orientation &orientation) const;
    //trade
    void tradeCanceled() const;
    void tradeAccepted() const;
    void tradeFinished() const;
    void tradeAddTradeCash(const quint64 &cash) const;
    void tradeAddTradeObject(const quint32 &item,const quint32 &quantity) const;
    void tradeAddTradeMonster(const quint32 &monsterId) const;
    //packet parsed (broadcast)
    void sendPM(const QString &text,const QString &pseudo) const;
    void sendChatText(const Chat_type &chatType,const QString &text) const;
    void sendLocalChatText(const QString &text) const;
    void sendBroadCastCommand(const QString &command,const QString &extraText) const;
    void sendHandlerCommand(const QString &command,const QString &extraText) const;
    //plant
    void plantSeed(const quint8 &query_id,const quint8 &plant_id) const;
    void collectPlant(const quint8 &query_id) const;
    //crafting
    void useRecipe(const quint8 &query_id,const quint32 &recipe_id) const;
    //inventory
    void destroyObject(const quint32 &itemId,const quint32 &quantity) const;
    void useObject(const quint8 &query_id,const quint32 &itemId) const;
    void wareHouseStore(const qint64 &cash, const QList<QPair<quint32, qint32> > &items, const QList<quint32> &withdrawMonsters, const QList<quint32> &depositeMonsters) const;
    //shop
    void getShopList(const quint32 &query_id,const quint32 &shopId) const;
    void buyObject(const quint32 &query_id,const quint32 &shopId,const quint32 &objectId,const quint32 &quantity,const quint32 &price) const;
    void sellObject(const quint32 &query_id,const quint32 &shopId,const quint32 &objectId,const quint32 &quantity,const quint32 &price) const;
    //fight
    void tryEscape() const;
    void useSkill(const quint32 &skill) const;
    void learnSkill(const quint32 &monsterId,const quint32 &skill) const;
    void heal() const;
    void requestFight(const quint32 &fightId) const;
    //quest
    void newQuestAction(const QuestAction &action,const quint32 &questId) const;
    //battle
    void battleCanceled() const;
    void battleAccepted() const;
    //clan
    void clanAction(const quint8 &query_id,const quint8 &action,const QString &text) const;
    void clanInvite(const bool &accept) const;
private:
    // for status
    bool have_send_protocol;
    bool is_logging_in_progess;
    bool stopIt;
    // function
    ConnectedSocket * socket;
    Player_internal_informations *player_informations;
    //to prevent memory presure
    quint8 previousMovedUnit;
    quint8 direction;
    QList<TeleportationPoint> lastTeleportation;
    //to parse the netwrok stream
    quint8 mainCodeType;
    quint16 subCodeType;
    quint8 queryNumber;
    QList<quint8> queryNumberList;

    static QRegularExpression commandRegExp;
    static QRegularExpression commandRegExpWithArgs;
};
}

#endif // CLIENTNETWORKREAD_H
