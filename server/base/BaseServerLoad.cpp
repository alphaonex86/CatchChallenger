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

#include <openssl/sha.h>
#include <vector>
#include <time.h>
#include <iostream>
#include <algorithm>
#include <regex>
#ifndef EPOLLCATCHCHALLENGERSERVER
#include <QFile>
#include <QDateTime>
#include <QTime>
#include <QCryptographicHash>
#include <QTimer>
#endif

using namespace CatchChallenger;

void BaseServer::preload_randomBlock()
{
    //don't use BaseServerLogin::fpRandomFile to prevent lost entropy
    GlobalServerData::serverPrivateVariables.randomData.resize(CATCHCHALLENGER_SERVER_RANDOM_INTERNAL_SIZE);
    int index=0;
    while(index<CATCHCHALLENGER_SERVER_RANDOM_INTERNAL_SIZE)
    {
        GlobalServerData::serverPrivateVariables.randomData[index]=uint8_t(rand()%256);
        index++;
    }
}

void BaseServer::preload_other()
{
    #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    //game server only, the server data message
    {
        if(CommonSettingsServer::commonSettingsServer.exportedXml.size()>(1024-64))
        {
            std::cerr << "The server is alonem why do you need more than 900 char for description? Limited to 900 to limit the memory usage" << std::endl;
            abort();
        }
        char logicalGroup[64];
        Client::protocolMessageLogicalGroupAndServerListSize=0;
        //C20F
        {
            //no logical group
            //send the network message
            logicalGroup[0x00]=0x44;
            //size
            logicalGroup[0x01]=0x04;
            logicalGroup[0x02]=0x00;
            logicalGroup[0x03]=0x00;
            logicalGroup[0x04]=0x00;
            //one logical group
            logicalGroup[0x05]=0x01;
            //empty string
            logicalGroup[0x06]=0x00;
            logicalGroup[0x07]=0x00;
            logicalGroup[0x08]=0x00;

            Client::protocolMessageLogicalGroupAndServerListSize+=9;
        }
        uint32_t posOutput=0;
        //C20E
        {
            //send the network message
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x40;
            posOutput+=1+4;

            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x02;//Server mode, unique then proxy
            posOutput+=1;
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x01;//server list size, only this alone server
            posOutput+=1;
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x00;//charactersgroup empty
            posOutput+=1;
            *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=0x00000000;//unique key, useless here
            posOutput+=4;
            {
                const std::string &text=CommonSettingsServer::commonSettingsServer.exportedXml;
                *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole16(text.size());
                posOutput+=2;
                memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,text.data(),text.size());
                posOutput+=text.size();
            }
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x00;//logical group empty
            posOutput+=1;
            if(GlobalServerData::serverSettings.sendPlayerNumber)
            {
                *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole16(GlobalServerData::serverSettings.max_players);
                posOutput+=2;
                Client::protocolMessageLogicalGroupAndServerListPosPlayerNumber=Client::protocolMessageLogicalGroupAndServerListSize+posOutput;
                *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=0;
                posOutput+=2;
            }
            else
            {
                if(GlobalServerData::serverSettings.max_players<=255)
                    *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole16(65534);
                else
                    *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole16(65535);
                posOutput+=2;
                *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole16(0);
                posOutput+=2;
            }
            Client::protocolMessageLogicalGroupAndServerListSize+=posOutput;
            *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1)=htole32(posOutput-1-4);//set the dynamic size
        }
        if(Client::protocolMessageLogicalGroupAndServerList!=NULL)
            delete Client::protocolMessageLogicalGroupAndServerList;
        Client::protocolMessageLogicalGroupAndServerList=(unsigned char *)malloc(Client::protocolMessageLogicalGroupAndServerListSize+16);
        memcpy(Client::protocolMessageLogicalGroupAndServerList,logicalGroup,9);
        memcpy(Client::protocolMessageLogicalGroupAndServerList+9,ProtocolParsingBase::tempBigBufferForOutput,Client::protocolMessageLogicalGroupAndServerListSize);
    }

    //charater list reply header
    {
        //send the network reply
        uint32_t posOutput=0;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT;
        posOutput+=1+1+4;

        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=01;//all is good
        posOutput+=1;

        //login/common part
        *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole32(CommonSettingsCommon::commonSettingsCommon.character_delete_time);
        posOutput+=4;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CommonSettingsCommon::commonSettingsCommon.max_character;
        posOutput+=1;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CommonSettingsCommon::commonSettingsCommon.min_character;
        posOutput+=1;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CommonSettingsCommon::commonSettingsCommon.max_pseudo_size;
        posOutput+=1;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CommonSettingsCommon::commonSettingsCommon.maxPlayerMonsters;
        posOutput+=1;
        *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole16(CommonSettingsCommon::commonSettingsCommon.maxWarehousePlayerMonsters);
        posOutput+=2;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CommonSettingsCommon::commonSettingsCommon.maxPlayerItems;
        posOutput+=1;
        *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole16(CommonSettingsCommon::commonSettingsCommon.maxWarehousePlayerItems);
        posOutput+=2;

        if(CommonSettingsCommon::commonSettingsCommon.datapackHashBase.size()!=28)
        {
            std::cerr << "CommonSettingsCommon::commonSettingsCommon.datapackHashBase.size()!=28 into BaseServer::preload_other()" << std::endl;
            abort();
        }
        memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,CommonSettingsCommon::commonSettingsCommon.datapackHashBase.data(),CommonSettingsCommon::commonSettingsCommon.datapackHashBase.size());
        posOutput+=CommonSettingsCommon::commonSettingsCommon.datapackHashBase.size();
        {
            const std::string &text=CommonSettingsCommon::commonSettingsCommon.httpDatapackMirrorBase;
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=text.size();
            posOutput+=1;
            memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,text.data(),text.size());
            posOutput+=text.size();
        }

        Client::protocolReplyCharacterListSize=posOutput;

        if(Client::protocolReplyCharacterList!=NULL)
            delete Client::protocolReplyCharacterList;
        Client::protocolReplyCharacterList=(unsigned char *)malloc(Client::protocolReplyCharacterListSize+16);
        memcpy(Client::protocolReplyCharacterList,ProtocolParsingBase::tempBigBufferForOutput,Client::protocolReplyCharacterListSize);
    }
    #endif




    //load the character reply header
    {
        uint32_t posOutput=0;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT;
        posOutput+=1+1+4;

        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x01;// all is good
        posOutput+=1;

        if(GlobalServerData::serverSettings.sendPlayerNumber)
            *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole16(GlobalServerData::serverSettings.max_players);
        else
        {
            if(GlobalServerData::serverSettings.max_players<=255)
                *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole16(65534);
            else
                *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole16(65535);
        }
        posOutput+=2;
        #ifndef EPOLLCATCHCHALLENGERSERVER
        if(GlobalServerData::serverPrivateVariables.timer_city_capture==NULL)
            *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=0x00000000;
        else if(GlobalServerData::serverPrivateVariables.timer_city_capture->isActive())
        {
            const int64_t &time=GlobalServerData::serverPrivateVariables.time_city_capture-QDateTime::currentMSecsSinceEpoch();
            *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole32(time/1000);
        }
        else
            *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=0x00000000;
        #else
        *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=0x00000000;
        #endif
        posOutput+=4;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=GlobalServerData::serverSettings.city.capture.frenquency;
        posOutput+=1;

        //common settings
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CommonSettingsServer::commonSettingsServer.plantOnlyVisibleByPlayer;
        posOutput+=1;
        *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole32(CommonSettingsServer::commonSettingsServer.waitBeforeConnectAfterKick);
        posOutput+=4;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CommonSettingsServer::commonSettingsServer.forceClientToSendAtMapChange;
        posOutput+=1;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CommonSettingsServer::commonSettingsServer.forcedSpeed;
        posOutput+=1;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CommonSettingsServer::commonSettingsServer.useSP;
        posOutput+=1;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CommonSettingsServer::commonSettingsServer.tcpCork;
        posOutput+=1;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CommonSettingsServer::commonSettingsServer.autoLearn;
        posOutput+=1;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CommonSettingsServer::commonSettingsServer.dontSendPseudo;
        posOutput+=1;
        *reinterpret_cast<float *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole32(CommonSettingsServer::commonSettingsServer.rates_xp);
        posOutput+=4;
        *reinterpret_cast<float *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole32(CommonSettingsServer::commonSettingsServer.rates_gold);
        posOutput+=4;
        *reinterpret_cast<float *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole32(CommonSettingsServer::commonSettingsServer.rates_xp_pow);
        posOutput+=4;
        *reinterpret_cast<float *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole32(CommonSettingsServer::commonSettingsServer.rates_drop);
        posOutput+=4;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CommonSettingsServer::commonSettingsServer.chat_allow_all;
        posOutput+=1;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CommonSettingsServer::commonSettingsServer.chat_allow_local;
        posOutput+=1;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CommonSettingsServer::commonSettingsServer.chat_allow_private;
        posOutput+=1;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CommonSettingsServer::commonSettingsServer.chat_allow_clan;
        posOutput+=1;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CommonSettingsServer::commonSettingsServer.factoryPriceChange;
        posOutput+=1;

        //Main type code
        {
            const std::string &text=CommonSettingsServer::commonSettingsServer.mainDatapackCode;
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=text.size();
            posOutput+=1;
            memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,text.data(),text.size());
            posOutput+=text.size();
        }
        //Sub type cod
        {
            const std::string &text=CommonSettingsServer::commonSettingsServer.subDatapackCode;
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=text.size();
            posOutput+=1;
            memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,text.data(),text.size());
            posOutput+=text.size();
        }
        if(CommonSettingsServer::commonSettingsServer.datapackHashServerMain.size()!=28)
        {
            std::cerr << "CommonSettingsServer::commonSettingsServer.datapackHashServerMain.size()!=28" << std::endl;
            abort();
        }
        if(CommonSettingsCommon::commonSettingsCommon.datapackHashBase.size()!=28)
        {
            std::cerr << "CommonSettingsCommon::commonSettingsCommon.datapackHashBase.size()!=28 into BaseServer::preload_other()" << std::endl;
            abort();
        }
        //main hash
        memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,CommonSettingsServer::commonSettingsServer.datapackHashServerMain.data(),28);
        posOutput+=28;
        //sub hash
        if(!CommonSettingsServer::commonSettingsServer.subDatapackCode.empty())
        {
            if(CommonSettingsServer::commonSettingsServer.datapackHashServerSub.size()!=28)
            {
                std::cerr << "CommonSettingsServer::commonSettingsServer.datapackHashServerSub.size()!=28" << std::endl;
                abort();
            }
            memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,CommonSettingsServer::commonSettingsServer.datapackHashServerSub.data(),28);
            posOutput+=28;
        }
        //mirror
        {
            const std::string &text=CommonSettingsServer::commonSettingsServer.httpDatapackMirrorServer;
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=text.size();
            posOutput+=1;
            memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,text.data(),text.size());
            posOutput+=text.size();
        }

        if(Client::characterIsRightFinalStepHeader!=NULL)
            delete Client::characterIsRightFinalStepHeader;
        Client::characterIsRightFinalStepHeaderSize=posOutput;
        Client::characterIsRightFinalStepHeader=(unsigned char *)malloc(Client::characterIsRightFinalStepHeaderSize+16);
        memcpy(Client::characterIsRightFinalStepHeader,ProtocolParsingBase::tempBigBufferForOutput,Client::characterIsRightFinalStepHeaderSize);
    }
}


void BaseServer::preload_the_events()
{
    GlobalServerData::serverPrivateVariables.events.clear();
    unsigned int index=0;
    while(index<CommonDatapack::commonDatapack.events.size())
    {
        GlobalServerData::serverPrivateVariables.events.push_back(0);
        index++;
    }
    {
        auto i = GlobalServerData::serverSettings.programmedEventList.begin();
        while(i!=GlobalServerData::serverSettings.programmedEventList.end())
        {
            unsigned int index=0;
            while(index<CommonDatapack::commonDatapack.events.size())
            {
                const Event &event=CommonDatapack::commonDatapack.events.at(index);
                if(event.name==i->first)
                {
                    auto j = i->second.begin();
                    while (j!=i->second.end())
                    {
                        // event.values is std::vector<std::string >
                        const auto &iter = std::find(event.values.begin(), event.values.end(), j->second.value);
                        const size_t &sub_index = std::distance(event.values.begin(), iter);
                        if(sub_index<event.values.size())
                        {
                            #ifdef EPOLLCATCHCHALLENGERSERVER
                            TimerEvents * const timer=new TimerEvents(index,sub_index);
                            GlobalServerData::serverPrivateVariables.timerEvents.push_back(timer);
                            timer->start(j->second.cycle*1000*60,j->second.offset*1000*60);
                            #else
                            GlobalServerData::serverPrivateVariables.timerEvents.push_back(new QtTimerEvents(j->second.offset*1000*60,j->second.cycle*1000*60,index,sub_index));
                            #endif
                        }
                        else
                            GlobalServerData::serverSettings.programmedEventList[i->first].erase(i->first);
                        ++j;
                    }
                    break;
                }
                index++;
            }
            if(index==CommonDatapack::commonDatapack.events.size())
                GlobalServerData::serverSettings.programmedEventList.erase(i->first);
            ++i;
        }
    }
}

void BaseServer::preload_the_ddos()
{
    unload_the_ddos();
    int index=0;
    while(index<CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE)
    {
        Client::generalChatDrop.push_back(0);
        Client::clanChatDrop.push_back(0);
        Client::privateChatDrop.push_back(0);
        index++;
    }
    Client::generalChatDropTotalCache=0;
    Client::generalChatDropNewValue=0;
    Client::clanChatDropTotalCache=0;
    Client::clanChatDropNewValue=0;
    Client::privateChatDropTotalCache=0;
    Client::privateChatDropNewValue=0;
}

