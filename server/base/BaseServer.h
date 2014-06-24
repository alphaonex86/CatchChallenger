#ifndef CATCHCHALLENGER_BASESERVER_H
#define CATCHCHALLENGER_BASESERVER_H

#include <QObject>
#include <QSettings>
#include <QDebug>
#include <QTimer>
#include <QCoreApplication>
#include <QList>
#include <QByteArray>
#include <QSqlDatabase>
#include <QDir>
#include <QSemaphore>
#include <QString>
#include <QRegularExpression>

#include "../../general/base/DebugClass.h"
#include "../../general/base/Map_loader.h"
#include "../../general/base/ProtocolParsing.h"
#include "../../general/base/QFakeServer.h"
#include "../../general/base/QFakeSocket.h"
#include "ServerStructures.h"
#include "Client.h"
#include "MapServer.h"

namespace CatchChallenger {
class BaseServer
{
public:
    explicit BaseServer();
    ~BaseServer();
    void setSettings(const ServerSettings &settings);
    ServerSettings getSettings() const;
    static void initialize_the_database_prepared_query();

    void load_clan_max_id();
    static void load_clan_max_id_static(void *object);
    void load_clan_max_id_return();

    void load_account_max_id();
    static void load_account_max_id_static(void *object);
    void load_account_max_id_return();

    void load_character_max_id();
    static void load_character_max_id_static(void *object);
    void load_character_max_id_return();
protected:
    //init, constructor, destructor
    void initAll();//call before all
    //remove all finished client
    void load_next_city_capture();
protected:
    void parseJustLoadedMap(const Map_to_send &,const QString &);
    void closeDB();
    //starting function
    void loadAndFixSettings();

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
        CommonMap* map;
        Map_semi_border border;
        Map_to_send old_map;
    };
    void preload_the_data();
    void preload_the_events();
    void preload_the_randomData();
    void preload_the_ddos();
    void preload_zone();
    void preload_industries();
    void preload_market_monsters();
    void preload_market_items();
    void preload_the_city_capture();
    void preload_the_map();
    void preload_the_skin();
    void preload_the_datapack();
    void preload_the_players();
    void preload_the_bytearray();
    virtual void preload_the_visibility_algorithm();
    void preload_the_bots(const QList<Map_semi> &semi_loaded_map);
    virtual void preload_finish();
    void preload_the_plant_on_map();
    static void preload_the_plant_on_map_static(void *object);
    void preload_the_plant_on_map_return();
    void preload_shop();
    void preload_monsters_drops();
    void load_monsters_max_id();
    static void load_monsters_max_id_static(void *object);
    void load_monsters_max_id_return();
    QHash<quint32,MonsterDrops> loadMonsterDrop(const QString &file, QHash<quint32,Item> items,const QHash<quint32,Monster> &monsters);

    static void preload_zone_static(void *object);
    void preload_zone_init();
    void preload_zone_sql();
    void preload_zone_return();
    static void preload_industries_static(void *object);
    void preload_industries_return();
    static void preload_market_items_static(void *object);
    void preload_market_items_return();
    static void preload_market_monsters_static(void *object);
    void preload_market_monsters_return();

    void unload_industries();
    void unload_zone();
    void unload_market();
    void unload_the_city_capture();
    void unload_the_bots();
    void unload_the_data();
    void unload_the_static_data();
    void unload_the_map();
    void unload_the_skin();
    void unload_the_datapack();
    void unload_the_players();
    void unload_the_visibility_algorithm();
    void unload_the_ddos();
    void unload_the_events();
    void unload_the_plant_on_map();
    void unload_shop();
    void unload_monsters_drops();
    void unload_the_randomData();

    void loadMonsterBuffs(const quint32 &index);
    static void loadMonsterBuffs_static(void *object);
    void loadMonsterBuffs_return();

    void loadMonsterSkills(const quint32 &index);
    static void loadMonsterSkills_static(void *object);
    void loadMonsterSkills_return();

    bool initialize_the_database();
    void loadBotFile(const QString &mapfile, const QString &fileName);
    //FakeServer server;//wrong, create another object, here need use the global static object

    //to keep client list, QSet because it will have lot of more disconnecion than server closing
    QSet<Client *> client_list;
    bool dataLoaded;

    QHash<QString/*name*/,QHash<quint8/*bot id*/,CatchChallenger::Bot> > botFiles;
    QSet<quint32> botIdLoaded;
    QTime timeDatapack;

    QFileInfoList entryListZone;
    int entryListIndex;
    int plant_on_the_map;

    static QRegularExpression regexXmlFile;
    static QString text_dotxml;
    static QString text_zone;
    static QString text_capture;
    static QString text_fightId;
    static QString text_dotcomma;
    static QString text_male;
    static QString text_female;
    static QString text_unknown;
    static QString text_slash;
    static QString text_antislash;
    static QString text_type;
    static QString text_shop;
    static QString text_learn;
    static QString text_heal;
    static QString text_market;
    static QString text_zonecapture;
    static QString text_fight;
    static QString text_fightid;
    static QString text_lookAt;
    static QString text_left;
    static QString text_right;
    static QString text_top;
    static QString text_bottom;
    static QString text_bots;
    static QString text_bot;
    static QString text_id;
    static QString text_name;
    static QString text_step;
    static QString text_arrow;
    static QString text_dottmx;
    static QString text_shops;
    static QString text_product;
    static QString text_itemId;
    static QString text_overridePrice;
    static QString text_list;
    static QString text_monster;
    static QString text_monsters;
    static QString text_drops;
    static QString text_drop;
    static QString text_item;
    static QString text_quantity_min;
    static QString text_quantity_max;
    static QString text_luck;
    static QString text_percent;
};
}

#endif
