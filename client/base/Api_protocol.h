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
#include "ClientStructures.h"

class Api_protocol : public ProtocolParsingInput
{
	Q_OBJECT
public:
	explicit Api_protocol(QAbstractSocket *socket);
	~Api_protocol();

	//protocol command
	bool tryLogin(QString login,QString pass);
	bool sendProtocol();
	bool not_needed_player_informations(quint32 id);
	bool add_player_watching(quint32 id);
	bool remove_player_watching(quint32 id);
	bool add_player_watching(QList<quint32> ids);
	bool remove_player_watching(QList<quint32> ids);

	//get the stored data
	QList<Player_public_informations> get_player_informations_list();
	Player_private_and_public_informations get_player_informations();
	QString getPseudo();
	quint32 getId();
private:
	//status for the query
	bool is_logged;
	bool have_send_protocol;

	QList<Player_public_informations> player_informations_list;
	QList<quint32> player_id_watching;

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
	quint32 player_id;
	Player_private_and_public_informations player_informations;

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
	void number_of_player(quint16 number,quint16 max);

	//map move
	void insert_player(quint32 id,QString mapName,quint16 x,quint16 y,quint8 direction,quint16 speed);
	void move_player(quint32 id,QList<QPair<quint8,quint8> > movement);
	void remove_player(quint32 id);

	//chat
	void new_chat_text(quint32 player_id,quint8 chat_type,QString text);

	//player info
	void new_player_info(QList<Player_public_informations> info);
	void have_current_player_info(Player_private_and_public_informations info);

	//datapack
	void haveTheDatapack();
	void haveNewFile(QString fileName,QByteArray data,quint32 mtime);
	void removeFile(QString fileName);
public slots:
	void send_player_move(quint8 moved_unit,quint8 direction);
	void sendChatText(Chat_type chatType,QString text);
	void sendPM(QString text,QString pseudo);
};

#endif // POKECRAFT_PROTOCOL_H