bool BaseServer::preload_zone_init()
{
    const unsigned int &listsize=entryListZone.size();
    unsigned int index=0;
    while(index<listsize)
    {
        if(!regex_search(entryListZone.at(index).name,regexXmlFile))
        {
            std::cerr << entryListZone.at(index).name << " the zone file name not match" << std::endl;
            index++;
            continue;
        }
        std::string zoneCodeName=entryListZone.at(index).name;
        stringreplaceOne(zoneCodeName,CACHEDSTRING_dotxml,"");
        const std::string &file=entryListZone.at(index).absoluteFilePath;
        CATCHCHALLENGER_XMLDOCUMENT *domDocument;
        #ifndef EPOLLCATCHCHALLENGERSERVER
        if(CommonDatapack::commonDatapack.xmlLoadedFile.find(file)!=CommonDatapack::commonDatapack.xmlLoadedFile.cend())
            domDocument=&CommonDatapack::commonDatapack.xmlLoadedFile[file];
        else
        {
            domDocument=&CommonDatapack::commonDatapack.xmlLoadedFile[file];
            #else
            domDocument=new CATCHCHALLENGER_XMLDOCUMENT();
            #endif
            const auto loadOkay = domDocument->CATCHCHALLENGER_XMLDOCUMENTLOAD(CATCHCHALLENGER_XMLSTDSTRING_TONATIVESTRING(file));
            if(!CATCHCHALLENGER_XMLDOCUMENTRETURNISLOADED(loadOkay))
            {
                std::cerr << file+", "+CATCHCHALLENGER_XMLDOCUMENTERROR(domDocument) << std::endl;
                index++;
                continue;
            }
            #ifndef EPOLLCATCHCHALLENGERSERVER
        }
        #endif
        auto search = GlobalServerData::serverPrivateVariables.captureFightIdListByZoneToCaptureCity.find(zoneCodeName);
        if(search != GlobalServerData::serverPrivateVariables.captureFightIdListByZoneToCaptureCity.end())
        {
            std::cerr << "Unable to open the file: " << file.c_str() << ", zone code name already found" << std::endl;
            index++;
            continue;
        }
        const CATCHCHALLENGER_XMLELEMENT* root = domDocument->RootElement();
        if(root==NULL)
        {
            index++;
            continue;
        }
        if(!CATCHCHALLENGER_XMLNATIVETYPECOMPAREISSAME(root->CATCHCHALLENGER_XMLELENTVALUE(),XMLCACHEDSTRING_zone))
        {
            std::cerr << "Unable to open the file: " << file.c_str() << ", \"zone\" root balise not found for the xml file" << std::endl;
            index++;
            continue;
        }

        //load capture
        std::vector<uint16_t> fightIdList;
        const CATCHCHALLENGER_XMLELEMENT * capture=root->FirstChildElement(XMLCACHEDSTRING_capture);
        if(capture!=NULL)
        {
            if(CATCHCHALLENGER_XMLELENTISXMLELEMENT(capture) && capture->Attribute(XMLCACHEDSTRING_fightId)!=NULL)
            {
                bool ok;
                const std::vector<std::string> &fightIdStringList=stringsplit(CATCHCHALLENGER_XMLATTRIBUTETOSTRING(capture->Attribute(XMLCACHEDSTRING_fightId)),';');
                int sub_index=0;
                const int &listsize=fightIdStringList.size();
                while(sub_index<listsize)
                {
                    const uint16_t &fightId=stringtouint16(fightIdStringList.at(sub_index),&ok);
                    if(ok)
                    {
                        if(CommonDatapackServerSpec::commonDatapackServerSpec.botFights.find(fightId)==CommonDatapackServerSpec::commonDatapackServerSpec.botFights.end())
                            std::cerr << "bot fightId " << fightId << " not found for capture zone " << zoneCodeName << std::endl;
                        else
                            fightIdList.push_back(fightId);
                    }
                    sub_index++;
                }
                if(sub_index==listsize && !fightIdList.empty())
                    GlobalServerData::serverPrivateVariables.captureFightIdListByZoneToCaptureCity[zoneCodeName]=fightIdList;
                break;
            }
            else
                std::cerr << "Unable to open the file: " << file << ", is not an element: (at line: " << CATCHCHALLENGER_XMLELENTATLINE(capture) << ")" << std::endl;
        }
        #ifdef EPOLLCATCHCHALLENGERSERVER
        delete domDocument;
        #endif
        index++;
    }

    std::cout << GlobalServerData::serverPrivateVariables.captureFightIdListByZoneToCaptureCity.size() << " zone(s) loaded" << std::endl;
    return true;
}

void BaseServer::preload_map_semi_after_db_id()
{
    if(DictionaryServer::dictionary_map_database_to_internal.size()==0)
    {
        std::cerr << "Need be called after preload_dictionary_map()" << std::endl;
        abort();
    }

    DictionaryServer::datapack_index_temp_for_item=0;
    DictionaryServer::datapack_index_temp_for_plant=0;
    unsigned int indexMapSemi=0;
    while(indexMapSemi<semi_loaded_map.size())
    {
        const Map_semi &map_semi=semi_loaded_map.at(indexMapSemi);
        MapServer * const mapServer=static_cast<MapServer *>(map_semi.map);
        const std::string &sortFileName=mapServer->map_file;
        //dirt/plant
        {
            unsigned int index=0;
            while(index<map_semi.old_map.dirts.size())
            {
                const Map_to_send::DirtOnMap_Semi &dirt=map_semi.old_map.dirts.at(index);

                uint16_t pointOnMapDbCode;
                std::pair<uint8_t/*x*/,uint8_t/*y*/> pair(dirt.point.x,dirt.point.y);
                bool found=false;
                if(DictionaryServer::dictionary_pointOnMap_internal_to_database.find(sortFileName)!=DictionaryServer::dictionary_pointOnMap_internal_to_database.end())
                {
                    const std::map<std::pair<uint8_t/*x*/,uint8_t/*y*/>,uint16_t/*db code*/> &subItem=DictionaryServer::dictionary_pointOnMap_internal_to_database.at(sortFileName);
                    if(subItem.find(pair)!=subItem.end())
                        found=true;
                }
                if(found)
                    pointOnMapDbCode=DictionaryServer::dictionary_pointOnMap_internal_to_database.at(sortFileName).at(pair);
                else
                {
                    dictionary_pointOnMap_maxId++;
                    std::string queryText;
                    switch(GlobalServerData::serverPrivateVariables.db_server->databaseType())
                    {
                        default:
                        case DatabaseBase::DatabaseType::Mysql:
                            queryText="INSERT INTO `dictionary_pointonmap`(`id`,`map`,`x`,`y`) VALUES("+
                                    std::to_string(dictionary_pointOnMap_maxId)+","+
                                    std::to_string(mapServer->reverse_db_id)+","+
                                    std::to_string(dirt.point.x)+","+
                                    std::to_string(dirt.point.y)+");"
                                    ;
                        break;
                        case DatabaseBase::DatabaseType::SQLite:
                            queryText="INSERT INTO dictionary_pointonmap(id,map,x,y) VALUES("+
                                    std::to_string(dictionary_pointOnMap_maxId)+","+
                                    std::to_string(mapServer->reverse_db_id)+","+
                                    std::to_string(dirt.point.x)+","+
                                    std::to_string(dirt.point.y)+");"
                                    ;
                        break;
                        case DatabaseBase::DatabaseType::PostgreSQL:
                            queryText="INSERT INTO dictionary_pointonmap(id,map,x,y) VALUES("+
                                    std::to_string(dictionary_pointOnMap_maxId)+","+
                                    std::to_string(mapServer->reverse_db_id)+","+
                                    std::to_string(dirt.point.x)+","+
                                    std::to_string(dirt.point.y)+");"
                                    ;
                        break;
                    }
                    if(!GlobalServerData::serverPrivateVariables.db_server->asyncWrite(queryText))
                    {
                        std::cerr << "Sql error for: " << queryText << ", error: " << GlobalServerData::serverPrivateVariables.db_server->errorMessage() << std::endl;
                        criticalDatabaseQueryFailed();abort();//stop because can't resolv the name
                    }
                    DictionaryServer::dictionary_pointOnMap_internal_to_database[sortFileName][std::pair<uint8_t,uint8_t>(dirt.point.x,dirt.point.y)]=dictionary_pointOnMap_maxId;
                    pointOnMapDbCode=dictionary_pointOnMap_maxId;
                }

                MapServer::PlantOnMap plantOnMap;
                #ifndef CATCHCHALLENGER_GAMESERVER_PLANTBYPLAYER
                plantOnMap.plant=0;//plant id
                plantOnMap.character=0;//player id
                plantOnMap.mature_at=0;//timestamp when is mature
                plantOnMap.player_owned_expire_at=0;//timestamp when is mature
                #endif
                plantOnMap.pointOnMapDbCode=pointOnMapDbCode;
                mapServer->plants[pair]=plantOnMap;

                {
                    while((uint32_t)DictionaryServer::dictionary_pointOnMap_database_to_internal.size()<=pointOnMapDbCode)
                    {
                        DictionaryServer::MapAndPoint mapAndPoint;
                        mapAndPoint.map=NULL;
                        mapAndPoint.x=0;
                        mapAndPoint.y=0;
                        mapAndPoint.datapack_index_item=0;
                        mapAndPoint.datapack_index_plant=0;
                        DictionaryServer::dictionary_pointOnMap_database_to_internal.push_back(mapAndPoint);
                    }
                    DictionaryServer::MapAndPoint &mapAndPoint=DictionaryServer::dictionary_pointOnMap_database_to_internal[pointOnMapDbCode];
                    mapAndPoint.map=mapServer;
                    mapAndPoint.x=dirt.point.x;
                    mapAndPoint.y=dirt.point.y;
                    mapAndPoint.datapack_index_plant=DictionaryServer::datapack_index_temp_for_plant;
                    DictionaryServer::datapack_index_temp_for_plant++;
                }
                index++;
            }
        }

        //item on map
        {
            unsigned int index=0;
            while(index<map_semi.old_map.items.size())
            {
                const Map_to_send::ItemOnMap_Semi &item=map_semi.old_map.items.at(index);

                const std::pair<uint8_t/*x*/,uint8_t/*y*/> pair(item.point.x,item.point.y);
                uint16_t pointOnMapDbCode;
                bool found=false;
                if(DictionaryServer::dictionary_pointOnMap_internal_to_database.find(sortFileName)!=DictionaryServer::dictionary_pointOnMap_internal_to_database.end())
                {
                    const std::map<std::pair<uint8_t/*x*/,uint8_t/*y*/>,uint16_t/*db code*/> &subItem=DictionaryServer::dictionary_pointOnMap_internal_to_database.at(sortFileName);
                    if(subItem.find(pair)!=subItem.end())
                        found=true;
                }
                if(found)
                    pointOnMapDbCode=DictionaryServer::dictionary_pointOnMap_internal_to_database.at(sortFileName).at(pair);
                else
                {
                    dictionary_pointOnMap_maxId++;
                    std::string queryText;
                    switch(GlobalServerData::serverPrivateVariables.db_server->databaseType())
                    {
                        default:
                        case DatabaseBase::DatabaseType::Mysql:
                            queryText="INSERT INTO `dictionary_pointonmap`(`id`,`map`,`x`,`y`) VALUES("+
                                    std::to_string(dictionary_pointOnMap_maxId)+","+
                                    std::to_string(mapServer->reverse_db_id)+","+
                                    std::to_string(item.point.x)+","+
                                    std::to_string(item.point.y)+");"
                                    ;
                        break;
                        case DatabaseBase::DatabaseType::SQLite:
                            queryText="INSERT INTO dictionary_pointonmap(id,map,x,y) VALUES("+
                                    std::to_string(dictionary_pointOnMap_maxId)+","+
                                    std::to_string(mapServer->reverse_db_id)+","+
                                    std::to_string(item.point.x)+","+
                                    std::to_string(item.point.y)+");"
                                    ;
                        break;
                        case DatabaseBase::DatabaseType::PostgreSQL:
                            queryText="INSERT INTO dictionary_pointonmap(id,map,x,y) VALUES("+
                                    std::to_string(dictionary_pointOnMap_maxId)+","+
                                    std::to_string(mapServer->reverse_db_id)+","+
                                    std::to_string(item.point.x)+","+
                                    std::to_string(item.point.y)+");"
                                    ;
                        break;
                    }
                    if(!GlobalServerData::serverPrivateVariables.db_server->asyncWrite(queryText))
                    {
                        std::cerr << "Sql error for: " << queryText << ", error: " << GlobalServerData::serverPrivateVariables.db_server->errorMessage() << std::endl;
                        criticalDatabaseQueryFailed();abort();//stop because can't resolv the name
                    }
                    DictionaryServer::dictionary_pointOnMap_internal_to_database[sortFileName][std::pair<uint8_t,uint8_t>(item.point.x,item.point.y)]=dictionary_pointOnMap_maxId;
                    pointOnMapDbCode=dictionary_pointOnMap_maxId;
                }

                MapServer::ItemOnMap itemOnMap;
                itemOnMap.infinite=item.infinite;
                itemOnMap.item=item.item;
                itemOnMap.pointOnMapDbCode=pointOnMapDbCode;
                mapServer->pointOnMap_Item[pair]=itemOnMap;

                {
                    while((uint32_t)DictionaryServer::dictionary_pointOnMap_database_to_internal.size()<=pointOnMapDbCode)
                    {
                        DictionaryServer::MapAndPoint mapAndPoint;
                        mapAndPoint.map=NULL;
                        mapAndPoint.x=0;
                        mapAndPoint.y=0;
                        mapAndPoint.datapack_index_item=0;
                        mapAndPoint.datapack_index_plant=0;
                        DictionaryServer::dictionary_pointOnMap_database_to_internal.push_back(mapAndPoint);
                    }
                    DictionaryServer::MapAndPoint &mapAndPoint=DictionaryServer::dictionary_pointOnMap_database_to_internal[pointOnMapDbCode];
                    mapAndPoint.map=mapServer;
                    mapAndPoint.x=item.point.x;
                    mapAndPoint.y=item.point.y;
                    mapAndPoint.datapack_index_item=DictionaryServer::datapack_index_temp_for_item;
                    DictionaryServer::datapack_index_temp_for_item++;
                }
                index++;
            }
        }
        indexMapSemi++;
    }

    semi_loaded_map.clear();
    plant_on_the_map=0;
    preload_zone_sql();
}

/**
 * into the BaseServerLogin
 * */
