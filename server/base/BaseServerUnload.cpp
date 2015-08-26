#include "BaseServer.h"
#include "GlobalServerData.h"
#include "../../general/base/FacilityLib.h"
#include "../../general/base/FacilityLibGeneral.h"
#include "../../general/base/CommonDatapack.h"
#include "../../general/base/CommonDatapackServerSpec.h"
#include "../../general/base/DatapackGeneralLoader.h"
#include "ClientMapManagement/MapVisibilityAlgorithm_None.h"
#include "ClientMapManagement/MapVisibilityAlgorithm_Simple_StoreOnSender.h"
#include "ClientMapManagement/MapVisibilityAlgorithm_WithBorder_StoreOnSender.h"
#include "ClientMapManagement/Map_server_MapVisibility_Simple_StoreOnSender.h"
#include "ClientMapManagement/Map_server_MapVisibility_WithBorder_StoreOnSender.h"
#include "LocalClientHandlerWithoutSender.h"
#include "ClientNetworkReadWithoutSender.h"
#include "SqlFunction.h"
#include "DictionaryServer.h"
#include "DictionaryLogin.h"
#include "PreparedDBQuery.h"
#include "../../general/base/CommonSettingsCommon.h"
#include "../../general/base/CommonSettingsServer.h"
#include "../../general/base/cpp11addition.h"

#include <QFile>
#include <QByteArray>
#include <QDateTime>
#include <QTime>
#include <QCryptographicHash>
#include <time.h>
#include <iostream>
#ifndef EPOLLCATCHCHALLENGERSERVER
#include <QTimer>
#endif

using namespace CatchChallenger;

////////////////////////////////////////////////// server stopping ////////////////////////////////////////////

void BaseServer::unload_the_data()
{
    dataLoaded=false;
    GlobalServerData::serverPrivateVariables.stopIt=true;

    unload_dictionary();
    unload_market();
    unload_industries();
    unload_zone();
    unload_profile();
    unload_the_city_capture();
    unload_the_visibility_algorithm();
    unload_the_plant_on_map();
    unload_the_map();
    unload_the_bots();
    unload_monsters_drops();
    unload_the_skin();
    unload_the_datapack();
    unload_the_players();
    unload_the_static_data();
    unload_the_ddos();
    unload_the_events();
    BaseServerLogin::unload();

    CommonDatapack::commonDatapack.unload();
    CommonDatapackServerSpec::commonDatapackServerSpec.unload();
}

void BaseServer::unload_profile()
{
    GlobalServerData::serverPrivateVariables.serverProfileInternalList.clear();
}

void BaseServer::unload_dictionary()
{
    BaseServerMasterLoadDictionary::unload();
    baseServerMasterSendDatapack.unload();
    DictionaryServer::dictionary_pointOnMap_internal_to_database.clear();
    DictionaryServer::dictionary_pointOnMap_database_to_internal.clear();
}

void BaseServer::unload_the_static_data()
{
    Client::simplifiedIdList.clear();
}

void BaseServer::unload_zone()
{
    GlobalServerData::serverPrivateVariables.captureFightIdListByZoneToCaptureCity.clear();
    GlobalServerData::serverPrivateVariables.plantUsedId.clear();
}

void BaseServer::unload_market()
{
    GlobalServerData::serverPrivateVariables.tradedMonster.clear();
    GlobalServerData::serverPrivateVariables.marketPlayerMonsterList.clear();
    GlobalServerData::serverPrivateVariables.marketItemList.clear();
}

void BaseServer::unload_industries()
{
    GlobalServerData::serverPrivateVariables.industriesStatus.clear();
}

void BaseServer::unload_the_city_capture()
{
    #ifndef EPOLLCATCHCHALLENGERSERVER
    if(GlobalServerData::serverPrivateVariables.timer_city_capture!=NULL)
    {
        delete GlobalServerData::serverPrivateVariables.timer_city_capture;
        GlobalServerData::serverPrivateVariables.timer_city_capture=NULL;
    }
    #endif
}

void BaseServer::unload_the_bots()
{
}

void BaseServer::unload_the_map()
{
    semi_loaded_map.clear();
    auto i=GlobalServerData::serverPrivateVariables.map_list.begin();
    while (i != GlobalServerData::serverPrivateVariables.map_list.end())
    {
        CommonMap::removeParsedLayer(i->second->parsed_layer);
        delete i->second;
        ++i;
    }
    GlobalServerData::serverPrivateVariables.map_list.clear();
    if(GlobalServerData::serverPrivateVariables.flat_map_list!=NULL)
    {
        delete GlobalServerData::serverPrivateVariables.flat_map_list;
        GlobalServerData::serverPrivateVariables.flat_map_list=NULL;
    }
    botIdLoaded.clear();
    Client::indexOfItemOnMap=0;
    #ifdef CATCHCHALLENGER_GAMESERVER_PLANTBYPLAYER
    Client::indexOfDirtOnMap=0;//index of plant on map, ordened by map and x,y ordened into the xml file, less bandwith than send map,x,y
    #endif
}

void BaseServer::unload_the_skin()
{
    GlobalServerData::serverPrivateVariables.skinList.clear();
}

void BaseServer::unload_the_visibility_algorithm()
{
    if(GlobalServerData::serverPrivateVariables.timer_to_send_insert_move_remove!=NULL)
    {
        #ifndef EPOLLCATCHCHALLENGERSERVER
        GlobalServerData::serverPrivateVariables.timer_to_send_insert_move_remove->stop();
        GlobalServerData::serverPrivateVariables.timer_to_send_insert_move_remove->deleteLater();
        #else
        delete GlobalServerData::serverPrivateVariables.timer_to_send_insert_move_remove;
        #endif
    }
}

void BaseServer::unload_the_ddos()
{
    Client::generalChatDrop.clear();
    Client::clanChatDrop.clear();
    Client::privateChatDrop.clear();
}

void BaseServer::unload_the_events()
{
    GlobalServerData::serverPrivateVariables.events.clear();
    unsigned int index=0;
    while(index<GlobalServerData::serverPrivateVariables.timerEvents.size())
    {
        delete GlobalServerData::serverPrivateVariables.timerEvents.at(index);
        index++;
    }
    GlobalServerData::serverPrivateVariables.timerEvents.clear();
}

void BaseServer::unload_the_datapack()
{
    baseServerMasterSendDatapack.compressedExtension.clear();
    Client::datapack_file_hash_cache_base.clear();
    Client::datapack_file_hash_cache_main.clear();
    Client::datapack_file_hash_cache_sub.clear();
}

void BaseServer::unload_the_players()
{
    Client::simplifiedIdList.clear();
}
