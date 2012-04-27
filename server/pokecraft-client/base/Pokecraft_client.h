#ifndef POKECRAFT_CLIENT_H
#define POKECRAFT_CLIENT_H

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

#include "../pokecraft-general/DebugClass.h"
#include "../pokecraft-general/GeneralStructures.h"
#include "ClientStructures.h"

class Pokecraft_client : public QObject
{
	Q_OBJECT
public:
	explicit Pokecraft_client();
	~Pokecraft_client();
	void tryConnect(QString host,quint16 port);
	void tryLogin(QString login,QString pass);
	void tryDisconnect();
	QString errorString();
	void sendProtocol();
	QList<Player_public_informations> get_player_informations_list();
	Player_private_and_public_informations get_player_informations();
	void not_needed_player_informations(quint32 id);
	void add_player_watching(quint32 id);
	void remove_player_watching(quint32 id);
	void add_player_watching(QList<quint32> ids);
	void remove_player_watching(QList<quint32> ids);
	int indexOfPlayerInformations(quint32 id);
	QString getPseudo();
	QString getHost();
	quint16 getPort();
	void sendDatapackContent();
	const QStringList listDatapack(QString suffix);
	void cleanDatapack(QString suffix);
	quint32 getId();
	QString get_datapack_base_name();
private:
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
	QString host;
	quint16 port;
	QTcpSocket socket;
	QString error_string;
	quint8 queryNumber();
	quint8 lastQueryNumber;
	void sendData(QByteArray data);
	bool haveData;
	quint32 dataSize;
	QByteArray data;
	void dataClear();
	bool checkStringIntegrity(QByteArray data);
	void parseInput(QByteArray inputData);
	bool is_logged;
	bool have_send_protocol;
	QList<query_send> query_list;
	void add_to_query_list(quint8 id,query_type type);
	QString skin_name;
	query_type remove_to_query_list(quint8 id);
	quint32 player_id;
	Player_private_and_public_informations player_informations;
	QList<Player_public_informations> player_informations_list;
	QList<quint32> player_id_watching;
	void resetAll();
	//file list
	struct query_files
	{
		quint8 id;
		QStringList filesName;
	};
	QList<query_files> query_files_list;
	QString datapack_base_name;
	bool wait_datapack_content;
	QStringList datapackFilesList;
	quint8 last_mainIdent;
	quint16 last_subIdent;
	QByteArray last_query;
	//for the inputData parsing
	quint8 mainIdent;
	quint8 queryNumberReturned;
	quint16 subIdent;
	quint8 returnCode;
signals:
	void stateChanged(QAbstractSocket::SocketState socketState);
	void error(QAbstractSocket::SocketError socketError);
	void haveNewError();
	void disconnected(QString reason);
	void notLogged(QString reason);
	void logged();
	void protocol_is_good();
	void number_of_player(quint16 number,quint16 max);
	void have_current_player_info();
	void insert_player(quint32 id,QString mapName,quint16 x,quint16 y,quint8 direction,quint16 speed);
	void move_player(quint32 id,QList<QPair<quint8,quint8> > movement);
	void remove_player(quint32 id);
	void new_chat_text(quint32 player_id,quint8 chat_type,QString text);
	void new_player_info();
	void new_player_info(QList<Player_public_informations> info);
	void new_player_info(Player_private_and_public_informations info);
	void haveTheDatapack();
private slots:
	void readyRead();
	void disconnected();
public slots:
	void send_player_move(quint8 moved_unit,quint8 direction);
	void sendChatText(quint8 chatType,QString text);
	void sendPM(QString text,QString pseudo);
	void errorOutput(QString error,QString detailedError="");
};

#endif // POKECRAFT_CLIENT_H