void BaseServer::preload_profile()
{
    #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    std::cout << DictionaryLogin::dictionary_skin_database_to_internal.size() << " SQL skin dictionary" << std::endl;
    #endif

    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    /*if(CommonDatapack::commonDatapack.profileList.size()!=GlobalServerData::serverPrivateVariables.serverProfileList.size())
    {
        std::cout << "profile common and server don't match" << std::endl;
        return;
    }*/
    if(CommonDatapack::commonDatapack.profileList.size()!=CommonDatapackServerSpec::commonDatapackServerSpec.serverProfileList.size())
    {
        std::cout << "profile common and server don't match" << std::endl;
        return;
    }
    #endif
    {
        unsigned int index=0;
        while(index<CommonDatapackServerSpec::commonDatapackServerSpec.serverProfileList.size())
        {
            stringreplaceOne(CommonDatapackServerSpec::commonDatapackServerSpec.serverProfileList[index].mapString,CACHEDSTRING_dottmx,"");
            index++;
        }
    }
    GlobalServerData::serverPrivateVariables.serverProfileInternalList.clear();

    GlobalServerData::serverPrivateVariables.serverProfileInternalList.resize(CommonDatapack::commonDatapack.profileList.size());
    #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    const DatabaseBase::DatabaseType &type=GlobalServerData::serverPrivateVariables.db_common->databaseType();
    #endif

    unsigned int index=0;
    while(index<CommonDatapack::commonDatapack.profileList.size())
    {
        const ServerSpecProfile &serverProfile=CommonDatapackServerSpec::commonDatapackServerSpec.serverProfileList.at(index);
        ServerProfileInternal &serverProfileInternal=GlobalServerData::serverPrivateVariables.serverProfileInternalList.at(index);
        serverProfileInternal.map=
                static_cast<MapServer *>(GlobalServerData::serverPrivateVariables.map_list.at(serverProfile.mapString));
        serverProfileInternal.x=serverProfile.x;
        serverProfileInternal.y=serverProfile.y;
        serverProfileInternal.orientation=serverProfile.orientation;

        #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
        const Profile &profile=CommonDatapack::commonDatapack.profileList.at(index);

        std::string encyclopedia_item,item;
        if(!profile.items.empty())
        {
            uint32_t lastItemId=0;
            auto item_list=profile.items;
            std::sort(item_list.begin(),item_list.end(),[](const Profile::Item &a, const Profile::Item &b) {
                return a.id<b.id;
            });
            auto max=item_list.at(item_list.size()-1).id;
            uint32_t pos=0;
            char item_raw[(2+4)*item_list.size()];
            unsigned int index=0;
            while(index<item_list.size())
            {
                const auto &item=item_list.at(index);
                *reinterpret_cast<uint16_t *>(item_raw+pos)=htole16(item.id-lastItemId);
                pos+=2;
                lastItemId=item.id;
                *reinterpret_cast<uint32_t *>(item_raw+pos)=htole32(item.quantity);
                pos+=4;
                index++;
            }
            item=binarytoHexa(item_raw,sizeof(item_raw));

            const size_t size=max/8+1;
            char bitlist[size];
            memset(bitlist,0,size);
            index=0;
            while(index<item_list.size())
            {
                const auto &item=profile.items.at(index);
                uint16_t bittoUp=item.id;
                bitlist[bittoUp/8]|=(1<<(7-bittoUp%8));
                index++;
            }
            encyclopedia_item=binarytoHexa(bitlist,sizeof(bitlist));
        }
        std::string reputations;
        if(!profile.reputations.empty())
        {
            uint32_t lastReputationId=0;
            auto reputations_list=profile.reputations;
            std::sort(reputations_list.begin(),reputations_list.end(),[](const Profile::Reputation &a, const Profile::Reputation &b) {
                return a.reputationDatabaseId<b.reputationDatabaseId;
            });
            uint32_t pos=0;
            char reputation_raw[(1+4+1)*reputations_list.size()];
            unsigned int index=0;
            while(index<reputations_list.size())
            {
                const auto &reputation=reputations_list.at(index);
                *reinterpret_cast<uint32_t *>(reputation_raw+pos)=htole32(reputation.point);
                pos+=4;
                reputation_raw[pos]=reputation.reputationDatabaseId-lastReputationId;
                pos+=1;
                lastReputationId=reputation.reputationDatabaseId;
                reputation_raw[pos]=reputation.level;
                pos+=1;
                index++;
            }
            reputations=binarytoHexa(reputation_raw,sizeof(reputation_raw));
        }

        //assume here all is the same type
        {
            unsigned int monsterGroupIndex=0;
            serverProfileInternal.monster_insert.resize(profile.monstergroup.size());
            while(monsterGroupIndex<profile.monstergroup.size())
            {
                const auto &monsters=profile.monstergroup.at(monsterGroupIndex);
                std::vector<uint16_t> monsterForEncyclopedia;
                std::vector<StringWithReplacement> &monsterGroupQuery=serverProfileInternal.monster_insert[monsterGroupIndex];
                unsigned int monsterIndex=0;
                while(monsterIndex<monsters.size())
                {
                    const auto &monster=monsters.at(monsterIndex);
                    const auto &monsterDatapack=CommonDatapack::commonDatapack.monsters.at(monster.id);
                    const Monster::Stat &monsterStat=CommonFightEngineBase::getStat(monsterDatapack,monster.level);
                    std::vector<CatchChallenger::PlayerMonster::PlayerSkill> skills_list=CommonFightEngineBase::generateWildSkill(monsterDatapack,monster.level);
                    if(skills_list.empty())
                    {
                        std::cerr << "skills_list.empty() for some profile" << std::endl;
                        abort();
                    }
                    uint32_t lastSkillId=0;
                    std::sort(skills_list.begin(),skills_list.end(),[](const CatchChallenger::PlayerMonster::PlayerSkill &a, const CatchChallenger::PlayerMonster::PlayerSkill &b) {
                        return a.skill<b.skill;
                    });
                    char raw_skill[(2+1)*skills_list.size()],raw_skill_endurance[1*skills_list.size()];
                    //the skills
                    unsigned int sub_index=0;
                    while(sub_index<skills_list.size())
                    {
                        const auto &skill=skills_list.at(sub_index);
                        *reinterpret_cast<uint16_t *>(raw_skill+sub_index*(2+1))=htole16(skill.skill-lastSkillId);
                        lastSkillId=skill.skill;
                        raw_skill[sub_index*(2+1)+2]=skill.level;
                        raw_skill_endurance[sub_index]=skill.endurance;
                        sub_index++;
                    }
                    //dynamic part
                    {
                        //id,character,place,hp,monster,level,xp,sp,captured_with,gender,egg_step,character_origin,position,skills,skills_endurance
                        if(monsterDatapack.ratio_gender!=-1)
                        {
                            const std::string &queryText=PreparedDBQueryCommon::db_query_insert_monster.compose(
                                    "%1",
                                    "%2",
                                    "1",
                                    std::to_string(monsterStat.hp),
                                    std::to_string(monster.id),
                                    std::to_string(monster.level),
                                    std::to_string(monster.captured_with),
                                    "%3",
                                    "%4",
                                    std::to_string(monsterIndex+1),
                                    binarytoHexa(raw_skill,sizeof(raw_skill)),
                                    binarytoHexa(raw_skill_endurance,sizeof(raw_skill_endurance))
                                    );
                            monsterGroupQuery.push_back(queryText);
                        }
                        else
                        {
                            const std::string &queryText=PreparedDBQueryCommon::db_query_insert_monster.compose(
                                    "%1",
                                    "%2",
                                    "1",
                                    std::to_string(monsterStat.hp),
                                    std::to_string(monster.id),
                                    std::to_string(monster.level),
                                    std::to_string(monster.captured_with),
                                    "3",
                                    "%3",
                                    std::to_string(monsterIndex+1),
                                    binarytoHexa(raw_skill,sizeof(raw_skill)),
                                    binarytoHexa(raw_skill_endurance,sizeof(raw_skill_endurance))
                                    );
                            monsterGroupQuery.push_back(queryText);
                        }
                    }
                    monsterForEncyclopedia.push_back(monster.id);
                    monsterIndex++;
                }
                //do the encyclopedia monster
                const auto &result=std::max_element(monsterForEncyclopedia.begin(),monsterForEncyclopedia.end());
                const size_t size=*result/8+1;
                char bitlist[size];
                memset(bitlist,0,size);
                monsterIndex=0;
                while(monsterIndex<monsterForEncyclopedia.size())
                {
                    uint16_t bittoUp=monsterForEncyclopedia.at(monsterIndex);
                    bitlist[bittoUp/8]|=(1<<(7-bittoUp%8));
                    monsterIndex++;
                }
                serverProfileInternal.monster_encyclopedia_insert.push_back(binarytoHexa(bitlist,sizeof(bitlist)));

                monsterGroupIndex++;
            }
        }
        switch(type)
        {
            default:
            case DatabaseBase::DatabaseType::Mysql:
                serverProfileInternal.character_insert=std::string("INSERT INTO `character`("
                        "`id`,`account`,`pseudo`,`skin`,`type`,`clan`,`cash`,`date`,`warehouse_cash`,`clan_leader`,"
                        "`time_to_delete`,`played_time`,`last_connect`,`starter`,`item`,`reputations`,`encyclopedia_monster`,`encyclopedia_item`"
                        ",`blob_version`) VALUES(%1,%2,'%3',%4,0,0,"+
                        std::to_string(profile.cash)+",%5,0,0,"
                        "0,0,0,"+
                        std::to_string(DictionaryLogin::dictionary_starter_internal_to_database.at(index)/*starter*/)+",UNHEX('"+item+"'),UNHEX('"+reputations+"'),UNHEX('%6'),UNHEX('"+encyclopedia_item+"'),"+std::to_string(GlobalServerData::serverPrivateVariables.server_blobversion_datapack)+");");
            break;
            case DatabaseBase::DatabaseType::SQLite:
                serverProfileInternal.character_insert=std::string("INSERT INTO character("
                        "id,account,pseudo,skin,type,clan,cash,date,warehouse_cash,clan_leader,"
                        "time_to_delete,played_time,last_connect,starter,item,reputations,encyclopedia_monster,encyclopedia_item"
                        ",blob_version) VALUES(%1,%2,'%3',%4,0,0,"+
                        std::to_string(profile.cash)+",%5,0,0,"
                        "0,0,0,"+
                        std::to_string(DictionaryLogin::dictionary_starter_internal_to_database.at(index)/*starter*/)+",'"+item+"','"+reputations+"','%6'','"+encyclopedia_item+"',"+std::to_string(GlobalServerData::serverPrivateVariables.server_blobversion_datapack)+");");
            break;
            case DatabaseBase::DatabaseType::PostgreSQL:
                serverProfileInternal.character_insert=std::string("INSERT INTO character("
                        "id,account,pseudo,skin,type,clan,cash,date,warehouse_cash,clan_leader,"
                        "time_to_delete,played_time,last_connect,starter,item,reputations,encyclopedia_monster,encyclopedia_item"
                        ",blob_version) VALUES(%1,%2,'%3',%4,0,0,"+
                        std::to_string(profile.cash)+",%5,0,FALSE,"
                        "0,0,0,"+
                        std::to_string(DictionaryLogin::dictionary_starter_internal_to_database.at(index)/*starter*/)+",'\\x"+item+"','\\x"+reputations+"','\\x%6','\\x"+encyclopedia_item+"',"+std::to_string(GlobalServerData::serverPrivateVariables.server_blobversion_datapack)+");");
            break;
        }
        #endif
        const std::string &mapQuery=std::to_string(serverProfileInternal.map->reverse_db_id)+
                ","+
                std::to_string(serverProfileInternal.x)+
                ","+
                std::to_string(serverProfileInternal.y)+
                ","+
                std::to_string(Orientation_bottom);
        #ifdef CATCHCHALLENGER_GAMESERVER_PLANTBYPLAYER
        switch(GlobalServerData::serverPrivateVariables.db_server->databaseType())
        {
            default:
            case DatabaseBase::DatabaseType::Mysql:
                serverProfileInternal.preparedQueryAddCharacterForServer=std::string("INSERT INTO `character_forserver`(`character`,`map`,`x`,`y`,`orientation`,`rescue_map`,`rescue_x`,`rescue_y`,`rescue_orientation`,`unvalidated_rescue_map`,`unvalidated_rescue_x`,`unvalidated_rescue_y`,`unvalidated_rescue_orientation`,`date`,`market_cash`,`botfight_id`,`itemonmap`,plants`,`blob_version`,`quest`) VALUES("
                "%1,"+mapQuery+","+mapQuery+","+mapQuery+",%2,0,UNHEX(''),UNHEX(''),UNHEX(''),"+std::to_string(GlobalServerData::serverPrivateVariables.server_blobversion_datapack)+",UNHEX(''));");
            break;
            case DatabaseBase::DatabaseType::SQLite:
                serverProfileInternal.preparedQueryAddCharacterForServer=std::string("INSERT INTO character_forserver(character,map,x,y,orientation,rescue_map,rescue_x,rescue_y,rescue_orientation,unvalidated_rescue_map,unvalidated_rescue_x,unvalidated_rescue_y,unvalidated_rescue_orientation,date,market_cash,botfight_id,itemonmap,plants,blob_version,quest) VALUES("
                "%1,"+mapQuery+","+mapQuery+","+mapQuery+",%2,0,'','','',"+std::to_string(GlobalServerData::serverPrivateVariables.server_blobversion_datapack)+",'');");
            break;
            case DatabaseBase::DatabaseType::PostgreSQL:
                serverProfileInternal.preparedQueryAddCharacterForServer=std::string("INSERT INTO character_forserver(character,map,x,y,orientation,rescue_map,rescue_x,rescue_y,rescue_orientation,unvalidated_rescue_map,unvalidated_rescue_x,unvalidated_rescue_y,unvalidated_rescue_orientation,date,market_cash,botfight_id,itemonmap,plants,blob_version,quest) VALUES("
                "%1,"+mapQuery+","+mapQuery+","+mapQuery+",%2,0,'\\x','\\x','\\x',"+std::to_string(GlobalServerData::serverPrivateVariables.server_blobversion_datapack)+",'\\x');");
            break;
        }
        #else
        switch(GlobalServerData::serverPrivateVariables.db_server->databaseType())
        {
            default:
            case DatabaseBase::DatabaseType::Mysql:
                serverProfileInternal.preparedQueryAddCharacterForServer=std::string("INSERT INTO `character_forserver`(`character`,`map`,`x`,`y`,`orientation`,`rescue_map`,`rescue_x`,`rescue_y`,`rescue_orientation`,`unvalidated_rescue_map`,`unvalidated_rescue_x`,`unvalidated_rescue_y`,`unvalidated_rescue_orientation`,`date`,`market_cash`,`botfight_id`,`itemonmap`,`blob_version`,`quest`) VALUES("
                "%1,"+mapQuery+","+mapQuery+","+mapQuery+",%2,0,UNHEX(''),UNHEX(''),"+std::to_string(GlobalServerData::serverPrivateVariables.server_blobversion_datapack)+",UNHEX(''));");
            break;
            case DatabaseBase::DatabaseType::SQLite:
                serverProfileInternal.preparedQueryAddCharacterForServer=std::string("INSERT INTO character_forserver(character,map,x,y,orientation,rescue_map,rescue_x,rescue_y,rescue_orientation,unvalidated_rescue_map,unvalidated_rescue_x,unvalidated_rescue_y,unvalidated_rescue_orientation,date,market_cash,botfight_id,itemonmap,blob_version,quest) VALUES("
                "%1,"+mapQuery+","+mapQuery+","+mapQuery+",%2,0,'','',"+std::to_string(GlobalServerData::serverPrivateVariables.server_blobversion_datapack)+",'');");
            break;
            case DatabaseBase::DatabaseType::PostgreSQL:
                serverProfileInternal.preparedQueryAddCharacterForServer=std::string("INSERT INTO character_forserver(character,map,x,y,orientation,rescue_map,rescue_x,rescue_y,rescue_orientation,unvalidated_rescue_map,unvalidated_rescue_x,unvalidated_rescue_y,unvalidated_rescue_orientation,date,market_cash,botfight_id,itemonmap,blob_version,quest) VALUES("
                "%1,"+mapQuery+","+mapQuery+","+mapQuery+",%2,0,'\\x','\\x',"+std::to_string(GlobalServerData::serverPrivateVariables.server_blobversion_datapack)+",'\\x');");
            break;
        }
        #endif
        serverProfileInternal.valid=true;

        index++;
    }

    if(GlobalServerData::serverPrivateVariables.serverProfileInternalList.empty())
    {
        std::cerr << "No profile loaded, seam unable to work without it" << std::endl;
        abort();
    }
    std::cout << GlobalServerData::serverPrivateVariables.serverProfileInternalList.size() << " profile loaded" << std::endl;
}

