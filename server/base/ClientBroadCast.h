#ifndef POKECRAFT_CLIENTBROADCAST_H
#define POKECRAFT_CLIENTBROADCAST_H

#include <QObject>
#include <QDataStream>
#include <QString>
#include <QByteArray>
#include <QList>
#include <QSet>
#include <QMultiHash>

#include "BroadCastWithoutSender.h"
#include "ServerStructures.h"
#include "../../general/base/DebugClass.h"
#include "../../general/base/GeneralStructures.h"
#include "../VariableServer.h"

namespace Pokecraft {
/// \warning here, 0 random should be call
/// \warning here only the call with global scope need be do
class ClientBroadCast : public QObject
{
    Q_OBJECT
public:
    explicit ClientBroadCast();
    ~ClientBroadCast();
    // general info
    Player_internal_informations *player_informations;
    // set the variable
    void setVariable(Player_internal_informations *player_informations);
    //player indexed list
    static QHash<QString,ClientBroadCast *> playerByPseudo;
    static QMultiHash<CLAN_ID_TYPE,ClientBroadCast *> playerByClan;
    static QList<ClientBroadCast *> clientBroadCastList;
public slots:
    //global slot
    void sendPM(const QString &text,const QString &pseudo);
    void receiveChatText(const Chat_type &chatType, const QString &text, const Player_internal_informations *sender_informations);
    void receiveSystemText(const QString &text,const bool &important=false);
    void sendChatText(const Chat_type &chatType,const QString &text);
    void receive_instant_player_number(qint32 connected_players);
    void kick();
    void sendBroadCastCommand(const QString &command,const QString &extraText);
    void setRights(const Player_type& type);
    //after login
    void send_player_informations();
    //normal slots
    void askIfIsReadyToStop();
    void sendSystemMessage(const QString &text,const bool &important=false);
signals:
    //normal signals
    void error(const QString &error);
    void kicked();
    void message(const QString &message);
    void isReadyToStop();
    //send packet on network
    void sendPacket(const quint8 &mainIdent,const quint16 &subIdent,const QByteArray &data=QByteArray());
    void sendPacket(const quint8 &mainIdent,const QByteArray &data=QByteArray());
private:
    //local data
    qint32 connected_players;//it's th last number of connected player send
    static QHash<QString,ClientBroadCast *>::const_iterator i_playerByPseudo;
    static QHash<QString,ClientBroadCast *>::const_iterator i_playerByPseudo_end;
    static ClientBroadCast *item;
};
}

#endif // CLIENTBROADCAST_H
