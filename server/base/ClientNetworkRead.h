#ifndef POKECRAFT_CLIENTNETWORKREAD_H
#define POKECRAFT_CLIENTNETWORKREAD_H

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

namespace Pokecraft {
class ClientNetworkRead : public ProtocolParsingInput
{
    Q_OBJECT
public:
    explicit ClientNetworkRead(Player_internal_informations *player_informations,ConnectedSocket * socket);
    void stopRead();
    void fake_send_protocol();
public slots:
    void fake_receive_data(const QByteArray &data);
    //normal slots
    void askIfIsReadyToStop();
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
signals:
    //normal signals
    void sendPacket(const quint8 &mainCodeType,const quint16 &subCodeType,const QByteArray &data=QByteArray());
    void sendPacket(const quint8 &mainCodeType,const QByteArray &data=QByteArray());
    //send reply
    void postReply(const quint8 &queryNumber,const QByteArray &data);
    //normal signals
    void isReadyToStop();
    //packet parsed (heavy)
    void askLogin(const quint8 &query_id,const QString &login,const QByteArray &hash);
    void datapackList(const quint8 &query_id,const QStringList &files,const QList<quint64> &timestamps);
    //packet parsed (map management)
    void moveThePlayer(const quint8 &previousMovedUnit,const Direction &direction);
    //packet parsed (broadcast)
    void sendPM(const QString &text,const QString &pseudo);
    void sendChatText(const Chat_type &chatType,const QString &text);
    void sendBroadCastCommand(const QString &command,const QString &extraText);
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
    //to parse the netwrok stream
    quint8 mainCodeType;
    quint16 subCodeType;
    quint8 queryNumber;

    static QRegExp commandRegExp;
    static QRegExp commandRegExpWithArgs;
};
}

#endif // CLIENTNETWORKREAD_H