bool BaseServer::preload_zone()
{
    //open and quick check the file
    entryListZone=CatchChallenger::FacilityLibGeneral::listFolderNotRecursive(GlobalServerData::serverSettings.datapack_basePath+DATAPACK_BASE_PATH_ZONE1+CommonSettingsServer::commonSettingsServer.mainDatapackCode+DATAPACK_BASE_PATH_ZONE2,CatchChallenger::FacilityLibGeneral::ListFolder::Files);
    entryListIndex=0;
    return preload_zone_init();
}

void BaseServer::preload_zone_static(void *object)
{
    static_cast<BaseServer *>(object)->preload_zone_return();
}

void BaseServer::preload_zone_return()
{
    if(GlobalServerData::serverPrivateVariables.db_server->next())
    {
        bool ok;
        std::string zoneCodeName=entryListZone.at(entryListIndex).name;
        stringreplaceOne(zoneCodeName,CACHEDSTRING_dotxml,"");
        const std::string &tempString=std::string(GlobalServerData::serverPrivateVariables.db_server->value(0));
        const uint32_t &clanId=stringtouint32(tempString,&ok);
        if(ok)
        {
            GlobalServerData::serverPrivateVariables.cityStatusList[zoneCodeName].clan=clanId;
            GlobalServerData::serverPrivateVariables.cityStatusListReverse[clanId]=zoneCodeName;
        }
        else
            std::cerr << "clan id is failed to convert to number for city status" << std::endl;
    }
    GlobalServerData::serverPrivateVariables.db_server->clear();
    entryListIndex++;
    preload_market_monsters_sql();
}

bool BaseServer::preload_the_city_capture()
{
    #ifndef EPOLLCATCHCHALLENGERSERVER
    if(GlobalServerData::serverPrivateVariables.timer_city_capture!=NULL)
        delete GlobalServerData::serverPrivateVariables.timer_city_capture;
    GlobalServerData::serverPrivateVariables.timer_city_capture=new QTimer();
    GlobalServerData::serverPrivateVariables.timer_city_capture->setSingleShot(true);
    #endif
    return load_next_city_capture();
}

