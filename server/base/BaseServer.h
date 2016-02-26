#ifndef CATCHCHALLENGER_BASESERVER_H
#define CATCHCHALLENGER_BASESERVER_H

#include <vector>
#include <string>
#include <regex>

#ifndef EPOLLCATCHCHALLENGERSERVER
#include <QSqlDatabase>
#include <QObject>
#include <QTimer>
#include <QCoreApplication>
#include <QDir>
#include <QSemaphore>
#include "../../general/base/QFakeServer.h"
#include "../../general/base/QFakeSocket.h"
#endif

#include "../../general/base/Map_loader.h"
#include "../../general/base/ProtocolParsing.h"
#include "../../general/base/FacilityLibGeneral.h"
#include "ServerStructures.h"
#include "Client.h"
#include "MapServer.h"
#include "BaseServerMasterLoadDictionary.h"
#include "BaseServerMasterSendDatapack.h"
#include "BaseServerLogin.h"

namespace CatchChallenger {
class BaseServer : public BaseServerMasterLoadDictionary, public BaseServerLogin
{
public:
    explicit BaseServer();
    virtual ~BaseServer();
    void setSettings(const GameServerSettings &settings);
    GameServerSettings getSettings() const;
    static void initialize_the_database_prepared_query();

    #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    void load_clan_max_id();
    static void load_clan_max_id_static(void *object);
    void load_clan_max_id_return();

    void load_account_max_id();
    static void load_account_max_id_static(void *object);
    void load_account_max_id_return();

    void load_character_max_id();
    static void load_character_max_id_static(void *object);
    void load_character_max_id_return();
    #endif
protected:
    //init, constructor, destructor
    void initAll();//call before all
    //remove all finished client
    bool load_next_city_capture();
    void SQL_common_load_finish();
protected:
    virtual void parseJustLoadedMap(const Map_to_send &,const std::string &);
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
    struct Monster_Semi_Market
    {
        //conversion x,y to position: x+y*width
        uint32_t id;
        uint64_t price;
    };

    void preload_randomBlock();
    void preload_other();
    void preload_the_data();
    void preload_the_events();
    void preload_the_ddos();
    bool preload_zone();
    void preload_industries();
    void preload_market_monsters_prices_sql();//unique table due to linked datas like skills/buffers product need of id, to be accruate on max id
    void preload_market_monsters_sql();//unique table due to linked datas like skills/buffers product need of id, to be accruate on max id
    void preload_market_items();
    bool preload_the_city_capture();
    bool preload_the_map();
    void preload_the_skin();
    void preload_the_datapack();
    void preload_the_players();
    void preload_profile();
    virtual void preload_the_visibility_algorithm();
    void preload_the_bots(const std::vector<Map_semi> &semi_loaded_map);
    virtual void preload_finish();
    void preload_plant_on_map_sql();
    static void preload_plant_on_map_static(void *object);
    void preload_plant_on_map_return();
    void preload_monsters_drops();
    void load_sql_monsters_max_id();
    static void load_monsters_max_id_static(void *object);
    void load_monsters_max_id_return();
    std::unordered_map<uint16_t,std::vector<MonsterDrops> > loadMonsterDrop(const std::string &file, std::unordered_map<uint16_t,Item> items,const std::unordered_map<uint16_t,Monster> &monsters);
    virtual void criticalDatabaseQueryFailed();
    virtual void quitForCriticalDatabaseQueryFailed() = 0;

    static void preload_zone_static(void *object);
    bool preload_zone_init();
    void preload_zone_sql();
    void preload_zone_return();
    static void preload_industries_static(void *object);
    void preload_industries_return();
    static void preload_market_items_static(void *object);
    void preload_market_items_return();
    static void preload_market_monsters_static(void *object);
    void preload_market_monsters_return();
    static void preload_market_monsters_prices_static(void *object);
    void preload_market_monsters_prices_return();
    void preload_pointOnMap_return();
    static void preload_pointOnMap_static(void *object);
    void preload_pointOnMap_sql();
    void preload_dictionary_map();
    static void preload_dictionary_map_static(void *object);
    void preload_dictionary_map_return();
    void preload_map_semi_after_db_id();

    void unload_other();
    void unload_randomBlock();
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
    void unload_dictionary();
    void unload_profile();

    void loadMonsterBuffs(const uint32_t &index);
    static void loadMonsterBuffs_static(void *object);
    void loadMonsterBuffs_return();

    void loadMonsterSkills(const uint32_t &index);
    static void loadMonsterSkills_static(void *object);
    void loadMonsterSkills_return();

    bool initialize_the_database();
    void loadBotFile(const std::string &mapfile, const std::string &fileName);
    //FakeServer server;//wrong, create another object, here need use the global static object

    //to keep client list, std::unordered_set because it will have lot of more disconnecion than server closing
    bool dataLoaded;

    std::unordered_map<std::string/*file name*/,std::unordered_map<uint8_t/*bot id*/,CatchChallenger::Bot> > botFiles;
    std::unordered_set<uint32_t> botIdLoaded;
    uint64_t timeDatapack;
    unsigned int dictionary_pointOnMap_maxId;
    BaseServerMasterSendDatapack baseServerMasterSendDatapack;
    std::vector<Monster_Semi_Market> monsterSemiMarketList;

    std::vector<FacilityLibGeneral::InodeDescriptor> entryListZone;
    int entryListIndex;
    int plant_on_the_map;
    std::vector<Map_semi> semi_loaded_map;

    static std::regex regexXmlFile;
    static const std::string text_dotxml;
    static const std::string text_zone;
    static const std::string text_capture;
    static const std::string text_fightId;
    static const std::string text_dotcomma;
    static const std::string text_male;
    static const std::string text_female;
    static const std::string text_unknown;
    static const std::string text_slash;
    static const std::string text_antislash;
    static const std::string text_type;
    static const std::string text_shop;
    static const std::string text_learn;
    static const std::string text_heal;
    static const std::string text_market;
    static const std::string text_zonecapture;
    static const std::string text_fight;
    static const std::string text_fightid;
    static const std::string text_lookAt;
    static const std::string text_left;
    static const std::string text_right;
    static const std::string text_top;
    static const std::string text_bottom;
    static const std::string text_fightRange;
    static const std::string text_bots;
    static const std::string text_bot;
    static const std::string text_id;
    static const std::string text_name;
    static const std::string text_step;
    static const std::string text_arrow;
    static const std::string text_dottmx;
    static const std::string text_shops;
    static const std::string text_product;
    static const std::string text_itemId;
    static const std::string text_overridePrice;
    static const std::string text_list;
    static const std::string text_monster;
    static const std::string text_monsters;
    static const std::string text_drops;
    static const std::string text_drop;
    static const std::string text_item;
    static const std::string text_quantity_min;
    static const std::string text_quantity_max;
    static const std::string text_luck;
    static const std::string text_percent;
};
}

#endif
