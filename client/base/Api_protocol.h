#ifndef POKECRAFT_PROTOCOL_H
#define POKECRAFT_PROTOCOL_H

#include <QObject>
#include <QString>
#include <QCoreApplication>
#include <QString>
#include <QByteArray>
#include <QList>
#include <QPair>
#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QDateTime>
#include <QCryptographicHash>

#include "../../general/base/DebugClass.h"
#include "../../general/base/GeneralStructures.h"
#include "../../general/base/GeneralVariable.h"
#include "../../general/base/ProtocolParsing.h"
#include "../../general/base/MoveOnTheMap.h"
#include "ClientStructures.h"

namespace Pokecraft {
class Api_protocol : public ProtocolParsingInput, public MoveOnTheMap
{
    Q_OBJECT
public:
    explicit Api_protocol(ConnectedSocket *socket);
    ~Api_protocol();

    //protocol command
    bool tryLogin(const QString &login,const QString &pass);
    bool sendProtocol();

    //get the stored data
    Player_private_and_public_informations get_player_informations();
    QString getPseudo();
    quint16 getId();

    virtual void sendDatapackContent() = 0;
    virtual void tryDisconnect() = 0;
    QString get_datapack_base_name();

    //to reset all
    void resetAll();
private:
    //status for the query
    bool is_logged;
    bool have_send_protocol;

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

    //stored local player info
    quint16 max_player;
    Player_private_and_public_informations player_informations;
    QString pseudo;

    //to send trame
    ProtocolParsingOutput *output;
    quint8 queryNumber();

    //datapack
    QString datapack_base_name;
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
    void insert_player(const Pokecraft::Player_public_informations &player,const QString &mapName,const quint16 &x,const quint16 &y,const Pokecraft::Direction &direction);
    void move_player(const quint16 &id,const QList<QPair<quint8,Direction> > &movement);
    void remove_player(const quint16 &id);
    void reinsert_player(const quint16 &id,const quint8 &x,const quint8 &y,const Pokecraft::Direction &direction);
    void reinsert_player(const quint16 &id,const QString &mapName,const quint8 &x,const quint8 y,const Pokecraft::Direction &direction);
    void dropAllPlayerOnTheMap();

    //chat
    void new_chat_text(const Pokecraft::Chat_type &chat_type,const QString &text,const QString &pseudo,const Pokecraft::Player_type &type);
    void new_system_text(const Pokecraft::Chat_type &chat_type,const QString &text);

    //player info
    void have_current_player_info(const Pokecraft::Player_private_and_public_informations &informations);
    void have_inventory(const QHash<quint32,quint32> &items);

    //datapack
    void haveTheDatapack();
    void newFile(const QString &fileName,const QByteArray &data,const quint32 &mtime);
    void removeFile(const QString &fileName);
public slots:
    void send_player_direction(const Pokecraft::Direction &the_direction);
    void send_player_move(const quint8 &moved_unit,const Pokecraft::Direction &direction);
    void sendChatText(const Pokecraft::Chat_type &chatType,const QString &text);
    void sendPM(const QString &text,const QString &pseudo);
};
}

#endif // POKECRAFT_PROTOCOL_H