bool BaseServer::preload_the_map()
{
    GlobalServerData::serverPrivateVariables.datapack_mapPath=GlobalServerData::serverSettings.datapack_basePath+
            DATAPACK_BASE_PATH_MAPMAIN+
            CommonSettingsServer::commonSettingsServer.mainDatapackCode+
            "/";
    #ifdef DEBUG_MESSAGE_MAP_LOAD
    std::cout << "start preload the map, into: " << GlobalServerData::serverPrivateVariables.datapack_mapPath << std::endl;
    #endif
    Map_loader map_temp;
    std::vector<std::string> map_name;
    std::vector<std::string> map_name_to_do_id;
    std::vector<std::string> returnList=FacilityLibGeneral::listFolder(GlobalServerData::serverPrivateVariables.datapack_mapPath);
    std::sort(returnList.begin(), returnList.end());

    if(returnList.size()==0)
    {
        std::cerr << "No file map to list" << std::endl;
        abort();
    }
    if(!semi_loaded_map.empty() || GlobalServerData::serverPrivateVariables.flat_map_list!=NULL)
    {
        std::cerr << "preload_the_map() already call" << std::endl;
        abort();
    }
    //load the map
    unsigned int size=returnList.size();
    unsigned int index=0;
    unsigned int sub_index;
    std::string tmxRemove(".tmx");
    std::regex mapFilter("\\.tmx$");
    std::regex mapExclude("[\"']");
    std::vector<CommonMap *> flat_map_list_temp;
    while(index<size)
    {
        std::string fileName=returnList.at(index);
        stringreplaceAll(fileName,CACHEDSTRING_antislash,CACHEDSTRING_slash);
        if(regex_search(fileName,mapFilter) && !regex_search(fileName,mapExclude))
        {
            #ifdef DEBUG_MESSAGE_MAP_LOAD
            std::cout << "load the map: " << fileName << std::endl;
            #endif
            std::string sortFileName=fileName;
            stringreplaceOne(sortFileName,tmxRemove,"");
            map_name_to_do_id.push_back(sortFileName);
            if(map_temp.tryLoadMap(GlobalServerData::serverPrivateVariables.datapack_mapPath+fileName))
            {
                switch(GlobalServerData::serverSettings.mapVisibility.mapVisibilityAlgorithm)
                {
                    case MapVisibilityAlgorithmSelection_Simple:
                        flat_map_list_temp.push_back(new Map_server_MapVisibility_Simple_StoreOnSender);
                    break;
                    case MapVisibilityAlgorithmSelection_WithBorder:
                        flat_map_list_temp.push_back(new Map_server_MapVisibility_WithBorder_StoreOnSender);
                    break;
                    case MapVisibilityAlgorithmSelection_None:
                    default:
                        flat_map_list_temp.push_back(new MapServer);
                    break;
                }
                MapServer *mapServer=static_cast<MapServer *>(flat_map_list_temp.back());
                GlobalServerData::serverPrivateVariables.map_list[sortFileName]=mapServer;

                mapServer->width			= map_temp.map_to_send.width;
                mapServer->height			= map_temp.map_to_send.height;
                mapServer->parsed_layer	= map_temp.map_to_send.parsed_layer;
                mapServer->map_file		= sortFileName;

                map_name.push_back(sortFileName);

                parseJustLoadedMap(map_temp.map_to_send,fileName);

                Map_semi map_semi;
                map_semi.map				= GlobalServerData::serverPrivateVariables.map_list.at(sortFileName);

                if(map_temp.map_to_send.border.top.fileName.size()>0)
                {
                    map_semi.border.top.fileName		= Map_loader::resolvRelativeMap(GlobalServerData::serverPrivateVariables.datapack_mapPath+fileName,map_temp.map_to_send.border.top.fileName,GlobalServerData::serverPrivateVariables.datapack_mapPath);
                    stringreplaceOne(map_semi.border.top.fileName,CACHEDSTRING_dottmx,"");
                    map_semi.border.top.x_offset		= map_temp.map_to_send.border.top.x_offset;
                }
                if(map_temp.map_to_send.border.bottom.fileName.size()>0)
                {
                    map_semi.border.bottom.fileName		= Map_loader::resolvRelativeMap(GlobalServerData::serverPrivateVariables.datapack_mapPath+fileName,map_temp.map_to_send.border.bottom.fileName,GlobalServerData::serverPrivateVariables.datapack_mapPath);
                    stringreplaceOne(map_semi.border.bottom.fileName,CACHEDSTRING_dottmx,"");
                    map_semi.border.bottom.x_offset		= map_temp.map_to_send.border.bottom.x_offset;
                }
                if(map_temp.map_to_send.border.left.fileName.size()>0)
                {
                    map_semi.border.left.fileName		= Map_loader::resolvRelativeMap(GlobalServerData::serverPrivateVariables.datapack_mapPath+fileName,map_temp.map_to_send.border.left.fileName,GlobalServerData::serverPrivateVariables.datapack_mapPath);
                    stringreplaceOne(map_semi.border.left.fileName,CACHEDSTRING_dottmx,"");
                    map_semi.border.left.y_offset		= map_temp.map_to_send.border.left.y_offset;
                }
                if(map_temp.map_to_send.border.right.fileName.size()>0)
                {
                    map_semi.border.right.fileName		= Map_loader::resolvRelativeMap(GlobalServerData::serverPrivateVariables.datapack_mapPath+fileName,map_temp.map_to_send.border.right.fileName,GlobalServerData::serverPrivateVariables.datapack_mapPath);
                    stringreplaceOne(map_semi.border.right.fileName,CACHEDSTRING_dottmx,"");
                    map_semi.border.right.y_offset		= map_temp.map_to_send.border.right.y_offset;
                }

                sub_index=0;
                const unsigned int &listsize=map_temp.map_to_send.teleport.size();
                while(sub_index<listsize)
                {
                    map_temp.map_to_send.teleport[sub_index].map=Map_loader::resolvRelativeMap(GlobalServerData::serverPrivateVariables.datapack_mapPath+fileName,map_temp.map_to_send.teleport.at(sub_index).map,GlobalServerData::serverPrivateVariables.datapack_mapPath);
                    stringreplaceOne(map_temp.map_to_send.teleport[sub_index].map,CACHEDSTRING_dottmx,"");
                    sub_index++;
                }

                map_semi.old_map=map_temp.map_to_send;

                semi_loaded_map.push_back(map_semi);
            }
            else
            {
                std::cout << "error at loading: " << GlobalServerData::serverPrivateVariables.datapack_mapPath << fileName << ", error: " << map_temp.errorString()
                          << "parsed due: " << "regex_search(" << fileName << ",\\.tmx$) && !regex_search("
                          << fileName << ",[\"'])"
                          << std::endl;
                abort();
            }
        }
        index++;
    }
    {
        GlobalServerData::serverPrivateVariables.flat_map_list=static_cast<CommonMap **>(malloc(sizeof(CommonMap *)*flat_map_list_temp.size()));
        memset(GlobalServerData::serverPrivateVariables.flat_map_list,0x00,sizeof(CommonMap *)*flat_map_list_temp.size());
        unsigned int index=0;
        while(index<flat_map_list_temp.size())
        {
            GlobalServerData::serverPrivateVariables.flat_map_list[index]=flat_map_list_temp.at(index);
            index++;
        }
        if(GlobalServerData::serverSettings.mapVisibility.mapVisibilityAlgorithm==CatchChallenger::MapVisibilityAlgorithmSelection_Simple)
        {
            Map_server_MapVisibility_Simple_StoreOnSender::map_to_update=static_cast<Map_server_MapVisibility_Simple_StoreOnSender **>(malloc(sizeof(CommonMap *)*flat_map_list_temp.size()));
            memset(Map_server_MapVisibility_Simple_StoreOnSender::map_to_update,0x00,sizeof(CommonMap *)*flat_map_list_temp.size());
            Map_server_MapVisibility_Simple_StoreOnSender::map_to_update_size=0;
        }
    }

    std::sort(map_name_to_do_id.begin(),map_name_to_do_id.end());

    //resolv the border map name into their pointer
    size=semi_loaded_map.size();
    index=0;
    while(index<size)
    {
        if(semi_loaded_map.at(index).border.bottom.fileName.size()>0 && GlobalServerData::serverPrivateVariables.map_list.find(semi_loaded_map.at(index).border.bottom.fileName)!=GlobalServerData::serverPrivateVariables.map_list.end())
            semi_loaded_map[index].map->border.bottom.map=GlobalServerData::serverPrivateVariables.map_list.at(semi_loaded_map.at(index).border.bottom.fileName);
        else
            semi_loaded_map[index].map->border.bottom.map=NULL;

        if(semi_loaded_map.at(index).border.top.fileName.size()>0 && GlobalServerData::serverPrivateVariables.map_list.find(semi_loaded_map.at(index).border.top.fileName)!=GlobalServerData::serverPrivateVariables.map_list.end())
            semi_loaded_map[index].map->border.top.map=GlobalServerData::serverPrivateVariables.map_list.at(semi_loaded_map.at(index).border.top.fileName);
        else
            semi_loaded_map[index].map->border.top.map=NULL;

        if(semi_loaded_map.at(index).border.left.fileName.size()>0 && GlobalServerData::serverPrivateVariables.map_list.find(semi_loaded_map.at(index).border.left.fileName)!=GlobalServerData::serverPrivateVariables.map_list.end())
            semi_loaded_map[index].map->border.left.map=GlobalServerData::serverPrivateVariables.map_list.at(semi_loaded_map.at(index).border.left.fileName);
        else
            semi_loaded_map[index].map->border.left.map=NULL;

        if(semi_loaded_map.at(index).border.right.fileName.size()>0 && GlobalServerData::serverPrivateVariables.map_list.find(semi_loaded_map.at(index).border.right.fileName)!=GlobalServerData::serverPrivateVariables.map_list.end())
            semi_loaded_map[index].map->border.right.map=GlobalServerData::serverPrivateVariables.map_list.at(semi_loaded_map.at(index).border.right.fileName);
        else
            semi_loaded_map[index].map->border.right.map=NULL;

        index++;
    }

    //resolv the teleported into their pointer
    size=semi_loaded_map.size();
    index=0;
    while(index<size)
    {
        unsigned int sub_index=0;
        Map_semi &map_semi=semi_loaded_map.at(index);
        map_semi.map->teleporter_list_size=0;
        /*The datapack dev should fix it and then drop duplicate teleporter, if well done then the final size is map_semi.old_map.teleport.size()*/
        map_semi.map->teleporter=(CommonMap::Teleporter *)malloc(sizeof(CommonMap::Teleporter)*map_semi.old_map.teleport.size());
        memset(map_semi.map->teleporter,0x00,sizeof(CommonMap::Teleporter)*map_semi.old_map.teleport.size());
        while(sub_index<map_semi.old_map.teleport.size() && sub_index<127)//code not ready for more than 127
        {
            const auto &teleport=map_semi.old_map.teleport.at(sub_index);
            std::string teleportString=teleport.map;
            stringreplaceOne(teleportString,CACHEDSTRING_dottmx,"");
            if(GlobalServerData::serverPrivateVariables.map_list.find(teleportString)!=GlobalServerData::serverPrivateVariables.map_list.end())
            {
                if(teleport.destination_x<GlobalServerData::serverPrivateVariables.map_list.at(teleportString)->width
                        && teleport.destination_y<GlobalServerData::serverPrivateVariables.map_list.at(teleportString)->height)
                {
                    uint16_t index_search=0;
                    while(index_search<map_semi.map->teleporter_list_size)
                    {
                        if(map_semi.map->teleporter[index_search].source_x==teleport.source_x && map_semi.map->teleporter[index_search].source_y==teleport.source_y)
                            break;
                        index_search++;
                    }
                    if(index_search==map_semi.map->teleporter_list_size)
                    {
                        #ifdef DEBUG_MESSAGE_MAP_LOAD
                        std::cout << "teleporter on the map: "
                             << map_semi.map->map_file
                             << "("
                             << std::to_string(teleport.source_x)
                             << ","
                             << std::to_string(teleport.source_y)
                             << "), to "
                             << teleportString
                             << "("
                             << std::to_string(semi_loaded_map.at(index).old_map.teleport.at(sub_index).destination_x)
                             << ","
                             << std::to_string(teleport.destination_y)
                             << ")"
                             << std::endl;
                        #endif
                        CommonMap::Teleporter teleporter;
                        teleporter.map=GlobalServerData::serverPrivateVariables.map_list.at(teleportString);
                        teleporter.source_x=teleport.source_x;
                        teleporter.source_y=teleport.source_y;
                        teleporter.destination_x=teleport.destination_x;
                        teleporter.destination_y=teleport.destination_y;
                        teleporter.condition=teleport.condition;
                        semi_loaded_map[index].map->teleporter[map_semi.map->teleporter_list_size]=teleporter;
                        map_semi.map->teleporter_list_size++;
                    }
                    else
                        std::cerr << "already found teleporter on the map: "
                             << map_semi.map->map_file
                             << "("
                             << std::to_string(teleport.source_x)
                             << ","
                             << std::to_string(teleport.source_y)
                             << "), to "
                             << teleportString
                             << "("
                             << std::to_string(teleport.destination_x)
                             << ","
                             << std::to_string(teleport.destination_y)
                             << ")"
                             << std::endl;
                }
                else
                    std::cerr << "wrong teleporter on the map: "
                         << map_semi.map->map_file
                         << "("
                         << std::to_string(teleport.source_x)
                         << ","
                         << std::to_string(teleport.source_y)
                         << "), to "
                         << teleportString
                         << "("
                         << std::to_string(teleport.destination_x)
                         << ","
                         << std::to_string(teleport.destination_y)
                         << ") because the tp is out of range"
                         << std::endl;
            }
            else
                std::cerr << "wrong teleporter on the map: "
                     << map_semi.map->map_file
                     << "("
                     << std::to_string(teleport.source_x)
                     << ","
                     << std::to_string(teleport.source_y)
                     << "), to "
                     << teleportString
                     << "("
                     << std::to_string(teleport.destination_x)
                     << ","
                     << std::to_string(teleport.destination_y)
                     << ") because the map is not found"
                     << std::endl;

            sub_index++;
        }
        index++;
    }

    //clean border balise without another oposite border
    size=semi_loaded_map.size();
    index=0;
    while(index<size)
    {
        const auto &currentTempMap=GlobalServerData::serverPrivateVariables.map_list.at(map_name.at(index));
        if(currentTempMap->border.bottom.map!=NULL && currentTempMap->border.bottom.map->border.top.map!=currentTempMap)
        {
            if(currentTempMap->border.bottom.map->border.top.map==NULL)
                std::cerr << "the map "
                          << currentTempMap->map_file
                          << "have bottom map: "
                          << currentTempMap->border.bottom.map->map_file
                          << ", but the bottom map have not a top map"
                          << std::endl;
            else
                std::cerr << "the map "
                          << currentTempMap->map_file
                          << "have bottom map: "
                          << currentTempMap->border.bottom.map->map_file
                          << ", but the bottom map have different top map: "
                          << currentTempMap->border.bottom.map->border.top.map->map_file
                          << std::endl;
            GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.bottom.map=NULL;
        }
        if(currentTempMap->border.top.map!=NULL && currentTempMap->border.top.map->border.bottom.map!=currentTempMap)
        {
            if(currentTempMap->border.top.map->border.bottom.map==NULL)
                std::cerr << "the map "
                          << currentTempMap->map_file
                          << "have top map: "
                          << currentTempMap->border.top.map->map_file
                          << ", but the bottom map have not a bottom map"
                          << std::endl;
            else
                std::cerr << "the map "
                          << currentTempMap->map_file
                          << "have top map: "
                          << currentTempMap->border.top.map->map_file
                          << ", but the bottom map have different bottom map: "
                          << currentTempMap->border.top.map->border.bottom.map->map_file
                          << std::endl;
            GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.top.map=NULL;
        }
        if(currentTempMap->border.left.map!=NULL && currentTempMap->border.left.map->border.right.map!=currentTempMap)
        {
            if(currentTempMap->border.left.map->border.right.map==NULL)
                std::cerr << "the map "
                          << currentTempMap->map_file
                          << "have left map: "
                          << currentTempMap->border.left.map->map_file
                          << ", but the right map have not a right map"
                          << std::endl;
            else
                std::cerr << "the map "
                          << currentTempMap->map_file
                          << "have left map: "
                          << currentTempMap->border.left.map->map_file
                          << ", but the right map have different right map: "
                          << currentTempMap->border.left.map->border.right.map->map_file
                          << std::endl;
            GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.left.map=NULL;
        }
        if(currentTempMap->border.right.map!=NULL && currentTempMap->border.right.map->border.left.map!=currentTempMap)
        {
            if(currentTempMap->border.right.map->border.left.map==NULL)
                std::cerr << "the map "
                          << currentTempMap->map_file
                          << "have right map: "
                          << currentTempMap->border.right.map->map_file
                          << ", but the left map have not a left map"
                          << std::endl;
            else
                std::cerr << "the map "
                          << currentTempMap->map_file
                          << "have right map: "
                          << currentTempMap->border.right.map->map_file
                          << ", but the left map have different left map: "
                          << currentTempMap->border.right.map->border.left.map->map_file
                          << std::endl;
            GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.right.map=NULL;
        }
        index++;
    }

    //resolv the near map
    size=semi_loaded_map.size();
    index=0;
    while(index<size)
    {
        const auto &currentTempMap=GlobalServerData::serverPrivateVariables.map_list.at(map_name.at(index));
        if(currentTempMap->border.bottom.map!=NULL &&
                std::find(currentTempMap->near_map.begin(),currentTempMap->near_map.end(),currentTempMap->border.bottom.map)
                ==
                currentTempMap->near_map.end())
        {
            GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->near_map.push_back(currentTempMap->border.bottom.map);
            if(currentTempMap->border.left.map!=NULL && std::find(currentTempMap->near_map.begin(),currentTempMap->near_map.end(),currentTempMap->border.left.map)
                    ==
                    currentTempMap->near_map.end())
                GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->near_map.push_back(currentTempMap->border.left.map);
            if(currentTempMap->border.right.map!=NULL && std::find(currentTempMap->near_map.begin(),currentTempMap->near_map.end(),currentTempMap->border.right.map)
                    ==
                    currentTempMap->near_map.end())
                GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->near_map.push_back(currentTempMap->border.right.map);
        }

        if(currentTempMap->border.top.map!=NULL &&
                std::find(currentTempMap->near_map.begin(),currentTempMap->near_map.end(),currentTempMap->border.top.map)
                ==
                currentTempMap->near_map.end()
                )
        {
            GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->near_map.push_back(currentTempMap->border.top.map);
            if(currentTempMap->border.left.map!=NULL &&  std::find(currentTempMap->near_map.begin(),currentTempMap->near_map.end(),currentTempMap->border.left.map)
                    ==
                    currentTempMap->near_map.end())
                GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->near_map.push_back(currentTempMap->border.left.map);
            if(currentTempMap->border.right.map!=NULL &&  std::find(currentTempMap->near_map.begin(),currentTempMap->near_map.end(),currentTempMap->border.right.map)
                    ==
                    currentTempMap->near_map.end())
                GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->near_map.push_back(currentTempMap->border.right.map);
        }

        if(currentTempMap->border.right.map!=NULL &&
                std::find(currentTempMap->near_map.begin(),currentTempMap->near_map.end(),currentTempMap->border.right.map)
                ==
                currentTempMap->near_map.end()
                )
        {
            GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->near_map.push_back(currentTempMap->border.right.map);
            if(currentTempMap->border.top.map!=NULL &&  std::find(currentTempMap->near_map.begin(),currentTempMap->near_map.end(),currentTempMap->border.top.map)
                    ==
                    currentTempMap->near_map.end())
                GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->near_map.push_back(currentTempMap->border.top.map);
            if(currentTempMap->border.bottom.map!=NULL &&  std::find(currentTempMap->near_map.begin(),currentTempMap->near_map.end(),currentTempMap->border.bottom.map)
                    ==
                    currentTempMap->near_map.end())
                GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->near_map.push_back(currentTempMap->border.bottom.map);
        }

        if(currentTempMap->border.left.map!=NULL &&
                std::find(currentTempMap->near_map.begin(),currentTempMap->near_map.end(),currentTempMap->border.left.map)
                ==
                currentTempMap->near_map.end()
                )
        {
            GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->near_map.push_back(currentTempMap->border.left.map);
            if(currentTempMap->border.top.map!=NULL &&  std::find(currentTempMap->near_map.begin(),currentTempMap->near_map.end(),currentTempMap->border.top.map)
                    ==
                    currentTempMap->near_map.end())
                GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->near_map.push_back(currentTempMap->border.top.map);
            if(currentTempMap->border.bottom.map!=NULL &&  std::find(currentTempMap->near_map.begin(),currentTempMap->near_map.end(),currentTempMap->border.bottom.map)
                    ==
                    currentTempMap->near_map.end())
                GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->near_map.push_back(currentTempMap->border.bottom.map);
        }

        GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border_map=GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->near_map;
        GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->near_map.push_back(currentTempMap);
        index++;
    }

    //resolv the offset to change of map
    size=semi_loaded_map.size();
    index=0;
    while(index<size)
    {
        const auto &currentTempMap=GlobalServerData::serverPrivateVariables.map_list.at(map_name.at(index));
        if(currentTempMap->border.bottom.map!=NULL)
        {
            GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.bottom.x_offset=
                    semi_loaded_map.at(vectorindexOf(map_name,semi_loaded_map.at(index).border.bottom.fileName)).border.top.x_offset-
                    semi_loaded_map.at(index).border.bottom.x_offset;
        }
        else
            GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.bottom.x_offset=0;
        if(currentTempMap->border.top.map!=NULL)
        {
            GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.top.x_offset=
                    semi_loaded_map.at(vectorindexOf(map_name,semi_loaded_map.at(index).border.top.fileName)).border.bottom.x_offset-
                    semi_loaded_map.at(index).border.top.x_offset;
        }
        else
            GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.top.x_offset=0;
        if(currentTempMap->border.left.map!=NULL)
        {
            GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.left.y_offset=
                    semi_loaded_map.at(vectorindexOf(map_name,semi_loaded_map.at(index).border.left.fileName)).border.right.y_offset-
                    semi_loaded_map.at(index).border.left.y_offset;
        }
        else
            GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.left.y_offset=0;
        if(currentTempMap->border.right.map!=NULL)
        {
            GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.right.y_offset=
                    semi_loaded_map.at(vectorindexOf(map_name,semi_loaded_map.at(index).border.right.fileName)).border.left.y_offset-
                    semi_loaded_map.at(index).border.right.y_offset;
        }
        else
            GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.right.y_offset=0;
        index++;
    }

    //preload_map_semi_after_db_id load the item id

    //nead be after the offet
    preload_the_bots(semi_loaded_map);

    //load the rescue
    size=semi_loaded_map.size();
    index=0;
    while(index<size)
    {
        sub_index=0;
        while(sub_index<semi_loaded_map.at(index).old_map.rescue_points.size())
        {
            const Map_to_send::Map_Point &point=semi_loaded_map.at(index).old_map.rescue_points.at(sub_index);
            std::pair<uint8_t,uint8_t> coord;
            coord.first=point.x;
            coord.second=point.y;
            static_cast<MapServer *>(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)])->rescue[coord]=Orientation_bottom;
            sub_index++;
        }
        index++;
    }

    size=map_name_to_do_id.size();
    index=0;
    while(index<size)
    {
        if(GlobalServerData::serverPrivateVariables.map_list.find(map_name_to_do_id.at(index))!=GlobalServerData::serverPrivateVariables.map_list.end())
        {
            GlobalServerData::serverPrivateVariables.map_list[map_name_to_do_id.at(index)]->id=index;
            GlobalServerData::serverPrivateVariables.id_map_to_map[GlobalServerData::serverPrivateVariables.map_list[map_name_to_do_id.at(index)]->id]=map_name_to_do_id.at(index);
        }
        else
            abort();
        index++;
    }

    std::cout << GlobalServerData::serverPrivateVariables.map_list.size() << " map(s) loaded" << std::endl;

    DictionaryServer::dictionary_pointOnMap_internal_to_database.clear();
    #ifdef EPOLLCATCHCHALLENGERSERVER
    {
        unsigned int index=0;
        while(index<toDeleteAfterBotLoad.size())
        {
            delete toDeleteAfterBotLoad.at(index);
            index++;
        }
        toDeleteAfterBotLoad.clear();
    }
    #endif
    botFiles.clear();
    return true;
}

void BaseServer::preload_the_skin()
{
    std::vector<std::string> skinFolderList=FacilityLibGeneral::skinIdList(GlobalServerData::serverSettings.datapack_basePath+DATAPACK_BASE_PATH_SKIN);
    int index=0;
    const int &listsize=skinFolderList.size();
    while(index<listsize)
    {
        GlobalServerData::serverPrivateVariables.skinList[skinFolderList.at(index)]=index;
        index++;
    }

    std::cout << listsize << " skin(s) loaded" << std::endl;
}

