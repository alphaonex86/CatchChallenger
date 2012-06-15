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

class Pokecraft_protocol : public ProtocolParsingInput
{
	Q_OBJECT
public:
	explicit Pokecraft_protocol(QAbstractSocket *socket);
	~Pokecraft_protocol();
	//socket action
	void tryConnect();
	void tryDisconnect();
	//protocol command
	void tryLogin(QString login,QString pass);
	void sendProtocol();
	void not_needed_player_informations(quint32 id);
	void add_player_watching(quint32 id);
	void remove_player_watching(quint32 id);
	void add_player_watching(QList<quint32> ids);
	void remove_player_watching(QList<quint32> ids);
	void sendDatapackContent();
	//post remove to drop not existing file/folder
	void cleanDatapack(QString suffix);
private:
	const QStringList listDatapack(QString suffix);
	enum query_type
	{
		query_type_other,
		query_type_protocol,
		query_type_login,
		query_type_datapack
	};
	struct query_send
	{
		quint8 id;
		query_type type;
	};
	ProtocolParsingOutput *output;
	QAbstractSocket *socket;
	QString error_string;
	quint8 queryNumber();
	quint8 lastQueryNumber;
	void sendData(QByteArray data);

	//status for the query
	bool is_logged;
	bool have_send_protocol;

	//query with reply
	QList<query_send> query_list;
	void add_to_query_list(quint8 id,query_type type);
	query_type remove_to_query_list(quint8 id);

	QList<quint32> player_id_watching;
	void resetAll();

	//for datapack
	//file list
	struct query_files
	{
		quint8 id;
		QStringList filesName;
	};
	QList<query_files> query_files_list;
	bool wait_datapack_content;
	QStringList datapackFilesList;

signals:
	void haveNewError(QString error);

	//protocol/connection info
	void disconnected(QString reason);
	void notLogged(QString reason);
	void logged();
	void protocol_is_good();

	//general info
	void number_of_player(quint16 number,quint16 max);
	void have_current_player_info();

	//map move
	void insert_player(quint32 id,QString mapName,quint16 x,quint16 y,quint8 direction,quint16 speed);
	void move_player(quint32 id,QList<QPair<quint8,quint8> > movement);
	void remove_player(quint32 id);

	//chat
	void new_chat_text(quint32 player_id,quint8 chat_type,QString text);

	//player info
	void new_player_info();
	void new_player_info(QList<Player_public_informations> info);
	void new_player_info(Player_private_and_public_informations info);

	//datapack
	void haveTheDatapack();
public slots:
	void send_player_move(quint8 moved_unit,quint8 direction);
	void sendChatText(quint8 chatType,QString text);
	void sendPM(QString text,QString pseudo);
};

#endif // POKECRAFT_PROTOCOL_H
