#ifndef POKECRAFT_PROTOCOL_H
#define POKECRAFT_PROTOCOL_H

#include <QObject>
#include <QString>
#include <QCoreApplication>
#include <QTcpSocket>
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
	explicit Api_protocol(QAbstractSocket *socket);
	~Api_protocol();

	//protocol command
	bool tryLogin(QString login,QString pass);
	bool sendProtocol();

	//get the stored data
	Player_private_and_public_informations get_player_informations();
	QString getPseudo();
	quint16 getId();
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

	//to reset all
	void resetAll();
signals:
	void newError(QString error,QString detailedError);

	//protocol/connection info
	void disconnected(QString reason);
	void notLogged(QString reason);
	void logged();
	void protocol_is_good();

	//general info
	void number_of_player(quint16 number,quint16 max_player);
	void random_seeds(QByteArray data);

	//map move
	void insert_player(Pokecraft::Player_public_informations player,QString mapName,quint16 x,quint16 y,Pokecraft::Direction direction);
	void move_player(quint16 id,QList<QPair<quint8,Direction> > movement);
	void remove_player(quint16 id);
	void reinsert_player(quint16 id,quint8 x,quint8 y,Pokecraft::Direction direction);
	void reinsert_player(quint16 id,QString mapName,quint8 x,quint8 y,Pokecraft::Direction direction);
	void dropAllPlayerOnTheMap();

	//chat
	void new_chat_text(Pokecraft::Chat_type chat_type,QString text,QString pseudo,Pokecraft::Player_type type);
	void new_system_text(Pokecraft::Chat_type chat_type,QString text);

	//player info
	void have_current_player_info(Pokecraft::Player_private_and_public_informations info,QString pseudo);

	//datapack
	void haveTheDatapack();
	void newFile(const QString &fileName,const QByteArray &data,const quint32 &mtime);
	void removeFile(QString fileName);
public slots:
	void send_player_direction(const Pokecraft::Direction &the_direction);
	void send_player_move(const quint8 &moved_unit,const Pokecraft::Direction &direction);
	void sendChatText(Pokecraft::Chat_type chatType,QString text);
	void sendPM(QString text,QString pseudo);
};
}

#endif // POKECRAFT_PROTOCOL_H