void BaseServer::preload_the_datapack()
{
    #ifndef CATCHCHALLENGER_SERVER_DATAPACK_ONLYBYMIRROR
    std::vector<std::string> extensionAllowedTemp=stringsplit(std::string(CATCHCHALLENGER_EXTENSION_ALLOWED+CACHEDSTRING_dotcoma+CATCHCHALLENGER_EXTENSION_COMPRESSED),';');
    BaseServerMasterSendDatapack::extensionAllowed=std::unordered_set<std::string>(extensionAllowedTemp.begin(),extensionAllowedTemp.end());
    #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
    std::vector<std::string> compressedExtensionAllowedTemp=stringsplit(std::string(CATCHCHALLENGER_EXTENSION_COMPRESSED),';');
    BaseServerMasterSendDatapack::compressedExtension=std::unordered_set<std::string>(compressedExtensionAllowedTemp.begin(),compressedExtensionAllowedTemp.end());
    #endif
    Client::datapack_list_cache_timestamp_base=0;
    Client::datapack_list_cache_timestamp_main=0;
    Client::datapack_list_cache_timestamp_sub=0;
    #endif

    GlobalServerData::serverPrivateVariables.mainDatapackFolder=
            GlobalServerData::serverSettings.datapack_basePath+
            "map/main/"+
            CommonSettingsServer::commonSettingsServer.mainDatapackCode+
            "/";
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    {
        if(CommonSettingsServer::commonSettingsServer.mainDatapackCode.size()==0)
        {
            std::cerr << "CommonSettingsServer::commonSettingsServer.mainDatapackCode.isEmpty" << std::endl;
            abort();
        }
        if(!regex_search(CommonSettingsServer::commonSettingsServer.mainDatapackCode,std::regex(CATCHCHALLENGER_CHECK_MAINDATAPACKCODE)))
        {
            std::cerr << "CommonSettingsServer::commonSettingsServer.mainDatapackCode not match CATCHCHALLENGER_CHECK_MAINDATAPACKCODE "
                      << CommonSettingsServer::commonSettingsServer.mainDatapackCode
                      << " not contain "
                      << CATCHCHALLENGER_CHECK_MAINDATAPACKCODE;
            abort();
        }
        if(!FacilityLibGeneral::isDir(GlobalServerData::serverPrivateVariables.mainDatapackFolder))
        {
            std::cerr << GlobalServerData::serverPrivateVariables.mainDatapackFolder << " don't exists" << std::endl;
            abort();
        }
    }
    #endif
    if(CommonSettingsServer::commonSettingsServer.subDatapackCode.size()>0)
    {
        GlobalServerData::serverPrivateVariables.subDatapackFolder=
                GlobalServerData::serverSettings.datapack_basePath+
                "map/main/"+
                CommonSettingsServer::commonSettingsServer.mainDatapackCode+
                "/sub/"+
                CommonSettingsServer::commonSettingsServer.subDatapackCode+
                "/";
        if(!FacilityLibGeneral::isDir(GlobalServerData::serverPrivateVariables.subDatapackFolder))
        {
            std::cerr << GlobalServerData::serverPrivateVariables.subDatapackFolder << " don't exists, drop spec" << std::endl;
            GlobalServerData::serverPrivateVariables.subDatapackFolder.clear();
            CommonSettingsServer::commonSettingsServer.subDatapackCode.clear();
        }
    }

    #ifdef CATCHCHALLENGER_SERVER_DATAPACK_ONLYBYMIRROR
    if(CommonSettingsServer::commonSettingsServer.httpDatapackMirrorServer.empty() || CommonSettingsCommon::commonSettingsCommon.httpDatapackMirrorBase.empty())
    {
        std::cerr << "CATCHCHALLENGER_SERVER_DATAPACK_ONLYBYMIRROR, only send datapack by mirror is supported" << std::endl;
        abort();
    }
    #endif

    //do the base
    {
        SHA256_CTX hashBase;
        if(SHA224_Init(&hashBase)!=1)
        {
            std::cerr << "SHA224_Init(&hashBase)!=1" << std::endl;
            abort();
        }
        const std::unordered_map<std::string,Client::DatapackCacheFile> &pair=Client::datapack_file_list(GlobalServerData::serverSettings.datapack_basePath,"map/main/",false);
        if(pair.size()==0)
        {
            std::cout << "Client::datapack_file_list(GlobalServerData::serverSettings.datapack_basePath,\"map/main/\",false) empty (abort)" << std::endl;
            abort();
        }
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        {
            auto i=pair.begin();
            while(i!=pair.cend())
            {
                if(i->first.find("map/main/")!=std::string::npos)
                {
                    std::cerr << "map/main/ found into: " << i->first << " (abort)" << std::endl;
                    abort();
                }
                ++i;
            }
        }
        #endif
        std::vector<std::string> datapack_file_temp=unordered_map_keys_vector(pair);
        std::sort(datapack_file_temp.begin(), datapack_file_temp.end());
        const std::regex mainDatapackBaseFilter("^map[/\\\\]main[/\\\\]");
        unsigned int index=0;
        while(index<datapack_file_temp.size()) {
            if(regex_search(datapack_file_temp.at(index),GlobalServerData::serverPrivateVariables.datapack_rightFileName))
            {
                FILE *filedesc = fopen((GlobalServerData::serverSettings.datapack_basePath+datapack_file_temp.at(index)).c_str(), "rb");
                if(filedesc!=NULL)
                {
                    //read and load the file
                    const std::vector<char> &data=FacilityLibGeneral::readAllFileAndClose(filedesc);

                    #ifndef CATCHCHALLENGER_SERVER_DATAPACK_ONLYBYMIRROR
                    if((1+datapack_file_temp.at(index).size()+4+data.size())>=CATCHCHALLENGER_MAX_PACKET_SIZE)
                    {
                        if(GlobalServerData::serverSettings.max_players>1)//if not internal
                        {
                            if(BaseServerMasterSendDatapack::compressedExtension.find(FacilityLibGeneral::getSuffix(datapack_file_temp.at(index)))!=BaseServerMasterSendDatapack::compressedExtension.end())
                            {
                                if(ProtocolParsing::compressionTypeServer==ProtocolParsing::CompressionType::None)
                                {
                                    std::cerr << "The file " << GlobalServerData::serverSettings.datapack_basePath << datapack_file_temp.at(index) << " is over the maximum packet size, but can be compressed, try enable the compression" << std::endl;
                                    abort();
                                }
                            }
                            else
                            {
                                std::cerr << "The file " << GlobalServerData::serverSettings.datapack_basePath << datapack_file_temp.at(index) << " is over the maximum packet size" << std::endl;
                                abort();
                            }
                        }
                    }
                    #endif

                    //switch the data to correct hash or drop it
                    if(regex_search(datapack_file_temp.at(index),mainDatapackBaseFilter))
                    {}
                    else
                    {
                        #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
                        #ifndef CATCHCHALLENGER_SERVER_DATAPACK_ONLYBYMIRROR
                        SHA256_CTX hashFile;
                        if(SHA224_Init(&hashFile)!=1)
                        {
                            std::cerr << "SHA224_Init(&hashBase)!=1" << std::endl;
                            abort();
                        }
                        SHA224_Update(&hashFile,data.data(),data.size());
                        Client::DatapackCacheFile cacheFile;
                        SHA224_Final(reinterpret_cast<unsigned char *>(ProtocolParsingBase::tempBigBufferForOutput),&hashFile);
                        cacheFile.partialHash=*reinterpret_cast<const uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput);
                        Client::datapack_file_hash_cache_base[datapack_file_temp.at(index)]=cacheFile;
                        #endif
                        #endif

                        SHA224_Update(&hashBase,data.data(),data.size());
                    }
                }
                else
                {
                    std::cerr << "Stop now! Unable to open the file " << GlobalServerData::serverSettings.datapack_basePath << datapack_file_temp.at(index) << " to do the datapack checksum for the mirror" << std::endl;
                    abort();
                }
            }
            else
                std::cerr << "File excluded because don't match the regex (1): " << GlobalServerData::serverSettings.datapack_basePath << datapack_file_temp.at(index) << std::endl;
            index++;
        }
        if(CommonSettingsCommon::commonSettingsCommon.datapackHashBase.size()==CATCHCHALLENGER_SHA224HASH_SIZE)
        {
            SHA224_Final(reinterpret_cast<unsigned char *>(ProtocolParsingBase::tempBigBufferForOutput),&hashBase);
            if(memcmp(CommonSettingsCommon::commonSettingsCommon.datapackHashBase.data(),ProtocolParsingBase::tempBigBufferForOutput,CATCHCHALLENGER_SHA224HASH_SIZE)!=0)
            {
                std::cerr << "datapackHashBase sha224 sum not match master "
                          << binarytoHexa(CommonSettingsCommon::commonSettingsCommon.datapackHashBase)
                          << " != master "
                          << binarytoHexa(ProtocolParsingBase::tempBigBufferForOutput,CATCHCHALLENGER_SHA224HASH_SIZE)
                          << " (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                abort();
            }
        }
        else
        {
            CommonSettingsCommon::commonSettingsCommon.datapackHashBase.resize(CATCHCHALLENGER_SHA224HASH_SIZE);
            SHA224_Final(reinterpret_cast<unsigned char *>(CommonSettingsCommon::commonSettingsCommon.datapackHashBase.data()),&hashBase);
        }
    }
    /// \todo check if big file is compressible under 1MB

    //do the main
    {
        SHA256_CTX hashMain;
        if(SHA224_Init(&hashMain)!=1)
        {
            std::cerr << "SHA224_Init(&hashBase)!=1" << std::endl;
            abort();
        }
        const std::unordered_map<std::string,Client::DatapackCacheFile> &pair=Client::datapack_file_list(GlobalServerData::serverPrivateVariables.mainDatapackFolder,"sub/",false);
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        {
            auto i=pair.begin();
            while(i!=pair.cend())
            {
                if(i->first.find("sub/")!=std::string::npos)
                {
                    std::cerr << "sub/ found into: " << i->first << " (abort)" << std::endl;
                    abort();
                }
                ++i;
            }
        }
        #endif
        std::vector<std::string> datapack_file_temp=unordered_map_keys_vector(pair);
        std::sort(datapack_file_temp.begin(), datapack_file_temp.end());
        const std::regex mainDatapackFolderFilter("^sub[/\\\\]");
        unsigned int index=0;
        while(index<datapack_file_temp.size()) {
            if(regex_search(datapack_file_temp.at(index),GlobalServerData::serverPrivateVariables.datapack_rightFileName))
            {
                FILE *filedesc = fopen((GlobalServerData::serverPrivateVariables.mainDatapackFolder+datapack_file_temp.at(index)).c_str(), "rb");
                if(filedesc!=NULL)
                {
                    //read and load the file
                    const std::vector<char> &data=FacilityLibGeneral::readAllFileAndClose(filedesc);

                    #ifndef CATCHCHALLENGER_SERVER_DATAPACK_ONLYBYMIRROR
                    if((1+datapack_file_temp.at(index).size()+4+data.size())>=CATCHCHALLENGER_MAX_PACKET_SIZE)
                    {
                        if(GlobalServerData::serverSettings.max_players>1)//if not internal
                        {
                            #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
                            if(BaseServerMasterSendDatapack::compressedExtension.find(FacilityLibGeneral::getSuffix(datapack_file_temp.at(index)))!=BaseServerMasterSendDatapack::compressedExtension.end())
                            {
                                if(ProtocolParsing::compressionTypeServer==ProtocolParsing::CompressionType::None)
                                {
                                    std::cerr << "The file " << GlobalServerData::serverSettings.datapack_basePath << datapack_file_temp.at(index) << " is over the maximum packet size, but can be compressed, try enable the compression" << std::endl;
                                    abort();
                                }
                            }
                            else
                            #endif
                            {
                                std::cerr << "The file " << GlobalServerData::serverSettings.datapack_basePath << datapack_file_temp.at(index) << " is over the maximum packet size" << std::endl;
                                abort();
                            }
                        }
                    }
                    #endif

                    //switch the data to correct hash or drop it
                    if(regex_search(datapack_file_temp.at(index),mainDatapackFolderFilter))
                    {
                    }
                    else
                    {
                        #ifndef CATCHCHALLENGER_SERVER_DATAPACK_ONLYBYMIRROR
                        SHA256_CTX hashFile;
                        if(SHA224_Init(&hashFile)!=1)
                        {
                            std::cerr << "SHA224_Init(&hashBase)!=1" << std::endl;
                            abort();
                        }
                        SHA224_Update(&hashFile,data.data(),data.size());
                        Client::DatapackCacheFile cacheFile;
                        SHA224_Final(reinterpret_cast<unsigned char *>(ProtocolParsingBase::tempBigBufferForOutput),&hashFile);
                        cacheFile.partialHash=*reinterpret_cast<const uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput);
                        Client::datapack_file_hash_cache_main[datapack_file_temp.at(index)]=cacheFile;
                        #endif

                        SHA224_Update(&hashMain,data.data(),data.size());
                    }
                }
                else
                {
                    std::cerr << "Stop now! Unable to open the file " << GlobalServerData::serverSettings.datapack_basePath << datapack_file_temp.at(index) << " to do the datapack checksum for the mirror" << std::endl;
                    abort();
                }
            }
            else
                std::cerr << "File excluded because don't match the regex (2): " << GlobalServerData::serverSettings.datapack_basePath << datapack_file_temp.at(index) << std::endl;
            index++;
        }
        CommonSettingsServer::commonSettingsServer.datapackHashServerMain.resize(CATCHCHALLENGER_SHA224HASH_SIZE);
        SHA224_Final(reinterpret_cast<unsigned char *>(CommonSettingsServer::commonSettingsServer.datapackHashServerMain.data()),&hashMain);
    }
    //do the sub
    if(GlobalServerData::serverPrivateVariables.subDatapackFolder.size()>0)
    {
        SHA256_CTX hashSub;
        if(SHA224_Init(&hashSub)!=1)
        {
            std::cerr << "SHA224_Init(&hashBase)!=1" << std::endl;
            abort();
        }
        const std::unordered_map<std::string,Client::DatapackCacheFile> &pair=Client::datapack_file_list(GlobalServerData::serverPrivateVariables.subDatapackFolder,"",false);
        std::vector<std::string> datapack_file_temp=unordered_map_keys_vector(pair);
        std::sort(datapack_file_temp.begin(), datapack_file_temp.end());
        unsigned int index=0;
        while(index<datapack_file_temp.size()) {
            if(regex_search(datapack_file_temp.at(index),GlobalServerData::serverPrivateVariables.datapack_rightFileName))
            {
                FILE *filedesc = fopen((GlobalServerData::serverPrivateVariables.subDatapackFolder+datapack_file_temp.at(index)).c_str(), "rb");
                if(filedesc!=NULL)
                {
                    //read and load the file
                    const std::vector<char> &data=FacilityLibGeneral::readAllFileAndClose(filedesc);

                    #ifndef CATCHCHALLENGER_SERVER_DATAPACK_ONLYBYMIRROR
                    if((1+datapack_file_temp.at(index).size()+4+data.size())>=CATCHCHALLENGER_MAX_PACKET_SIZE)
                    {
                        if(GlobalServerData::serverSettings.max_players>1)//if not internal
                        {
                            #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
                            if(BaseServerMasterSendDatapack::compressedExtension.find(FacilityLibGeneral::getSuffix(datapack_file_temp.at(index)))!=BaseServerMasterSendDatapack::compressedExtension.end())
                            {
                                if(ProtocolParsing::compressionTypeServer==ProtocolParsing::CompressionType::None)
                                {
                                    std::cerr << "The file " << GlobalServerData::serverSettings.datapack_basePath << datapack_file_temp.at(index) << " is over the maximum packet size, but can be compressed, try enable the compression" << std::endl;
                                    abort();
                                }
                            }
                            else
                            #endif
                            {
                                std::cerr << "The file " << GlobalServerData::serverSettings.datapack_basePath << datapack_file_temp.at(index) << " is over the maximum packet size" << std::endl;
                                abort();
                            }
                        }
                    }
                    #endif

                    #ifndef CATCHCHALLENGER_SERVER_DATAPACK_ONLYBYMIRROR
                    //switch the data to correct hash or drop it
                    SHA256_CTX hashFile;
                    if(SHA224_Init(&hashFile)!=1)
                    {
                        std::cerr << "SHA224_Init(&hashBase)!=1" << std::endl;
                        abort();
                    }
                    SHA224_Update(&hashFile,data.data(),data.size());
                    Client::DatapackCacheFile cacheFile;
                    SHA224_Final(reinterpret_cast<unsigned char *>(ProtocolParsingBase::tempBigBufferForOutput),&hashFile);
                    cacheFile.partialHash=*reinterpret_cast<const uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput);
                    Client::datapack_file_hash_cache_sub[datapack_file_temp.at(index)]=cacheFile;
                    #endif

                    SHA224_Update(&hashSub,data.data(),data.size());
                }
                else
                {
                    std::cerr << "Stop now! Unable to open the file " << GlobalServerData::serverSettings.datapack_basePath << datapack_file_temp.at(index) << " to do the datapack checksum for the mirror" << std::endl;
                    abort();
                }
            }
            else
                std::cerr << "File excluded because don't match the regex (3): " << GlobalServerData::serverSettings.datapack_basePath << datapack_file_temp.at(index) << std::endl;
            index++;
        }
        CommonSettingsServer::commonSettingsServer.datapackHashServerSub.resize(CATCHCHALLENGER_SHA224HASH_SIZE);
        SHA224_Final(reinterpret_cast<unsigned char *>(CommonSettingsServer::commonSettingsServer.datapackHashServerSub.data()),&hashSub);
    }

    #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    #ifndef CATCHCHALLENGER_SERVER_DATAPACK_ONLYBYMIRROR
    if(Client::datapack_file_hash_cache_base.size()==0)
    {
        std::cout << "0 file for datapack loaded base (abort)" << std::endl;
        abort();
    }
    #endif
    #endif
    #ifndef CATCHCHALLENGER_SERVER_DATAPACK_ONLYBYMIRROR
    if(Client::datapack_file_hash_cache_main.size()==0)
    {
        std::cout << "0 file for datapack loaded main (abort)" << std::endl;
        abort();
    }

    std::cout << Client::datapack_file_hash_cache_base.size()
              << " file for datapack loaded base, "
              << Client::datapack_file_hash_cache_main.size()
              << " file for datapack loaded main, "
              << Client::datapack_file_hash_cache_sub.size()
              << " file for datapack loaded sub" << std::endl;
    #endif
    std::cout << binarytoHexa(CommonSettingsCommon::commonSettingsCommon.datapackHashBase)
              << " hash for datapack loaded base, "
              << binarytoHexa(CommonSettingsServer::commonSettingsServer.datapackHashServerMain)
              << " hash for datapack loaded main, "
              << binarytoHexa(CommonSettingsServer::commonSettingsServer.datapackHashServerSub)
              << " hash for datapack loaded sub" << std::endl;
}

