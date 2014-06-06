#ifndef CATCHCHALLENGER_CLIENTBROADCAST_H
#define CATCHCHALLENGER_CLIENTBROADCAST_H

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

namespace CatchChallenger {
/// \warning here, 0 random should be call
/// \warning here only the call with global scope need be do
#ifdef EPOLLCATCHCHALLENGERSERVER
class Client;
#endif
class ClientBroadCast
        #ifndef EPOLLCATCHCHALLENGERSERVER
        : public QObject
        #endif
{
    #ifndef EPOLLCATCHCHALLENGERSERVER
    Q_OBJECT
    #endif
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

    static QString text_chat;
    static QString text_space;
    static QString text_system;
    static QString text_system_important;
    static QString text_setrights;
    static QString text_normal;
    static QString text_premium;
    static QString text_gm;
    static QString text_dev;
    static QString text_playerlist;
    static QString text_startbold;
    static QString text_stopbold;
    static QString text_playernumber;
    static QString text_kick;
    static QString text_Youarealoneontheserver;
    static QString text_playersconnected;
    static QString text_playersconnectedspace;
    static QString text_havebeenkickedby;
    static QString text_unknowcommand;
    static QString text_commandnotunderstand;
    static QString text_command;
    static QString text_commaspace;
    static QString text_unabletofoundtheconnectedplayertokick;
    static QString text_unabletofoundthisrightslevel;

    static QList<int> generalChatDrop;
    static int generalChatDropTotalCache;
    static int generalChatDropNewValue;
    static QList<int> clanChatDrop;
    static int clanChatDropTotalCache;
    static int clanChatDropNewValue;
    static QList<int> privateChatDrop;
    static int privateChatDropTotalCache;
    static int privateChatDropNewValue;
#ifdef EPOLLCATCHCHALLENGERSERVER
public:
    Client *client;
#endif
public:
    //global slot
    void sendPM(const QString &text,const QString &pseudo);
    void receiveChatText(const Chat_type &chatType, const QString &text, const Player_internal_informations *sender_informations);
    void receiveSystemText(const QString &text,const bool &important=false);
    void sendChatText(const Chat_type &chatType,const QString &text);
    void receive_instant_player_number(const quint16 &connected_players, const QByteArray &outputData);
    void kick();
    void sendBroadCastCommand(const QString &command,const QString &extraText);
    void setRights(const Player_type& type);
    //after login
    void send_player_informations();
    //normal slots
    void askIfIsReadyToStop();
    void sendSystemMessage(const QString &text,const bool &important=false);
    //clan
    void clanChange(const quint32 &clanId);
#ifndef EPOLLCATCHCHALLENGERSERVER
signals:
#else
protected:
#endif
#ifndef EPOLLCATCHCHALLENGERSERVER
    void isReadyToStop() const;
#endif
    //normal signals
    void error(const QString &error) const;
    void kicked() const;
    void message(const QString &message) const;
    //send packet on network
    void sendFullPacket(const quint8 &mainIdent,const quint16 &subIdent,const QByteArray &data=QByteArray()) const;
    void sendPacket(const quint8 &mainIdent,const QByteArray &data=QByteArray()) const;
    bool sendRawSmallPacket(const QByteArray &data) const;
private:
    //local data
    qint32 connected_players;//it's th last number of connected player send
    static ClientBroadCast *item;
    CLAN_ID_TYPE clan;
};
}

#endif // CLIENTBROADCAST_H
