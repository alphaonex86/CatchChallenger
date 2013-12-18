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
#include "../../general/base/Map_loader.h"
#include "../../general/base/ProtocolParsing.h"
#include "../../general/base/QFakeServer.h"
#include "../../general/base/QFakeSocket.h"
#include "ServerStructures.h"
#include "Client.h"
#include "Bot/FakeBot.h"
#include "MapServer.h"
#include "../crafting/BaseServerCrafting.h"
#include "../fight/BaseServerFight.h"

#ifndef CATCHCHALLENGER_BASESERVER_H
#define CATCHCHALLENGER_BASESERVER_H

namespace CatchChallenger {
class BaseServer : public QObject, public BaseServerCrafting, public BaseServerFight
{
    Q_OBJECT
public:
    explicit BaseServer();
    virtual ~BaseServer();
    void setSettings(ServerSettings settings);
    ServerSettings getSettings();
    //stat function
    virtual bool isListen();
    virtual bool isStopped();
    virtual void stop();
    void load_clan_max_id();
    void load_account_max_id();
    void load_character_max_id();
    void start();
protected slots:
    virtual void start_internal_server();
    virtual void stop_internal_server();
    //init, constructor, destructor
    virtual void initAll();//call before all
    //remove all finished client
    virtual void removeOneClient();
    //new connection
    virtual void newConnection();
    virtual void load_next_city_capture();
    //bitcoin
    void bitcoinProcessReadyReadStandardError();
    void bitcoinProcessReadyReadStandardOutput();
    void bitcoinProcessError(QProcess::ProcessError error);
    void bitcoinProcessStateChanged(QProcess::ProcessState newState);
signals:
    void error(const QString &error) const;
    void try_initAll() const;
    void try_stop_server() const;
    void need_be_started() const;
    //stat
    void is_started(const bool &) const;
protected:
    virtual void parseJustLoadedMap(const Map_to_send &,const QString &);
    virtual void connect_the_last_client(Client * client);
    void closeDB();
    //starting function
    virtual bool check_if_now_stopped();//return true if can be stopped
    virtual void loadAndFixSettings();
    //player related
    virtual ClientMapManagement * getClientMapManagement();

    //stat
    enum ServerStat
    {
        Down=0,
        InUp=1,
        Up=2,
        InDown=3
    };
    ServerStat stat;

    //to load/unload the content
    struct Map_semi
    {
        //conversion x,y to position: x+y*width
        Map* map;
        Map_semi_border border;
        Map_to_send old_map;
    };
    virtual void preload_the_data();
    virtual void preload_zone();
    virtual void preload_industries();
    virtual void preload_market_monsters();
    virtual void preload_market_items();
    virtual void preload_the_city_capture();
    virtual void preload_the_map();
    virtual void preload_the_skin();
    virtual void preload_the_datapack();
    virtual void preload_the_players();
    virtual void preload_the_visibility_algorithm();
    virtual void preload_the_bots(const QList<Map_semi> &semi_loaded_map);
    virtual void unload_industries();
    virtual void unload_zone();
    virtual void unload_market();
    virtual void unload_the_city_capture();
    virtual void unload_the_bots();
    virtual void unload_the_data();
    virtual void unload_the_static_data();
    virtual void unload_the_map();
    virtual void unload_the_skin();
    virtual void unload_the_datapack();
    virtual void unload_the_players();
    virtual void unload_the_visibility_algorithm();

    virtual QList<PlayerBuff> loadMonsterBuffs(const quint32 &monsterId);
    virtual QList<PlayerMonster::PlayerSkill> loadMonsterSkills(const quint32 &monsterId);

    virtual bool initialize_the_database();
    virtual void loadBotFile(const QString &fileName);
    //FakeServer server;//wrong, create another object, here need use the global static object

    //to keep client list, QSet because it will have lot of more disconnecion than server closing
    QSet<Client *> client_list;
    bool dataLoaded;

    QHash<QString/*name*/,QHash<quint8/*bot id*/,CatchChallenger::Bot> > botFiles;
    QSet<quint32> botIdLoaded;
};
}

#endif