void BaseServer::preload_the_players()
{
    Client::simplifiedIdList.clear();
    Client::marketObjectIdList.reserve(GlobalServerData::serverSettings.max_players);
    switch(GlobalServerData::serverSettings.mapVisibility.mapVisibilityAlgorithm)
    {
        default:
        case MapVisibilityAlgorithmSelection_Simple:
        case MapVisibilityAlgorithmSelection_WithBorder:
        {
            uint16_t index=0;
            while(index<GlobalServerData::serverSettings.max_players)
            {
                Client::simplifiedIdList.push_back(index);
                index++;
            }
            std::random_shuffle(Client::simplifiedIdList.begin(),Client::simplifiedIdList.end());
        }
        break;
        case MapVisibilityAlgorithmSelection_None:
        break;
    }
}

void BaseServer::preload_the_visibility_algorithm()
{
    switch(GlobalServerData::serverSettings.mapVisibility.mapVisibilityAlgorithm)
    {
        case MapVisibilityAlgorithmSelection_Simple:
        case MapVisibilityAlgorithmSelection_WithBorder:
        if(GlobalServerData::serverPrivateVariables.timer_to_send_insert_move_remove!=NULL)
        {
            #ifndef EPOLLCATCHCHALLENGERSERVER
            GlobalServerData::serverPrivateVariables.timer_to_send_insert_move_remove->stop();
            GlobalServerData::serverPrivateVariables.timer_to_send_insert_move_remove->deleteLater();
            #else
            delete GlobalServerData::serverPrivateVariables.timer_to_send_insert_move_remove;
            #endif
        }
        #ifndef EPOLLCATCHCHALLENGERSERVER
        GlobalServerData::serverPrivateVariables.timer_to_send_insert_move_remove=new QTimer();
        #else
        GlobalServerData::serverPrivateVariables.timer_to_send_insert_move_remove=new TimerSendInsertMoveRemove();
        #endif
        GlobalServerData::serverPrivateVariables.timer_to_send_insert_move_remove->start(CATCHCHALLENGER_SERVER_MAP_TIME_TO_SEND_MOVEMENT);
        break;
        case MapVisibilityAlgorithmSelection_None:
        default:
        break;
    }

    switch(GlobalServerData::serverSettings.mapVisibility.mapVisibilityAlgorithm)
    {
        case MapVisibilityAlgorithmSelection_Simple:
            std::cout << "Visibility: MapVisibilityAlgorithmSelection_Simple" << std::endl;
        break;
        case MapVisibilityAlgorithmSelection_WithBorder:
            std::cout << "Visibility: MapVisibilityAlgorithmSelection_WithBorder" << std::endl;
        break;
        case MapVisibilityAlgorithmSelection_None:
            std::cout << "Visibility: MapVisibilityAlgorithmSelection_None" << std::endl;
        break;
    }
}

void BaseServer::preload_the_bots(const std::vector<Map_semi> &semi_loaded_map)
{
    int shops_number=0;
    int bots_number=0;
    int learnpoint_number=0;
    int healpoint_number=0;
    int marketpoint_number=0;
    int zonecapturepoint_number=0;
    int botfights_number=0;
    int botfightstigger_number=0;
    //resolv the botfights, bots, shops, learn, heal, zonecapture, market
    const int &size=semi_loaded_map.size();
    int index=0;
    bool ok;
    while(index<size)
    {
        int sub_index=0;
        const int &botssize=semi_loaded_map.at(index).old_map.bots.size();
        while(sub_index<botssize)
        {
            bots_number++;
            Map_to_send::Bot_Semi bot_Semi=semi_loaded_map.at(index).old_map.bots.at(sub_index);
            if(!stringEndsWith(bot_Semi.file,CACHEDSTRING_dotxml))
                bot_Semi.file+=CACHEDSTRING_dotxml;
            loadBotFile(semi_loaded_map.at(index).map->map_file,bot_Semi.file);
            if(botFiles.find(bot_Semi.file)!=botFiles.end())
                if(botFiles.at(bot_Semi.file).find(bot_Semi.id)!=botFiles.at(bot_Semi.file).end())
                {
                    const auto &step_list=botFiles.at(bot_Semi.file).at(bot_Semi.id).step;
                    #ifdef DEBUG_MESSAGE_MAP_LOAD
                    std::cout << "Bot "
                              << bot_Semi.id
                              << " ("
                              << bot_Semi.file
                              << "), spawn at: "
                              << semi_loaded_map.at(index).map->map_file
                              << " ("
                              << std::to_string(bot_Semi.point.x)
                              << ","
                              << std::to_string(bot_Semi.point.y)
                              << ")"
                              << std::endl;
                    #endif
                    auto i=step_list.begin();
                    while (i!=step_list.end())
                    {
                        const CATCHCHALLENGER_XMLELEMENT * step = i->second;
                        if(step==NULL)
                        {
                            ++i;
                            continue;
                        }
                        std::pair<uint8_t,uint8_t> pairpoint(bot_Semi.point.x,bot_Semi.point.y);
                        MapServer * const mapServer=static_cast<MapServer *>(semi_loaded_map.at(index).map);
                        if(step->Attribute(XMLCACHEDSTRING_type)==NULL)
                        {
                            ++i;
                            continue;
                        }
                        const std::string step_type=CATCHCHALLENGER_XMLATTRIBUTETOSTRING(step->Attribute(XMLCACHEDSTRING_type));
                        if(step_type==CACHEDSTRING_shop)
                        {
                            if(step->Attribute(XMLCACHEDSTRING_shop)==NULL)
                                std::cerr << "Has not attribute \"shop\": for bot id: "
                                          << bot_Semi.id
                                          << " ("
                                          << bot_Semi.file
                                          << "), spawn at: "
                                          << semi_loaded_map.at(index).map->map_file
                                          << " ("
                                          << std::to_string(bot_Semi.point.x)
                                          << ","
                                          << std::to_string(bot_Semi.point.y)
                                          << "), for step: "
                                          << std::to_string(i->first)
                                          << std::endl;
                            else
                            {
                                uint32_t shop=stringtouint32(CATCHCHALLENGER_XMLATTRIBUTETOSTRING(step->Attribute(XMLCACHEDSTRING_shop)),&ok);
                                if(!ok)
                                    std::cerr << "shop is not a number: for bot id: "
                                              << bot_Semi.id
                                              << " ("
                                              << bot_Semi.file
                                              << "), spawn at: "
                                              << semi_loaded_map.at(index).map->map_file
                                              << " ("
                                              << std::to_string(bot_Semi.point.x)
                                              << ","
                                              << std::to_string(bot_Semi.point.y)
                                              << "), for step: "
                                              << std::to_string(i->first)
                                              << std::endl;
                                else if(CommonDatapackServerSpec::commonDatapackServerSpec.shops.find(shop)==CommonDatapackServerSpec::commonDatapackServerSpec.shops.end())
                                        std::cerr << "shop number is not valid shop: for bot id: "
                                                  << bot_Semi.id
                                                  << " ("
                                                  << bot_Semi.file
                                                  << "), spawn at: "
                                                  << semi_loaded_map.at(index).map->map_file
                                                  << " ("
                                                  << std::to_string(bot_Semi.point.x)
                                                  << ","
                                                  << std::to_string(bot_Semi.point.y)
                                                  << "), for step: "
                                                  << std::to_string(i->first)
                                                  << std::endl;
                                else
                                {
                                    #ifdef DEBUG_MESSAGE_MAP_LOAD
                                    std::cout << "shop put at: for bot id: "
                                              << bot_Semi.id
                                              << " ("
                                              << bot_Semi.file
                                              << "), spawn at: "
                                              << semi_loaded_map.at(index).map->map_file
                                              << " ("
                                              << std::to_string(bot_Semi.point.x)
                                              << ","
                                              << std::to_string(bot_Semi.point.y)
                                              << "), for step: "
                                              << std::to_string(i->first)
                                              << std::endl;
                                    #endif
                                    mapServer->shops[pairpoint].push_back(shop);
                                    shops_number++;
                                }
                            }
                        }
                        else if(step_type==CACHEDSTRING_learn)
                        {
                            if(mapServer->learn.find(pairpoint)!=mapServer->learn.end())
                                std::cerr << "learn point already on the map: for bot id: "
                                          << bot_Semi.id
                                          << " ("
                                          << bot_Semi.file
                                          << "), spawn at: "
                                          << semi_loaded_map.at(index).map->map_file
                                          << " ("
                                          << std::to_string(bot_Semi.point.x)
                                          << ","
                                          << std::to_string(bot_Semi.point.y)
                                          << "), for step: "
                                          << std::to_string(i->first)
                                          << std::endl;
                            else
                            {
                                #ifdef DEBUG_MESSAGE_MAP_LOAD
                                    std::cout << "learn point for bot id: "
                                              << bot_Semi.id
                                              << " ("
                                              << bot_Semi.file
                                              << "), spawn at: "
                                              << semi_loaded_map.at(index).map->map_file
                                              << " ("
                                              << std::to_string(bot_Semi.point.x)
                                              << ","
                                              << std::to_string(bot_Semi.point.y)
                                              << "), for step: "
                                              << std::to_string(i->first)
                                              << std::endl;
                                #endif
                                mapServer->learn.insert(pairpoint);
                                learnpoint_number++;
                            }
                        }
                        else if(step_type==CACHEDSTRING_heal)
                        {
                            if(mapServer->heal.find(pairpoint)!=mapServer->heal.end())
                                std::cerr << "heal point already on the map: for bot id: "
                                          << bot_Semi.id
                                          << " ("
                                          << bot_Semi.file
                                          << "), spawn at: "
                                          << semi_loaded_map.at(index).map->map_file
                                          << " ("
                                          << std::to_string(bot_Semi.point.x)
                                          << ","
                                          << std::to_string(bot_Semi.point.y)
                                          << "), for step: "
                                          << std::to_string(i->first)
                                          << std::endl;
                            else
                            {
                                #ifdef DEBUG_MESSAGE_MAP_LOAD
                                    std::cout << "heal point put at: for bot id: "
                                              << bot_Semi.id
                                              << " ("
                                              << bot_Semi.file
                                              << "), spawn at: "
                                              << semi_loaded_map.at(index).map->map_file
                                              << " ("
                                              << std::to_string(bot_Semi.point.x)
                                              << ","
                                              << std::to_string(bot_Semi.point.y)
                                              << "), for step: "
                                              << std::to_string(i->first)
                                              << std::endl;
                                #endif
                                mapServer->heal.insert(pairpoint);
                                healpoint_number++;
                            }
                        }
                        else if(step_type==CACHEDSTRING_market)
                        {
                            if(mapServer->market.find(pairpoint)!=mapServer->market.end())
                                std::cerr << "market point already on the map: for bot id: "
                                          << bot_Semi.id
                                          << " ("
                                          << bot_Semi.file
                                          << "), spawn at: "
                                          << semi_loaded_map.at(index).map->map_file
                                          << " ("
                                          << std::to_string(bot_Semi.point.x)
                                          << ","
                                          << std::to_string(bot_Semi.point.y)
                                          << "), for step: "
                                          << std::to_string(i->first)
                                          << std::endl;
                            else
                            {
                                #ifdef DEBUG_MESSAGE_MAP_LOAD
                                    std::cout << "market point put at: for bot id: "
                                              << bot_Semi.id
                                              << " ("
                                              << bot_Semi.file
                                              << "), spawn at: "
                                              << semi_loaded_map.at(index).map->map_file
                                              << " ("
                                              << std::to_string(bot_Semi.point.x)
                                              << ","
                                              << std::to_string(bot_Semi.point.y)
                                              << "), for step: "
                                              << std::to_string(i->first)
                                              << std::endl;
                                #endif
                                mapServer->market.insert(pairpoint);
                                marketpoint_number++;
                            }
                        }
                        else if(step_type==CACHEDSTRING_zonecapture)
                        {
                            if(step->Attribute(XMLCACHEDSTRING_zone)==NULL)
                                std::cerr << "zonecapture point have not the zone attribute: for bot id: "
                                          << bot_Semi.id
                                          << " ("
                                          << bot_Semi.file
                                          << "), spawn at: "
                                          << semi_loaded_map.at(index).map->map_file
                                          << " ("
                                          << std::to_string(bot_Semi.point.x)
                                          << ","
                                          << std::to_string(bot_Semi.point.y)
                                          << "), for step: "
                                          << std::to_string(i->first)
                                          << std::endl;
                            else if(mapServer->zonecapture.find(pairpoint)!=mapServer->zonecapture.end())
                                    std::cerr << "zonecapture point already on the map: for bot id: "
                                              << bot_Semi.id
                                              << " ("
                                              << bot_Semi.file
                                              << "), spawn at: "
                                              << semi_loaded_map.at(index).map->map_file
                                              << " ("
                                              << std::to_string(bot_Semi.point.x)
                                              << ","
                                              << std::to_string(bot_Semi.point.y)
                                              << "), for step: "
                                              << std::to_string(i->first)
                                              << std::endl;
                            else
                            {
                                #ifdef DEBUG_MESSAGE_MAP_LOAD
                                    std::cerr << "zonecapture point put at: for bot id: "
                                              << bot_Semi.id
                                              << " ("
                                              << bot_Semi.file
                                              << "), spawn at: "
                                              << semi_loaded_map.at(index).map->map_file
                                              << " ("
                                              << std::to_string(bot_Semi.point.x)
                                              << ","
                                              << std::to_string(bot_Semi.point.y)
                                              << "), for step: "
                                              << std::to_string(i->first)
                                              << std::endl;
                                #endif
                                mapServer->zonecapture[pairpoint]=CATCHCHALLENGER_XMLATTRIBUTETOSTRING(step->Attribute(XMLCACHEDSTRING_zone));
                                zonecapturepoint_number++;
                            }
                        }
                        else if(step_type==CACHEDSTRING_fight)
                        {
                            if(mapServer->botsFight.find(pairpoint)!=mapServer->botsFight.end())
                                std::cerr << "botsFight point already on the map: for bot id: "
                                          << bot_Semi.id
                                          << " ("
                                          << bot_Semi.file
                                          << "), spawn at: "
                                          << semi_loaded_map.at(index).map->map_file
                                          << " ("
                                          << std::to_string(bot_Semi.point.x)
                                          << ","
                                          << std::to_string(bot_Semi.point.y)
                                          << "), for step: "
                                          << std::to_string(i->first)
                                          << std::endl;
                            else
                            {
                                uint32_t fightid=0;
                                ok=false;
                                if(step->Attribute(XMLCACHEDSTRING_fightid)!=NULL)
                                    fightid=stringtouint32(CATCHCHALLENGER_XMLATTRIBUTETOSTRING(step->Attribute(XMLCACHEDSTRING_fightid)),&ok);
                                if(ok)
                                {
                                    if(CommonDatapackServerSpec::commonDatapackServerSpec.botFights.find(fightid)!=CommonDatapackServerSpec::commonDatapackServerSpec.botFights.end())
                                    {
                                        if(bot_Semi.property_text.find(CACHEDSTRING_lookAt)!=bot_Semi.property_text.end())
                                        {
                                            Direction direction;
                                            const std::string &lookAt=bot_Semi.property_text.at(CACHEDSTRING_lookAt);
                                            if(lookAt==CACHEDSTRING_left)
                                                direction=CatchChallenger::Direction_move_at_left;
                                            else if(lookAt==CACHEDSTRING_right)
                                                direction=CatchChallenger::Direction_move_at_right;
                                            else if(lookAt==CACHEDSTRING_top)
                                                direction=CatchChallenger::Direction_move_at_top;
                                            else
                                            {
                                                if(lookAt!=CACHEDSTRING_bottom)
                                                    std::cerr << "Wrong direction for the botp: for bot id: "
                                                              << bot_Semi.id
                                                              << " ("
                                                              << bot_Semi.file
                                                              << "), spawn at: "
                                                              << semi_loaded_map.at(index).map->map_file
                                                              << " ("
                                                              << std::to_string(bot_Semi.point.x)
                                                              << ","
                                                              << std::to_string(bot_Semi.point.y)
                                                              << "), for step: "
                                                              << std::to_string(i->first)
                                                              << std::endl;
                                                direction=CatchChallenger::Direction_move_at_bottom;
                                            }
                                            #ifdef DEBUG_MESSAGE_MAP_LOAD
                                            std::cout << "botsFight point put at: for bot id: "
                                                      << bot_Semi.id
                                                      << " ("
                                                      << bot_Semi.file
                                                      << "), spawn at: "
                                                      << semi_loaded_map.at(index).map->map_file
                                                      << " ("
                                                      << std::to_string(bot_Semi.point.x)
                                                      << ","
                                                      << std::to_string(bot_Semi.point.y)
                                                      << "), for step: "
                                                      << std::to_string(i->first)
                                                      << std::endl;
                                            #endif
                                            mapServer->botsFight[pairpoint].push_back(fightid);
                                            botfights_number++;

                                            uint8_t fightRange=5;
                                            if(bot_Semi.property_text.find(CACHEDSTRING_fightRange)!=bot_Semi.property_text.end())
                                            {
                                                fightRange=stringtouint8(bot_Semi.property_text.at(CACHEDSTRING_fightRange),&ok);
                                                if(!ok)
                                                {
                                                    std::cerr << "fightRange is not a number: for bot id: "
                                                              << bot_Semi.id
                                                              << " ("
                                                              << bot_Semi.file
                                                              << "), spawn at: "
                                                              << semi_loaded_map.at(index).map->map_file
                                                              << " ("
                                                              << std::to_string(bot_Semi.point.x)
                                                              << ","
                                                              << std::to_string(bot_Semi.point.y)
                                                              << "), for step: "
                                                              << std::to_string(i->first)
                                                              << std::endl;
                                                    fightRange=5;
                                                }
                                                else
                                                {
                                                    if(fightRange>10)
                                                    {
                                                        std::cerr << "fightRange is greater than 10: for bot id: "
                                                                  << bot_Semi.id
                                                                  << " ("
                                                                  << bot_Semi.file
                                                                  << "), spawn at: "
                                                                  << semi_loaded_map.at(index).map->map_file
                                                                  << " ("
                                                                  << std::to_string(bot_Semi.point.x)
                                                                  << ","
                                                                  << std::to_string(bot_Semi.point.y)
                                                                  << "), for step: "
                                                                  << std::to_string(i->first)
                                                                  << std::endl;
                                                        fightRange=5;
                                                    }
                                                }
                                            }
                                            //load the botsFightTrigger
                                            #ifdef DEBUG_MESSAGE_CLIENT_FIGHT_BOT
                                            std::cerr << "Put bot fight point: for bot id: "
                                                      << bot_Semi.id
                                                      << " ("
                                                      << bot_Semi.file
                                                      << "), spawn at: "
                                                      << semi_loaded_map.at(index).map->map_file
                                                      << " ("
                                                      << std::to_string(bot_Semi.point.x)
                                                      << ","
                                                      << std::to_string(bot_Semi.point.y)
                                                      << "), for step: "
                                                      << std::to_string(i->first)
                                                      << " in direction: "
                                                      << std::to_string(direction)
                                                      << std::endl;
                                            #endif
                                            uint8_t temp_x=bot_Semi.point.x,temp_y=bot_Semi.point.y;
                                            uint8_t index_botfight_range=0;
                                            CatchChallenger::CommonMap *map=semi_loaded_map.at(index).map;
                                            CatchChallenger::CommonMap *old_map=map;
                                            while(index_botfight_range<fightRange)
                                            {
                                                if(!CatchChallenger::MoveOnTheMap::canGoTo(direction,*map,temp_x,temp_y,true,false))
                                                    break;
                                                if(!CatchChallenger::MoveOnTheMap::move(direction,&map,&temp_x,&temp_y,true,false))
                                                    break;
                                                if(map!=old_map)
                                                    break;
                                                const std::pair<uint8_t,uint8_t> botFightRangePoint(temp_x,temp_y);
                                                mapServer->botsFightTrigger[botFightRangePoint].push_back(fightid);
                                                index_botfight_range++;
                                                botfightstigger_number++;
                                            }
                                        }
                                        else
                                            std::cerr << "lookAt not found: for bot id: "
                                                      << bot_Semi.id
                                                      << " ("
                                                      << bot_Semi.file
                                                      << "), spawn at: "
                                                      << semi_loaded_map.at(index).map->map_file
                                                      << " ("
                                                      << std::to_string(bot_Semi.point.x)
                                                      << ","
                                                      << std::to_string(bot_Semi.point.y)
                                                      << "), for step: "
                                                      << std::to_string(i->first)
                                                      << std::endl;
                                    }
                                    else
                                        std::cerr << "fightid not found into the list: for bot id: "
                                                  << bot_Semi.id
                                                  << " ("
                                                  << bot_Semi.file
                                                  << "), spawn at: "
                                                  << semi_loaded_map.at(index).map->map_file
                                                  << " ("
                                                  << std::to_string(bot_Semi.point.x)
                                                  << ","
                                                  << std::to_string(bot_Semi.point.y)
                                                  << "), for step: "
                                                  << std::to_string(i->first)
                                                  << std::endl;
                                }
                                else
                                    std::cerr << "botsFight point have wrong fightid: for bot id: "
                                              << bot_Semi.id
                                              << " ("
                                              << bot_Semi.file
                                              << "), spawn at: "
                                              << semi_loaded_map.at(index).map->map_file
                                              << " ("
                                              << std::to_string(bot_Semi.point.x)
                                              << ","
                                              << std::to_string(bot_Semi.point.y)
                                              << "), for step: "
                                              << std::to_string(i->first)
                                              << std::endl;
                            }
                        }
                        ++i;
                    }
                }
            sub_index++;
        }
        index++;
    }
    botIdLoaded.clear();

    std::cout << learnpoint_number << " learn point(s) on map loaded" << std::endl;
    std::cout << zonecapturepoint_number << " zonecapture point(s) on map loaded" << std::endl;
    std::cout << healpoint_number << " heal point(s) on map loaded" << std::endl;
    std::cout << marketpoint_number << " market point(s) on map loaded" << std::endl;
    std::cout << botfights_number << " bot fight(s) on map loaded" << std::endl;
    std::cout << botfightstigger_number << " bot fights tigger(s) on map loaded" << std::endl;
    std::cout << shops_number << " shop(s) on map loaded" << std::endl;
    std::cout << bots_number << " bots(s) on map loaded" << std::endl;
}

