#ifndef CATCHCHALLENGER_CLIENTNETWORKREAD_H
#define CATCHCHALLENGER_CLIENTNETWORKREAD_H

#include <QObject>
#include <QTcpSocket>
#include <QByteArray>
#include <QStringList>
#include <QRegExp>

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
private:
    void parseInputBeforeLogin(const quint8 &mainCodeType,const quint16 &subCodeType,const quint8 &queryNumber,const QByteArray & inputData);
    //have message without reply
    void parseMessage(const quint8 &mainCodeType,const QByteArray &data);
    void parseMessage(const quint8 &mainCodeType,const quint16 &subCodeType,const QByteArray &data);
    //have query with reply
    void parseQuery(const quint8 &mainCodeType,const quint8 &queryNumber,const QByteArray &data);
    void parseQuery(const quint8 &mainCodeType,const quint16 &subCodeType,const quint8 &queryNumber,const QByteArray &data);
    //send reply
    void parseReplyData(const quint8 &mainCodeType,const quint8 &queryNumber,const QByteArray &data);
    void parseReplyData(const quint8 &mainCodeType,const quint16 &subCodeType,const quint8 &queryNumber,const QByteArray &data);

    void parseError(const QString &errorString);
signals:
    //normal signals
    void sendPacket(const quint8 &mainCodeType,const quint16 &subCodeType,const QByteArray &data=QByteArray());
    void sendPacket(const quint8 &mainCodeType,const QByteArray &data=QByteArray());
    void sendQuery(const quint8 &mainIdent,const quint16 &subIdent,const quint8 &queryNumber,const QByteArray &data);
    //send reply
    void postReply(const quint8 &queryNumber,const QByteArray &data);
    //normal signals
    void isReadyToStop();
    //packet parsed (heavy)
    void askLogin(const quint8 &query_id,const QString &login,const QByteArray &hash);
    void datapackList(const quint8 &query_id,const QStringList &files,const QList<quint64> &timestamps);
    //packet parsed (map management)
    void moveThePlayer(const quint8 &previousMovedUnit,const Direction &direction);
    void teleportValidatedTo(Map *map,const /*COORD_TYPE*/quint8 &x,const /*COORD_TYPE*/quint8 &y,const Orientation &orientation);
    //trade
    void tradeCanceled();
    void tradeAccepted();
    void tradeFinished();
    void tradeAddTradeCash(const quint64 &cash);
    void tradeAddTradeObject(const quint32 &item,const quint32 &quantity);
    void tradeAddTradeMonster(const quint32 &monsterId);
    //packet parsed (broadcast)
    void sendPM(const QString &text,const QString &pseudo);
    void sendChatText(const Chat_type &chatType,const QString &text);
    void sendLocalChatText(const QString &text);
    void sendBroadCastCommand(const QString &command,const QString &extraText);
    void sendHandlerCommand(const QString &command,const QString &extraText);
    //plant
    void plantSeed(const quint8 &query_id,const quint8 &plant_id);
    void collectPlant(const quint8 &query_id);
    //crafting
    void useRecipe(const quint8 &query_id,const quint32 &recipe_id);
    //inventory
    void destroyObject(const quint32 &itemId,const quint32 &quantity);
    void useObject(const quint8 &query_id,const quint32 &itemId);
    //shop
    void getShopList(const quint32 &query_id,const quint32 &shopId);
    void buyObject(const quint32 &query_id,const quint32 &shopId,const quint32 &objectId,const quint32 &quantity,const quint32 &price);
    void sellObject(const quint32 &query_id,const quint32 &shopId,const quint32 &objectId,const quint32 &quantity,const quint32 &price);
    //fight
    void tryEscape();
    void useSkill(const quint32 &skill);
    void learnSkill(const quint32 &monsterId,const quint32 &skill);
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

    static QRegExp commandRegExp;
    static QRegExp commandRegExpWithArgs;
};
}

#endif // CLIENTNETWORKREAD_H
