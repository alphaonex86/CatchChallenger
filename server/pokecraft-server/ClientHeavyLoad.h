#ifndef CLIENTHEAVYLOAD_H
#define CLIENTHEAVYLOAD_H

#include <QObject>
#include <QSqlDatabase>
#include <QStringList>
#include <QString>
#include <QByteArray>
#include <QList>
#include <QCoreApplication>
#include <QFile>
#include <QCryptographicHash>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QFileInfo>
#include <QDateTime>
#include <QDir>

#include "ServerStructures.h"
#include "../VariableServer.h"
#include "../pokecraft-general/DebugClass.h"

class ClientHeavyLoad : public QObject
{
	Q_OBJECT
public:
	explicit ClientHeavyLoad();
	~ClientHeavyLoad();
	void setVariable(
		QSqlDatabase *db,
		QStringList *cached_files_name,
		QList<QByteArray> *cached_files_data,
		QList<quint32> *cached_files_mtime,
		qint64 *cache_max_file_size,
		qint64 *cache_max_size,
		qint64 *cache_size,
		QList<quint32> *connected_players_id_list
		);
public slots:
	virtual void askLogin(quint8 query_id,QString login,QByteArray hash);
	void askRandomSeedList(quint8 query_id);
	void datapackList(quint8 query_id,QStringList files,QList<quint32> timestamps);
	void updatePlayerPosition(QString map,quint16 x,quint16 y,Orientation orientation);
	void fakeLogin(quint32 last_fake_player_id,quint16 x,quint16 y,QString map,Orientation orientation,QString skin);
	//normal slots
	void askIfIsReadyToStop();
private:
	bool fake_mode;
	QSqlDatabase *db;
	QStringList *cached_files_name;
	QList<QByteArray> *cached_files_data;
	QList<quint32> *cached_files_mtime;
	qint64 *cache_max_file_size;
	qint64 *cache_max_size;
	qint64 *cache_size;
	QString datapack_base_name;
	QRegExp right_file_name;
	// ------------------------------
	void sendFile(const QString &fileName,const QByteArray &content,const quint32 &mtime);
	QString SQL_text_quote(QString text);
	// ------------------------------
	QList<quint32> *connected_players_id_list;
	Player_private_and_public_informations player_informations;
	QString basePath;
	bool sendFileIfNeeded(const QString &fileName,const quint32 &mtime,const bool &checkMtime=true);
	void listDatapack(QString suffix,const QStringList &files);
signals:
	//normal signals
	void error(const QString &error);
	void message(const QString &message);
	void sendPacket(const QByteArray &data);
	void isReadyToStop();
	//login linked signals
	void send_player_informations(const Player_private_and_public_informations &player_informations);
	void isLogged();
	void put_on_the_map(const quint32 &player_id,const QString &map,const quint16 &x,const quint16 &y,const Orientation &orientation,const quint16 &speed);
	//random linked signals
	void setRandomSeedList(const QByteArray &randomData);
protected:
	bool is_logged;
};

#endif // CLIENTHEAVYLOAD_H
