#include <QObject>
#include <QSettings>
#include <QDebug>
#include <QTimer>
#include <QCoreApplication>
#include <QList>
#include <QByteArray>
#include <QSqlDatabase>
#include <QSqlError>
#include <QDir>
#include <QSemaphore>
#include <QString>

#include "../../general/base/DebugClass.h"
#include "../../server/base/ServerStructures.h"
#include "../../server/base/Client.h"
#include "../../general/base/Map_loader.h"
#include "../../server/base/Bot/FakeBot.h"
#include "../../general/base/ProtocolParsing.h"
#include "../../general/base/QFakeServer.h"
#include "../../general/base/QFakeSocket.h"
#include "../../server/base/Map_server.h"

#ifndef POKECRAFT_INTERNALSERVER_H
#define POKECRAFT_INTERNALSERVER_H

namespace Pokecraft {
class InternalServer : public QObject
{
	Q_OBJECT
public:
    explicit InternalServer(const QString &dbPath);
    ~InternalServer();
	//stat function
	bool isListen();
    bool isStopped();
private:
	//to load/unload the content
	struct Map_semi
	{
		//conversion x,y to position: x+y*width
		Map* map;
		Map_semi_border border;
		Map_to_send old_map;
    };
	void preload_the_data();
    void preload_the_map();
	void unload_the_data();
    void unload_the_map();

	/// \brief To lunch event only when the event loop is setup
	QTimer *lunchInitFunction;

	//stat
	enum ServerStat
	{
		Down=0,
		InUp=1,
		Up=2,
		InDown=3
	};
	ServerStat stat;

    bool initialize_the_database();
    QFakeServer server;

    Client *theSinglePlayer;
    QString dbPath;
private slots:
	//new connection
	void newConnection();
	//remove all finished client
	void removeOneClient();
	//init, constructor, destructor
	void initAll();//call before all
	//starting function
	void stop_internal_server();
	void check_if_now_stopped();
	void start_internal_server();
signals:
	void error(const QString &error);
    void try_initAll();
    void try_stop_server();
    void need_be_started();
    void isReady();
};
}

#endif
