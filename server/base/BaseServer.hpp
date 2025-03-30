#ifndef CATCHCHALLENGER_BASESERVER_H
#define CATCHCHALLENGER_BASESERVER_H

#include <vector>
#include <string>
#include <regex>
#include <atomic>

#include "../../general/base/FacilityLibGeneral.hpp"
#include "../../general/base/lib.h"
#include "../../general/base/Map_loader.hpp"
#include "../../general/tinyXML2/tinyxml2.hpp"
#ifdef CATCHCHALLENGER_CACHE_HPS
#include "../../general/hps/hps.h"
#endif
#include "ServerStructures.hpp"
#include "BaseServerMasterLoadDictionary.hpp"
#include "BaseServerMasterSendDatapack.hpp"
#include "BaseServerLogin.hpp"

namespace CatchChallenger {
class BaseServer :
        #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
        public BaseServerMasterLoadDictionary,
        #endif
        public BaseServerLogin
{
public:
    explicit BaseServer();
    virtual ~BaseServer();
    DLL_PUBLIC void setSettings(const GameServerSettings &settings);
    #ifdef CATCHCHALLENGER_CACHE_HPS
    #ifdef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    void setMaster(const std::string &master_host, const uint16_t &master_port,
                   const uint8_t &master_tryInterval, const uint8_t &master_considerDownAfterNumberOfTry);
    #endif
    #endif
    DLL_PUBLIC GameServerSettings getSettings() const;
    static void initialize_the_database_prepared_query();
    #ifdef CATCHCHALLENGER_CACHE_HPS
    void setSave(const std::string &file);
    void setLoad(const std::string &file);
    bool binaryCacheIsOpen() const;
    bool binaryInputCacheIsOpen() const;
    bool binaryOutputCacheIsOpen() const;
    NormalServerSettings loadSettingsFromBinaryCache(std::string &master_host,
                                                     uint16_t &master_port,
                                                     uint8_t &master_tryInterval,
                                                     uint8_t &master_considerDownAfterNumberOfTry);
    void setNormalSettings(const NormalServerSettings &normalServerSettings);
    #endif

    #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    void preload_20_async_clan_max_id();
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
    #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    void SQL_common_load_finish();
    #endif
protected:
    void closeDB();
    //starting function
    virtual void loadAndFixSettings();

    //stat
    enum ServerStat
    {
        Down=0,
        InUp=1,
        Up=2,
        InDown=3
    };
    std::atomic<ServerStat> stat;

    virtual void setEventTimer(const uint8_t &event,const uint8_t &value,const unsigned int &time,const unsigned int &start) = 0;
    void preload_4_sync_randomBlock();
    void preload_30_sync_other();
    void preload_1_the_data();
    void preload_5_sync_the_events();
    void preload_3_sync_the_ddos();
    bool preload_zone();
    void preload_21_async_industries();
    void preload_17_async_baseServerMasterLoadDictionaryLoad();
    bool preload_the_city_capture();
    bool preload_9_sync_the_map();
    void preload_7_sync_the_skin();
    void preload_6_sync_the_datapack();
    void preload_10_sync_the_gift();
    void preload_11_sync_the_players();
    void preload_18_sync_profile();
    virtual void preload_the_visibility_algorithm();
    virtual void preload_finish();
    void preload_8_sync_monsters_drops();
    void preload_19_async_sql_monsters_max_id();
    static void load_monsters_max_id_static(void *object);
    void load_monsters_max_id_return();
    virtual void criticalDatabaseQueryFailed();
    virtual void quitForCriticalDatabaseQueryFailed() = 0;

    static void preload_zone_static(void *object);
    bool preload_zone_init();
    void preload_16_async_zone_sql();
    void preload_zone_return();
    static void preload_industries_static(void *object);
    void preload_industries_return();
    void preload_12_async_dictionary_map();
    static void preload_dictionary_map_static(void *object);
    void preload_dictionary_map_return();
    void preload_15_async_map_semi_after_db_id();

    void unload_other();
    void unload_randomBlock();
    void unload_industries();
    void unload_zone();
    void unload_the_city_capture();
    void unload_the_bots();
    void unload_the_data();
    void unload_the_static_data();
    void unload_the_map();
    void unload_the_skin();
    void unload_the_datapack();
    void unload_the_gift();
    void unload_the_players();
    virtual void unload_the_visibility_algorithm() = 0;
    void unload_the_ddos();
    virtual void unload_the_events() = 0;
    void unload_the_plant_on_map();
    void unload_shop();
    void unload_dictionary();
    void unload_profile();

/*    void loadMonsterBuffs(const uint32_t &index);
    static void loadMonsterBuffs_static(void *object);
    void loadMonsterBuffs_return();

    void loadMonsterSkills(const uint32_t &index);
    static void loadMonsterSkills_static(void *object);
    void loadMonsterSkills_return();*/

    bool initialize_the_database();
    //FakeServer server;//wrong, create another object, here need use the global static object

    //to keep client list, std::unordered_set because it will have lot of more disconnecion than server closing
    bool dataLoaded;

    std::unordered_map<std::string/*file name*/,std::unordered_map<uint16_t/*bot id, \see CommonDatapackServerSpec, Map_to_send,struct Bot_Semi,uint16_t id, 16Bit*/,CatchChallenger::Bot> > botFiles;
    std::unordered_set<uint32_t> botIdLoaded;
    uint64_t timeDatapack;
    BaseServerMasterSendDatapack baseServerMasterSendDatapack;

    std::vector<FacilityLibGeneral::InodeDescriptor> entryListZone;
    unsigned int entryListIndex;
    #ifdef EPOLLCATCHCHALLENGERSERVER
    std::vector<tinyxml2::XMLDocument *> toDeleteAfterBotLoad;
    #endif
    bool preload_industries_call;

    std::unordered_map<std::string, CATCHCHALLENGER_TYPE_MAPID> mapPathToId;

    #ifdef CATCHCHALLENGER_CACHE_HPS
    std::ifstream *in_file;
    hps::StreamInputBuffer *serialBuffer;
    std::ofstream *out_file;
    #endif

    #ifdef CATCHCHALLENGER_DB_FILE
public:
    //mostly static
    static std::ifstream *dictionary_in_file;
    static hps::StreamInputBuffer *dictionary_serialBuffer;
    static bool dictionary_haveChange;
    //mostly dynamic
    std::ifstream *server_in_file;
    hps::StreamInputBuffer *server_serialBuffer;
    #endif
};
}

#endif
