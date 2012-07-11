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
#include "../general/base/DebugClass.h"

class ClientHeavyLoad : public QObject
{
	Q_OBJECT
public:
	explicit ClientHeavyLoad();
	~ClientHeavyLoad();
	void setVariable(Player_internal_informations *player_informations);
	static QList<quint16> simplifiedIdList;
public slots:
	virtual void askLogin(const quint8 &query_id,const QString &login,const QByteArray &hash);
	void askRandomSeedList(const quint8 &query_id);
	void datapackList(const quint8 &query_id,const QStringList &files,const QList<quint32> &timestamps);
	void dbQuery(QSqlQuery &sqlQuery);
	//normal slots
	void askIfIsReadyToStop();
	void stop();
private:
	// ------------------------------
	bool sendFile(const QString &fileName,const QByteArray &content,const quint32 &mtime);
	QString SQL_text_quote(QString text);
	// ------------------------------
	Player_internal_informations *player_informations;
	bool sendFileIfNeeded(const QString &filePath,const QString &fileName,const quint32 &mtime,const bool &checkMtime=true);
	void listDatapack(const QString &suffix,const QStringList &files);
	bool loadTheRawUTF8String();
signals:
	//normal signals
	void error(const QString &error);
	void message(const QString &message);
	void isReadyToStop();
	//send packet on network
	void sendPacket(const quint8 &mainIdent,const quint16 &subIdent,const QByteArray &data=QByteArray());
	void sendPacket(const quint8 &mainIdent,const QByteArray &data=QByteArray());
	//send reply
	void postReply(const quint8 &queryNumber,const QByteArray &data);
	//login linked signals
	void send_player_informations();
	void isLogged();
	void put_on_the_map(Map_server* map,const COORD_TYPE &x,const COORD_TYPE &y,const Orientation &orientation);
	//random linked signals
	void setRandomSeedList(const QByteArray &randomData);
};

#endif // CLIENTHEAVYLOAD_H
