#include "BaseServer.hpp"
#include "../Client.hpp"
#include "../GlobalServerData.hpp"
#include "../DictionaryServer.hpp"
#include "../../general/base/CommonDatapack.hpp"
#include "../../general/base/CommonDatapackServerSpec.hpp"
#include "../../general/base/CommonSettingsCommon.hpp"
#include "../../general/base/CommonSettingsServer.hpp"
#include "../MapManagement/Map_server_MapVisibility_Simple_StoreOnSender.hpp"

#include <vector>
#include <time.h>
#include <iostream>

using namespace CatchChallenger;

////////////////////////////////////////////////// server stopping ////////////////////////////////////////////

void BaseServer::unload_the_data()
{
    dataLoaded=false;
    GlobalServerData::serverPrivateVariables.stopIt=true;

    unload_dictionary();
    unload_industries();
    unload_zone();
    unload_the_city_capture();
    unload_profile();
    unload_the_visibility_algorithm();
    unload_the_map();
    unload_the_bots();
    unload_the_skin();
    unload_the_datapack();
    unload_the_gift();
    unload_the_players();
    unload_the_static_data();
    unload_the_ddos();
    unload_the_events();
    unload_randomBlock();
    unload_other();
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
    #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    BaseServerMasterLoadDictionary::unload();
    #endif
    baseServerMasterSendDatapack.unload();
}

void BaseServer::unload_the_static_data()
{
    Client::simplifiedIdList.clear();
}

void BaseServer::unload_zone()
{
    GlobalServerData::serverPrivateVariables.zoneIdToMapList.clear();
    GlobalServerData::serverPrivateVariables.tradedMonster.clear();
}

void BaseServer::unload_industries()
{
}

void BaseServer::unload_the_city_capture()
{
}

void BaseServer::unload_the_bots()
{
}

void BaseServer::unload_the_map()
{
    Map_server_MapVisibility_Simple_StoreOnSender::flat_map_list.clear();
    mapPathToId.clear();
}

void BaseServer::unload_the_skin()
{
    GlobalServerData::serverPrivateVariables.skinList.clear();
}

void BaseServer::unload_the_ddos()
{
    Client::generalChatDrop.reset();
    Client::clanChatDrop.reset();
    Client::privateChatDrop.reset();
}

void BaseServer::unload_the_datapack()
{
    CommonSettingsCommon::commonSettingsCommon.datapackHashBase.clear();
    CommonSettingsServer::commonSettingsServer.datapackHashServerMain.clear();
    CommonSettingsServer::commonSettingsServer.datapackHashServerSub.clear();
    #ifndef CATCHCHALLENGER_SERVER_DATAPACK_ONLYBYMIRROR
    #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
    baseServerMasterSendDatapack.compressedExtension.clear();
    #endif
    BaseServerMasterSendDatapack::datapack_file_hash_cache_base.clear();
    Client::datapack_file_hash_cache_main.clear();
    Client::datapack_file_hash_cache_sub.clear();
    #endif
}

void BaseServer::unload_the_gift()
{
    GlobalServerData::serverPrivateVariables.gift_list.clear();
    GlobalServerData::serverSettings.daillygift.clear();
}

void BaseServer::unload_the_players()
{
    Client::simplifiedIdList.clear();
}

void BaseServer::unload_randomBlock()
{
}

void BaseServer::unload_other()
{
    #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    if(Client::protocolMessageLogicalGroupAndServerList!=NULL)
    {
        delete Client::protocolMessageLogicalGroupAndServerList;
        Client::protocolMessageLogicalGroupAndServerList=NULL;
    }
    if(Client::protocolReplyCharacterList!=NULL)
    {
        delete Client::protocolReplyCharacterList;
        Client::protocolReplyCharacterList=NULL;
    }
    #endif
    if(Client::characterIsRightFinalStepHeader!=NULL)
    {
        delete Client::characterIsRightFinalStepHeader;
        Client::characterIsRightFinalStepHeader=NULL;
    }

    CommonDatapack::commonDatapack.get_xmlLoadedFile_rw().clear();
    Map_loader::teleportConditionsUnparsed.clear();
}
