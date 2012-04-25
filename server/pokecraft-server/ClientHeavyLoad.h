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
	void setVariable(GeneralData *generalData,Player_private_and_public_informations *player_informations);
public slots:
	virtual void askLogin(const quint8 &query_id,const QString &login,const QByteArray &hash);
	void askRandomSeedList(const quint8 &query_id);
	void datapackList(const quint8 &query_id,const QStringList &files,const QList<quint32> &timestamps);
	void updatePlayerPosition(const QString &map,const quint16 &x,const quint16 &y,const Orientation &orientation);
	void fakeLogin(const quint32 &last_fake_player_id,const quint16 &x,const quint16 &y,Map_final *map,const Orientation &orientation,const QString &skin);
	//normal slots
	void askIfIsReadyToStop();
	void stop();
private:
	bool fake_mode;
	GeneralData *generalData;
	QString datapack_base_name;
	QRegExp right_file_name;
	// ------------------------------
	void sendFile(const QString &fileName,const QByteArray &content,const quint32 &mtime);
	QString SQL_text_quote(QString text);
	// ------------------------------
	Player_private_and_public_informations *player_informations;
	QString basePath;
	bool sendFileIfNeeded(const QString &filePath,const QString &fileName,const quint32 &mtime,const bool &checkMtime=true);
	void listDatapack(const QString &suffix,const QStringList &files);
signals:
	//normal signals
	void error(const QString &error);
	void message(const QString &message);
	void sendPacket(const QByteArray &data);
	void isReadyToStop();
	//login linked signals
	void send_player_informations();
	void isLogged();
	void put_on_the_map(const quint32 &player_id,Map_final* map,const quint16 &x,const quint16 &y,const Orientation &orientation,const quint16 &speed);
	//random linked signals
	void setRandomSeedList(const QByteArray &randomData);
protected:
	bool is_logged;
};

#endif // CLIENTHEAVYLOAD_H