void BaseServer::loadBotFile(const std::string &mapfile,const std::string &file)
{
    (void)mapfile;
    if(botFiles.find(file)!=botFiles.cend())
        return;
    botFiles[file];//create the entry
    CATCHCHALLENGER_XMLDOCUMENT *domDocument;
    #ifndef EPOLLCATCHCHALLENGERSERVER
    if(CommonDatapack::commonDatapack.xmlLoadedFile.find(file)!=CommonDatapack::commonDatapack.xmlLoadedFile.cend())
        domDocument=&CommonDatapack::commonDatapack.xmlLoadedFile[file];
    else
    {
        domDocument=&CommonDatapack::commonDatapack.xmlLoadedFile[file];
        #else
        domDocument=new CATCHCHALLENGER_XMLDOCUMENT();
        #endif
        const auto loadOkay = domDocument->CATCHCHALLENGER_XMLDOCUMENTLOAD(CATCHCHALLENGER_XMLSTDSTRING_TONATIVESTRING(file));
        if(!CATCHCHALLENGER_XMLDOCUMENTRETURNISLOADED(loadOkay))
        {
            std::cerr << file+", "+CATCHCHALLENGER_XMLDOCUMENTERROR(domDocument) << std::endl;
            return;
        }
        #ifndef EPOLLCATCHCHALLENGERSERVER
    }
    #endif
    bool ok;
    const CATCHCHALLENGER_XMLELEMENT * root = domDocument->RootElement();
    if(root==NULL)
        return;
    if(!CATCHCHALLENGER_XMLNATIVETYPECOMPAREISSAME(root->CATCHCHALLENGER_XMLELENTVALUE(),"bots"))
    {
        std::cerr << "\"bots\" root balise not found for the xml file" << std::endl;
        return;
    }
    //load the bots
    const CATCHCHALLENGER_XMLELEMENT * child = root->FirstChildElement("bot");
    while(child!=NULL)
    {
        if(child->Attribute("id")==NULL)
            std::cerr << "Has not attribute \"id\": child->CATCHCHALLENGER_XMLELENTVALUE(): " << child->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(child) << ")" << std::endl;
        else if(!CATCHCHALLENGER_XMLELENTISXMLELEMENT(child))
            std::cerr << "Is not an element: child->CATCHCHALLENGER_XMLELENTVALUE(): " << child->CATCHCHALLENGER_XMLELENTVALUE() << ", name: " << CATCHCHALLENGER_XMLATTRIBUTETOSTRING(child->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("name"))) << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(child) << ")" << std::endl;
        else
        {
            uint32_t id=stringtouint32(CATCHCHALLENGER_XMLATTRIBUTETOSTRING(child->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("id"))),&ok);
            if(ok)
            {
                if(botIdLoaded.find(id)!=botIdLoaded.cend())
                    std::cerr << "Bot " << id << " into file " << file << " have same id as another bot: bot->CATCHCHALLENGER_XMLELENTVALUE(): " << child->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(child) << ")" << std::endl;
                botIdLoaded.insert(id);
                botFiles[file][id];
                const CATCHCHALLENGER_XMLELEMENT * step = child->FirstChildElement("step");
                while(step!=NULL)
                {
                    if(step->Attribute("id")==NULL)
                        std::cerr << "Has not attribute \"type\": bot->CATCHCHALLENGER_XMLELENTVALUE(): " << step->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(step) << ")" << std::endl;
                    else if(step->Attribute("type")==NULL)
                        std::cerr << "Has not attribute \"type\": bot->CATCHCHALLENGER_XMLELENTVALUE(): " << step->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(step) << ")" << std::endl;
                    else if(!CATCHCHALLENGER_XMLELENTISXMLELEMENT(step))
                        std::cerr << "Is not an element: bot->CATCHCHALLENGER_XMLELENTVALUE(): " << step->CATCHCHALLENGER_XMLELENTVALUE() << ", type: " << CATCHCHALLENGER_XMLATTRIBUTETOSTRING(step->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("type"))) << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(step) << ")" << std::endl;
                    else
                    {
                        uint32_t stepId=stringtouint32(CATCHCHALLENGER_XMLATTRIBUTETOSTRING(step->Attribute(CATCHCHALLENGER_XMLCHARPOINTERTONATIVESTRING("id"))),&ok);
                        if(ok)
                            botFiles[file][id].step[stepId]=step;
                    }
                    step = step->NextSiblingElement("step");
                }
                if(botFiles.at(file).at(id).step.find(1)==botFiles.at(file).at(id).step.cend())
                    botFiles[file].erase(id);
            }
            else
                std::cerr << "Attribute \"id\" is not a number: bot->CATCHCHALLENGER_XMLELENTVALUE(): " << child->CATCHCHALLENGER_XMLELENTVALUE() << " (at line: " << CATCHCHALLENGER_XMLELENTATLINE(child) << ")" << std::endl;
        }
        child = child->NextSiblingElement("bot");
    }

    #ifdef EPOLLCATCHCHALLENGERSERVER
    toDeleteAfterBotLoad.push_back(domDocument);
    #endif
    //delete domDocument;->reused after, need near botFiles.clear
}
