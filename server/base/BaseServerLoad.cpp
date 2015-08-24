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

void BaseServer::preload_randomBlock()
{
    //don't use BaseServerLogin::fpRandomFile to prevent lost entropy
    QDataStream randomDataStream(&GlobalServerData::serverPrivateVariables.randomData, QIODevice::WriteOnly);
    randomDataStream.setVersion(QDataStream::Qt_4_4);
    int index=0;
    while(index<CATCHCHALLENGER_SERVER_RANDOM_INTERNAL_SIZE)
    {
        randomDataStream << uint8_t(rand()%256);
        index++;
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
                            GlobalServerData::serverPrivateVariables.timerEvents.push_back(new QtTimerEvents(j.value().offset*1000*60,j.value().cycle*1000*60,index,sub_index));
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
    const int &listsize=entryListZone.size();
    int index=0;
    while(index<listsize)
    {
        if(!entryListZone.at(index).isFile())
        {
            index++;
            continue;
        }
        if(!entryListZone.at(index).fileName().contains(regexXmlFile))
        {
            std::cerr << entryListZone.at(index).fileName().toStdString() << " the zone file name not match" << std::endl;
            index++;
            continue;
        }
        std::string zoneCodeName=entryListZone.at(index).fileName().toStdString();
        zoneCodeName.erase(std::find(zoneCodeName.begin(), zoneCodeName.end(), BaseServer::text_dotxml));
        QDomDocument domDocument;
        const std::string &file=entryListZone.at(index).absoluteFilePath().toStdString();
        #ifndef EPOLLCATCHCHALLENGERSERVER
        if(CommonDatapack::commonDatapack.xmlLoadedFile.contains(file))
            domDocument=CommonDatapack::commonDatapack.xmlLoadedFile.value(file);
        else
        {
        #endif
            QFile itemsFile(file.c_str());
            QByteArray xmlContent;
            if(!itemsFile.open(QIODevice::ReadOnly))
            {
                std::cerr << "Unable to open the file: " << file.c_str() << ", error: " << itemsFile.errorString().toStdString() << std::endl;
                index++;
                continue;
            }
            xmlContent=itemsFile.readAll();
            itemsFile.close();
            QString errorStr;
            int errorLine,errorColumn;
            if(!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
            {
                std::cerr << "Unable to open the file: " << file.c_str() << ", Parse error at line " << errorLine << ", column " << errorColumn << ": " << errorStr.toStdString() << std::endl;
                index++;
                continue;
            }
            #ifndef EPOLLCATCHCHALLENGERSERVER
            CommonDatapack::commonDatapack.xmlLoadedFile[file]=domDocument;
        }
        #endif
        auto search = GlobalServerData::serverPrivateVariables.captureFightIdListByZoneToCaptureCity.find(zoneCodeName);
        if(search != GlobalServerData::serverPrivateVariables.captureFightIdListByZoneToCaptureCity.end())
        {
            std::cerr << "Unable to open the file: " << file.c_str() << ", zone code name already found" << std::endl;
            index++;
            continue;
        }
        QDomElement root(domDocument.documentElement());
        if(root.tagName().toStdString()!=BaseServer::text_zone)
        {
            std::cerr << "Unable to open the file: " << file.c_str() << ", \"zone\" root balise not found for the xml file" << std::endl;
            index++;
            continue;
        }

        //load capture
        std::vector<uint16_t> fightIdList;
        QDomElement capture(root.firstChildElement(QString::fromStdString(BaseServer::text_capture)));
        if(!capture.isNull())
        {
            if(capture.isElement() && capture.hasAttribute(QString::fromStdString(BaseServer::text_fightId)))
            {
                bool ok;
                const std::vector<std::string> &fightIdStringList=stringsplit(capture.attribute(QString::fromStdString(BaseServer::text_fightId)).toStdString(),';');
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
                if(sub_index==listsize && !fightIdList.size()==0)
                    GlobalServerData::serverPrivateVariables.captureFightIdListByZoneToCaptureCity[zoneCodeName]=fightIdList;
                break;
            }
            else
                std::cerr << "Unable to open the file: " << file << ", is not an element: child.tagName(): " << capture.tagName().toStdString() << " (at line: " << capture.lineNumber() << ")" << std::endl;
        }
        index++;
    }

    std::cout << GlobalServerData::serverPrivateVariables.captureFightIdListByZoneToCaptureCity.size() << " zone(s) loaded" << std::endl;
    return true;
}

void BaseServer::preload_map_semi_after_db_id()
{
    if(DictionaryServer::dictionary_map_database_to_internal.size()==0)
    {
        qDebug() << "Need be called after preload_dictionary_map()";
        abort();
    }
    Client::indexOfItemOnMap=0;//index of item on map, ordened by map and x,y ordened into the xml file, less bandwith than send map,x,y
    #ifdef CATCHCHALLENGER_GAMESERVER_PLANTBYPLAYER
    Client::indexOfDirtOnMap=0;//index of plant on map, ordened by map and x,y ordened into the xml file, less bandwith than send map,x,y
    #endif
    unsigned int indexMapSemi=0;
    while(indexMapSemi<semi_loaded_map.size())
    {
        const Map_semi &map_semi=semi_loaded_map.at(indexMapSemi);
        MapServer * const mapServer=static_cast<MapServer *>(map_semi.map);
        const std::string &sortFileName=mapServer->map_file;
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
                    const std::unordered_map<std::pair<uint8_t/*x*/,uint8_t/*y*/>,uint16_t/*db code*/,pairhash> &subItem=DictionaryServer::dictionary_pointOnMap_internal_to_database.at(sortFileName);
                    if(subItem.find(pair)!=subItem.end())
                        found=true;
                }
                if(found)
                {
                    pointOnMapDbCode=DictionaryServer::dictionary_pointOnMap_internal_to_database.at(sortFileName).at(pair);
                    DictionaryServer::dictionary_pointOnMap_database_to_internal[pointOnMapDbCode].indexOfItemOnMap=Client::indexOfItemOnMap;
                }
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
                    if(!GlobalServerData::serverPrivateVariables.db_server->asyncWrite(queryText.c_str()))
                    {
                        std::cerr << "Sql error for: " << queryText << ", error: " << GlobalServerData::serverPrivateVariables.db_server->errorMessage() << std::endl;
                        criticalDatabaseQueryFailed();abort();//stop because can't resolv the name
                    }
                    DictionaryServer::dictionary_pointOnMap_internal_to_database[sortFileName][std::pair<uint8_t,uint8_t>(item.point.x,item.point.y)]=dictionary_pointOnMap_maxId;
                    while((uint32_t)DictionaryServer::dictionary_pointOnMap_database_to_internal.size()<=dictionary_pointOnMap_maxId)
                    {
                        DictionaryServer::MapAndPoint mapAndPoint;
                        mapAndPoint.map=NULL;
                        mapAndPoint.x=0;
                        mapAndPoint.y=0;
                        //less bandwith than send map,x,y
                        mapAndPoint.indexOfItemOnMap=255;
                        #ifdef CATCHCHALLENGER_GAMESERVER_PLANTBYPLAYER
                        mapAndPoint.indexOfDirtOnMap=255;
                        #endif
                        DictionaryServer::dictionary_pointOnMap_database_to_internal << mapAndPoint;
                    }
                    {
                        DictionaryServer::MapAndPoint mapAndPoint;
                        mapAndPoint.map=mapServer;
                        mapAndPoint.x=item.point.x;
                        mapAndPoint.y=item.point.y;
                        //less bandwith than send map,x,y
                        mapAndPoint.indexOfItemOnMap=Client::indexOfItemOnMap;
                        DictionaryServer::dictionary_pointOnMap_database_to_internal[dictionary_pointOnMap_maxId]=mapAndPoint;
                    }

                    pointOnMapDbCode=dictionary_pointOnMap_maxId;
                }

                MapServer::ItemOnMap itemOnMap;
                itemOnMap.infinite=item.infinite;
                itemOnMap.item=item.item;
                itemOnMap.pointOnMapDbCode=pointOnMapDbCode;
                itemOnMap.indexOfOnMap=Client::indexOfItemOnMap;
                mapServer->itemsOnMap[std::pair<uint8_t,uint8_t>(item.point.x,item.point.y)]=itemOnMap;

                if(Client::indexOfItemOnMap>=254)//255 reserved
                {
                    qDebug() << "indexOfItemOnMap will be more than 255, overflow, too many item on map";
                    abort();
                }
                Client::indexOfItemOnMap++;
                index++;
            }
        }
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
                    const std::unordered_map<std::pair<uint8_t/*x*/,uint8_t/*y*/>,uint16_t/*db code*/,pairhash> &subItem=DictionaryServer::dictionary_pointOnMap_internal_to_database.at(sortFileName);
                    if(subItem.find(pair)!=subItem.end())
                        found=true;
                }
                if(found)
                {
                    pointOnMapDbCode=DictionaryServer::dictionary_pointOnMap_internal_to_database.at(sortFileName).at(pair);
                    #ifdef CATCHCHALLENGER_GAMESERVER_PLANTBYPLAYER
                    DictionaryServer::dictionary_pointOnMap_database_to_internal[pointOnMapDbCode].indexOfDirtOnMap=Client::indexOfDirtOnMap;
                    #endif
                }
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
                    if(!GlobalServerData::serverPrivateVariables.db_server->asyncWrite(queryText.c_str()))
                    {
                        std::cerr << "Sql error for: " << queryText << ", error: " << GlobalServerData::serverPrivateVariables.db_server->errorMessage() << std::endl;
                        criticalDatabaseQueryFailed();abort();//stop because can't resolv the name
                    }
                    DictionaryServer::dictionary_pointOnMap_internal_to_database[sortFileName][std::pair<uint8_t,uint8_t>(dirt.point.x,dirt.point.y)]=dictionary_pointOnMap_maxId;
                    while((uint32_t)DictionaryServer::dictionary_pointOnMap_database_to_internal.size()<=dictionary_pointOnMap_maxId)
                    {
                        DictionaryServer::MapAndPoint mapAndPoint;
                        mapAndPoint.map=NULL;
                        mapAndPoint.x=0;
                        mapAndPoint.y=0;
                        //less bandwith than send map,x,y
                        #ifdef CATCHCHALLENGER_GAMESERVER_PLANTBYPLAYER
                        mapAndPoint.indexOfDirtOnMap=255;
                        #endif
                        mapAndPoint.indexOfItemOnMap=255;
                        DictionaryServer::dictionary_pointOnMap_database_to_internal << mapAndPoint;
                    }
                    {
                        DictionaryServer::MapAndPoint mapAndPoint;
                        mapAndPoint.map=mapServer;
                        mapAndPoint.x=dirt.point.x;
                        mapAndPoint.y=dirt.point.y;
                        //less bandwith than send map,x,y
                        #ifdef CATCHCHALLENGER_GAMESERVER_PLANTBYPLAYER
                        mapAndPoint.indexOfDirtOnMap=Client::indexOfDirtOnMap;
                        #endif
                        DictionaryServer::dictionary_pointOnMap_database_to_internal[dictionary_pointOnMap_maxId]=mapAndPoint;
                    }

                    pointOnMapDbCode=dictionary_pointOnMap_maxId;
                }

                MapServer::PlantOnMap plantOnMap;
                #ifndef CATCHCHALLENGER_GAMESERVER_PLANTBYPLAYER
                plantOnMap.plant=0;//plant id
                plantOnMap.character=0;//player id
                plantOnMap.mature_at=0;//timestamp when is mature
                plantOnMap.player_owned_expire_at=0;//timestamp when is mature
                #else
                plantOnMap.indexOfOnMap=Client::indexOfDirtOnMap;
                #endif
                plantOnMap.pointOnMapDbCode=pointOnMapDbCode;
                mapServer->plants[std::pair<uint8_t,uint8_t>(dirt.point.x,dirt.point.y)]=plantOnMap;

                #ifdef CATCHCHALLENGER_GAMESERVER_PLANTBYPLAYER
                if(Client::indexOfDirtOnMap>=254)//255 reserved
                {
                    qDebug() << "indexOfDirtOnMap will be more than 255, overflow, too many dirt on map";
                    abort();
                }
                Client::indexOfDirtOnMap++;
                #endif
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
    std::cout << DictionaryLogin::dictionary_skin_internal_to_database.size() << " SQL skin dictionary" << std::endl;

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
            stringreplace(CommonDatapackServerSpec::commonDatapackServerSpec.serverProfileList[index].mapString,BaseServer::text_dottmx,"");
            index++;
        }
    }

    GlobalServerData::serverPrivateVariables.serverProfileInternalList.clear();
    unsigned int index=0;
    while(index<CommonDatapack::commonDatapack.profileList.size())
    {
        const Profile &profile=CommonDatapack::commonDatapack.profileList.at(index);
        const ServerProfile &serverProfile=CommonDatapackServerSpec::commonDatapackServerSpec.serverProfileList.at(index);
        ServerProfileInternal serverProfileInternal;
        serverProfileInternal.valid=false;
        if(serverProfile.mapString.size()>0 && GlobalServerData::serverPrivateVariables.map_list.find(serverProfile.mapString)!=GlobalServerData::serverPrivateVariables.map_list.end())
        {
            serverProfileInternal.map=
                    static_cast<MapServer *>(GlobalServerData::serverPrivateVariables.map_list.at(serverProfile.mapString));
            serverProfileInternal.x=serverProfile.x;
            serverProfileInternal.y=serverProfile.y;
            serverProfileInternal.orientation=serverProfile.orientation;
            const uint32_t &mapId=serverProfileInternal.map->reverse_db_id;
            const std::string &mapQuery=std::to_string(mapId)+
                    ","+
                    std::to_string(serverProfile.x)+
                    ","+
                    std::to_string(serverProfile.y)+
                    ","+
                    std::to_string(Orientation_bottom);
            switch(GlobalServerData::serverPrivateVariables.db_common->databaseType())
            {
                default:
                case DatabaseBase::DatabaseType::Mysql:
                    serverProfileInternal.preparedQueryAdd.push_back("INSERT INTO `character`(`id`,`account`,`pseudo`,`skin`,`type`,`clan`,`cash`,`date`,`warehouse_cash`,`clan_leader`,`time_to_delete`,`played_time`,`last_connect`,`starter`) VALUES(");
                    serverProfileInternal.preparedQueryAdd.push_back(/*id*/ ",");
                    serverProfileInternal.preparedQueryAdd.push_back(/*account*/ ",'");
                    serverProfileInternal.preparedQueryAdd.push_back(/*pseudo*/ "',");
                    serverProfileInternal.preparedQueryAdd.push_back(/*skin*/ ",0,0,"+
                            std::to_string(profile.cash)+",");
                    serverProfileInternal.preparedQueryAdd.push_back(/*QDateTime::currentDateTime().toTime_t()*/ ",0,0,0,0,0,"+
                            std::to_string(DictionaryLogin::dictionary_starter_internal_to_database.at(index))+");");
                break;
                case DatabaseBase::DatabaseType::SQLite:
                    serverProfileInternal.preparedQueryAdd.push_back("INSERT INTO character(id,account,pseudo,skin,type,clan,cash,date,warehouse_cash,clan_leader,time_to_delete,played_time,last_connect,starter) VALUES(");
                    serverProfileInternal.preparedQueryAdd.push_back(/*id*/ ",");
                    serverProfileInternal.preparedQueryAdd.push_back(/*account*/ ",'");
                    serverProfileInternal.preparedQueryAdd.push_back(/*pseudo*/ "',");
                    serverProfileInternal.preparedQueryAdd.push_back(/*skin*/ ",0,0,"+
                            std::to_string(profile.cash)+",");
                    serverProfileInternal.preparedQueryAdd.push_back(/*QDateTime::currentDateTime().toTime_t()*/ ",0,0,0,0,0,"+
                            std::to_string(DictionaryLogin::dictionary_starter_internal_to_database.at(index))+");");
                break;
                case DatabaseBase::DatabaseType::PostgreSQL:
                    serverProfileInternal.preparedQueryAdd.push_back("INSERT INTO character(id,account,pseudo,skin,type,clan,cash,date,warehouse_cash,clan_leader,time_to_delete,played_time,last_connect,starter) VALUES(");
                    serverProfileInternal.preparedQueryAdd.push_back(/*id*/ ",");
                    serverProfileInternal.preparedQueryAdd.push_back(/*account*/ ",'");
                    serverProfileInternal.preparedQueryAdd.push_back(/*pseudo*/ "',");
                    serverProfileInternal.preparedQueryAdd.push_back(/*skin*/ ",0,0,"+
                            std::to_string(profile.cash)+",");
                    serverProfileInternal.preparedQueryAdd.push_back(/*QDateTime::currentDateTime().toTime_t()*/ ",0,FALSE,0,0,0,"+
                            std::to_string(DictionaryLogin::dictionary_starter_internal_to_database.at(index))+");");
                break;
            }
            switch(GlobalServerData::serverPrivateVariables.db_server->databaseType())
            {
                default:
                case DatabaseBase::DatabaseType::Mysql:
                    serverProfileInternal.preparedQuerySelect.push_back("INSERT INTO `character_forserver`(`character`,`map`,`x`,`y`,`orientation`,`rescue_map`,`rescue_x`,`rescue_y`,`rescue_orientation`,`unvalidated_rescue_map`,`unvalidated_rescue_x`,`unvalidated_rescue_y`,`unvalidated_rescue_orientation`,`date`,`market_cash`) VALUES(");
                    serverProfileInternal.preparedQuerySelect.push_back(/*id*/ ","+mapQuery+","+mapQuery+","+mapQuery+",");
                    serverProfileInternal.preparedQuerySelect.push_back(/*QDateTime::currentDateTime().toTime_t()*/ ",0);");
                break;
                case DatabaseBase::DatabaseType::SQLite:
                    serverProfileInternal.preparedQuerySelect.push_back("INSERT INTO character_forserver(character,map,x,y,orientation,rescue_map,rescue_x,rescue_y,rescue_orientation,unvalidated_rescue_map,unvalidated_rescue_x,unvalidated_rescue_y,unvalidated_rescue_orientation,date,market_cash) VALUES(");
                    serverProfileInternal.preparedQuerySelect.push_back(/*id*/ ","+mapQuery+","+mapQuery+","+mapQuery+",");
                    serverProfileInternal.preparedQuerySelect.push_back(/*QDateTime::currentDateTime().toTime_t()*/ ",0);");
                break;
                case DatabaseBase::DatabaseType::PostgreSQL:
                    serverProfileInternal.preparedQuerySelect.push_back("INSERT INTO character_forserver(character,map,x,y,orientation,rescue_map,rescue_x,rescue_y,rescue_orientation,unvalidated_rescue_map,unvalidated_rescue_x,unvalidated_rescue_y,unvalidated_rescue_orientation,date,market_cash) VALUES(");
                    serverProfileInternal.preparedQuerySelect.push_back(/*id*/ ","+mapQuery+","+mapQuery+","+mapQuery+",");
                    serverProfileInternal.preparedQuerySelect.push_back(/*QDateTime::currentDateTime().toTime_t()*/ ",0);");
                break;
            }
            serverProfileInternal.valid=true;
        }
        GlobalServerData::serverPrivateVariables.serverProfileInternalList.push_back(serverProfileInternal);
        index++;
    }

    std::cout << GlobalServerData::serverPrivateVariables.serverProfileInternalList.size() << " profile loaded" << std::endl;
}

bool BaseServer::preload_zone()
{
    //open and quick check the file
    entryListZone=QFileInfoList(QDir(
                                    std::string(GlobalServerData::serverSettings.datapack_basePath+DATAPACK_BASE_PATH_ZONE1+CommonSettingsServer::commonSettingsServer.mainDatapackCode+DATAPACK_BASE_PATH_ZONE2).c_str()
                                    ).entryInfoList(QDir::AllEntries|QDir::NoDotAndDotDot));
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
        std::string zoneCodeName=entryListZone.at(entryListIndex).fileName().toStdString();
        stringreplace(zoneCodeName,BaseServer::text_dotxml,"");
        const std::string &tempString=std::string(GlobalServerData::serverPrivateVariables.db_server->value(0));
        const uint32_t &clanId=stringtouint32(tempString,&ok);
        if(ok)
        {
            GlobalServerData::serverPrivateVariables.cityStatusList[zoneCodeName].clan=clanId;
            GlobalServerData::serverPrivateVariables.cityStatusListReverse[clanId]=zoneCodeName;
        }
        else
            std::cerr << "clan id is failed to convert to number for city status" << std::endl;;
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
    std::cout << "start preload the map, into: " << GlobalServerData::serverPrivateVariables.datapack_mapPath << std::endl;;
    #endif
    Map_loader map_temp;
    std::vector<std::string> map_name;
    std::vector<std::string> map_name_to_do_id;
    std::vector<std::string> returnList=FacilityLibGeneral::listFolder(GlobalServerData::serverPrivateVariables.datapack_mapPath);
    returnList.sort();

    if(returnList.isEmpty())
    {
        qDebug() << "No file map to list";
        abort();
    }
    if(!semi_loaded_map.isEmpty() || GlobalServerData::serverPrivateVariables.flat_map_list!=NULL)
    {
        qDebug() << "preload_the_map() already call";
        abort();
    }
    //load the map
    int size=returnList.size();
    int index=0;
    int sub_index;
    std::string tmxRemove(".tmx");
    QRegularExpression mapFilter(QLatin1String("\\.tmx$"));
    QRegularExpression mapExclude(QLatin1String("[\"']"));
    QList<CommonMap *> flat_map_list_temp;
    while(index<size)
    {
        std::string fileName=returnList.at(index);
        fileName.replace(BaseServer::text_antislash,BaseServer::text_slash);
        if(fileName.contains(mapFilter) && !fileName.contains(mapExclude))
        {
            #ifdef DEBUG_MESSAGE_MAP_LOAD
            DebugClass::debugConsole(std::stringLiteral("load the map: %1").arg(fileName));
            #endif
            std::string sortFileName=fileName;
            sortFileName.remove(tmxRemove);
            map_name_to_do_id << sortFileName;
            if(map_temp.tryLoadMap(GlobalServerData::serverPrivateVariables.datapack_mapPath+fileName))
            {
                switch(GlobalServerData::serverSettings.mapVisibility.mapVisibilityAlgorithm)
                {
                    case MapVisibilityAlgorithmSelection_Simple:
                        flat_map_list_temp << new Map_server_MapVisibility_Simple_StoreOnSender;
                    break;
                    case MapVisibilityAlgorithmSelection_WithBorder:
                        flat_map_list_temp << new Map_server_MapVisibility_WithBorder_StoreOnSender;
                    break;
                    case MapVisibilityAlgorithmSelection_None:
                    default:
                        flat_map_list_temp << new MapServer;
                    break;
                }
                MapServer *mapServer=static_cast<MapServer *>(flat_map_list_temp.last());
                GlobalServerData::serverPrivateVariables.map_list[sortFileName]=mapServer;

                mapServer->width			= map_temp.map_to_send.width;
                mapServer->height			= map_temp.map_to_send.height;
                mapServer->parsed_layer	= map_temp.map_to_send.parsed_layer;
                mapServer->map_file		= sortFileName;

                map_name << sortFileName;

                parseJustLoadedMap(map_temp.map_to_send,fileName);

                Map_semi map_semi;
                map_semi.map				= GlobalServerData::serverPrivateVariables.map_list.value(sortFileName);

                if(!map_temp.map_to_send.border.top.fileName.isEmpty())
                {
                    map_semi.border.top.fileName		= Map_loader::resolvRelativeMap(GlobalServerData::serverPrivateVariables.datapack_mapPath+fileName,map_temp.map_to_send.border.top.fileName,GlobalServerData::serverPrivateVariables.datapack_mapPath);
                    map_semi.border.top.fileName.remove(BaseServer::text_dottmx);
                    map_semi.border.top.x_offset		= map_temp.map_to_send.border.top.x_offset;
                }
                if(!map_temp.map_to_send.border.bottom.fileName.isEmpty())
                {
                    map_semi.border.bottom.fileName		= Map_loader::resolvRelativeMap(GlobalServerData::serverPrivateVariables.datapack_mapPath+fileName,map_temp.map_to_send.border.bottom.fileName,GlobalServerData::serverPrivateVariables.datapack_mapPath);
                    map_semi.border.bottom.fileName.remove(BaseServer::text_dottmx);
                    map_semi.border.bottom.x_offset		= map_temp.map_to_send.border.bottom.x_offset;
                }
                if(!map_temp.map_to_send.border.left.fileName.isEmpty())
                {
                    map_semi.border.left.fileName		= Map_loader::resolvRelativeMap(GlobalServerData::serverPrivateVariables.datapack_mapPath+fileName,map_temp.map_to_send.border.left.fileName,GlobalServerData::serverPrivateVariables.datapack_mapPath);
                    map_semi.border.left.fileName.remove(BaseServer::text_dottmx);
                    map_semi.border.left.y_offset		= map_temp.map_to_send.border.left.y_offset;
                }
                if(!map_temp.map_to_send.border.right.fileName.isEmpty())
                {
                    map_semi.border.right.fileName		= Map_loader::resolvRelativeMap(GlobalServerData::serverPrivateVariables.datapack_mapPath+fileName,map_temp.map_to_send.border.right.fileName,GlobalServerData::serverPrivateVariables.datapack_mapPath);
                    map_semi.border.right.fileName.remove(BaseServer::text_dottmx);
                    map_semi.border.right.y_offset		= map_temp.map_to_send.border.right.y_offset;
                }

                sub_index=0;
                const int &listsize=map_temp.map_to_send.teleport.size();
                while(sub_index<listsize)
                {
                    map_temp.map_to_send.teleport[sub_index].map=Map_loader::resolvRelativeMap(GlobalServerData::serverPrivateVariables.datapack_mapPath+fileName,map_temp.map_to_send.teleport.at(sub_index).map,GlobalServerData::serverPrivateVariables.datapack_mapPath);
                    map_temp.map_to_send.teleport[sub_index].map.remove(BaseServer::text_dottmx);
                    sub_index++;
                }

                map_semi.old_map=map_temp.map_to_send;

                semi_loaded_map << map_semi;
            }
            else
                DebugClass::debugConsole(std::stringLiteral("error at loading: %1, error: %2").arg(fileName).arg(map_temp.errorString()));
        }
        index++;
    }
    {
        GlobalServerData::serverPrivateVariables.flat_map_list=static_cast<CommonMap **>(malloc(sizeof(CommonMap *)*flat_map_list_temp.size()));
        int index=0;
        while(index<flat_map_list_temp.size())
        {
            GlobalServerData::serverPrivateVariables.flat_map_list[index]=flat_map_list_temp.at(index);
            index++;
        }
    }

    map_name_to_do_id.sort();

    //resolv the border map name into their pointer
    size=semi_loaded_map.size();
    index=0;
    while(index<size)
    {
        if(!semi_loaded_map.at(index).border.bottom.fileName.isEmpty() && GlobalServerData::serverPrivateVariables.map_list.contains(semi_loaded_map.at(index).border.bottom.fileName))
            semi_loaded_map[index].map->border.bottom.map=GlobalServerData::serverPrivateVariables.map_list.value(semi_loaded_map.at(index).border.bottom.fileName);
        else
            semi_loaded_map[index].map->border.bottom.map=NULL;

        if(!semi_loaded_map.at(index).border.top.fileName.isEmpty() && GlobalServerData::serverPrivateVariables.map_list.contains(semi_loaded_map.at(index).border.top.fileName))
            semi_loaded_map[index].map->border.top.map=GlobalServerData::serverPrivateVariables.map_list.value(semi_loaded_map.at(index).border.top.fileName);
        else
            semi_loaded_map[index].map->border.top.map=NULL;

        if(!semi_loaded_map.at(index).border.left.fileName.isEmpty() && GlobalServerData::serverPrivateVariables.map_list.contains(semi_loaded_map.at(index).border.left.fileName))
            semi_loaded_map[index].map->border.left.map=GlobalServerData::serverPrivateVariables.map_list.value(semi_loaded_map.at(index).border.left.fileName);
        else
            semi_loaded_map[index].map->border.left.map=NULL;

        if(!semi_loaded_map.at(index).border.right.fileName.isEmpty() && GlobalServerData::serverPrivateVariables.map_list.contains(semi_loaded_map.at(index).border.right.fileName))
            semi_loaded_map[index].map->border.right.map=GlobalServerData::serverPrivateVariables.map_list.value(semi_loaded_map.at(index).border.right.fileName);
        else
            semi_loaded_map[index].map->border.right.map=NULL;

        index++;
    }

    //resolv the teleported into their pointer
    size=semi_loaded_map.size();
    index=0;
    while(index<size)
    {
        int sub_index=0;
        while(sub_index<semi_loaded_map.value(index).old_map.teleport.size())
        {
            std::string teleportString=semi_loaded_map.value(index).old_map.teleport.at(sub_index).map;
            teleportString.remove(BaseServer::text_dottmx);
            if(GlobalServerData::serverPrivateVariables.map_list.contains(teleportString))
            {
                if(semi_loaded_map.value(index).old_map.teleport.at(sub_index).destination_x<GlobalServerData::serverPrivateVariables.map_list.value(teleportString)->width
                        && semi_loaded_map.value(index).old_map.teleport.at(sub_index).destination_y<GlobalServerData::serverPrivateVariables.map_list.value(teleportString)->height)
                {
                    int virtual_position=semi_loaded_map.value(index).old_map.teleport.at(sub_index).source_x+semi_loaded_map.value(index).old_map.teleport.at(sub_index).source_y*semi_loaded_map.value(index).map->width;
                    if(semi_loaded_map.value(index).map->teleporter.contains(virtual_position))
                    {
                        DebugClass::debugConsole(std::stringLiteral("already found teleporter on the map: %1(%2,%3), to %4 (%5,%6)")
                             .arg(semi_loaded_map.value(index).map->map_file)
                             .arg(semi_loaded_map.value(index).old_map.teleport.at(sub_index).source_x)
                             .arg(semi_loaded_map.value(index).old_map.teleport.at(sub_index).source_y)
                             .arg(teleportString)
                             .arg(semi_loaded_map.value(index).old_map.teleport.at(sub_index).destination_x)
                             .arg(semi_loaded_map.value(index).old_map.teleport.at(sub_index).destination_y));
                    }
                    else
                    {
                        #ifdef DEBUG_MESSAGE_MAP_LOAD
                        DebugClass::debugConsole(std::stringLiteral("teleporter on the map: %1(%2,%3), to %4 (%5,%6)")
                                     .arg(semi_loaded_map.value(index).map->map_file)
                                     .arg(semi_loaded_map.value(index).old_map.teleport.at(sub_index).source_x)
                                     .arg(semi_loaded_map.value(index).old_map.teleport.at(sub_index).source_y)
                                     .arg(teleportString)
                                     .arg(semi_loaded_map.value(index).old_map.teleport.at(sub_index).destination_x)
                                     .arg(semi_loaded_map.value(index).old_map.teleport.at(sub_index).destination_y));
                        #endif
                        CommonMap::Teleporter *teleporter=&semi_loaded_map[index].map->teleporter[virtual_position];
                        teleporter->map=GlobalServerData::serverPrivateVariables.map_list.value(teleportString);
                        teleporter->x=semi_loaded_map.value(index).old_map.teleport.at(sub_index).destination_x;
                        teleporter->y=semi_loaded_map.value(index).old_map.teleport.at(sub_index).destination_y;
                        teleporter->condition=semi_loaded_map.value(index).old_map.teleport.at(sub_index).condition;
                    }
                }
                else
                    DebugClass::debugConsole(std::stringLiteral("wrong teleporter on the map: %1(%2,%3), to %4 (%5,%6) because the tp is out of range")
                         .arg(semi_loaded_map.value(index).map->map_file)
                         .arg(semi_loaded_map.value(index).old_map.teleport.at(sub_index).source_x)
                         .arg(semi_loaded_map.value(index).old_map.teleport.at(sub_index).source_y)
                         .arg(teleportString)
                         .arg(semi_loaded_map.value(index).old_map.teleport.at(sub_index).destination_x)
                         .arg(semi_loaded_map.value(index).old_map.teleport.at(sub_index).destination_y));
            }
            else
                DebugClass::debugConsole(std::stringLiteral("wrong teleporter on the map: %1(%2,%3), to %4 (%5,%6) because the map is not found")
                     .arg(semi_loaded_map.value(index).map->map_file)
                     .arg(semi_loaded_map.value(index).old_map.teleport.at(sub_index).source_x)
                     .arg(semi_loaded_map.value(index).old_map.teleport.at(sub_index).source_y)
                     .arg(teleportString)
                     .arg(semi_loaded_map.value(index).old_map.teleport.at(sub_index).destination_x)
                     .arg(semi_loaded_map.value(index).old_map.teleport.at(sub_index).destination_y));

            sub_index++;
        }
        index++;
    }

    //clean border balise without another oposite border
    size=semi_loaded_map.size();
    index=0;
    while(index<size)
    {
        if(GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.bottom.map!=NULL && GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.bottom.map->border.top.map!=GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index)))
        {
            if(GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.bottom.map->border.top.map==NULL)
                DebugClass::debugConsole(std::stringLiteral("the map %1 have bottom map: %2, the map %2 have not a top map").arg(GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->map_file).arg(GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.bottom.map->map_file));
            else
                DebugClass::debugConsole(std::stringLiteral("the map %1 have bottom map: %2, the map %2 have this top map: %3").arg(GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->map_file).arg(GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.bottom.map->map_file).arg(GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.bottom.map->border.top.map->map_file));
            GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.bottom.map=NULL;
        }
        if(GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.top.map!=NULL && GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.top.map->border.bottom.map!=GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index)))
        {
            if(GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.top.map->border.bottom.map==NULL)
                DebugClass::debugConsole(std::stringLiteral("the map %1 have top map: %2, the map %2 have not a bottom map").arg(GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->map_file).arg(GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.top.map->map_file));
            else
                DebugClass::debugConsole(std::stringLiteral("the map %1 have top map: %2, the map %2 have this bottom map: %3").arg(GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->map_file).arg(GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.top.map->map_file).arg(GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.top.map->border.bottom.map->map_file));
            GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.top.map=NULL;
        }
        if(GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.left.map!=NULL && GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.left.map->border.right.map!=GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index)))
        {
            if(GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.left.map->border.right.map==NULL)
                DebugClass::debugConsole(std::stringLiteral("the map %1 have left map: %2, the map %2 have not a right map").arg(GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->map_file).arg(GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.left.map->map_file));
            else
                DebugClass::debugConsole(std::stringLiteral("the map %1 have left map: %2, the map %2 have this right map: %3").arg(GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->map_file).arg(GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.left.map->map_file).arg(GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.left.map->border.right.map->map_file));
            GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.left.map=NULL;
        }
        if(GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.right.map!=NULL && GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.right.map->border.left.map!=GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index)))
        {
            if(GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.right.map->border.left.map==NULL)
                DebugClass::debugConsole(std::stringLiteral("the map %1 have right map: %2, the map %2 have not a left map").arg(GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->map_file).arg(GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.right.map->map_file));
            else
                DebugClass::debugConsole(std::stringLiteral("the map %1 have right map: %2, the map %2 have this left map: %3").arg(GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->map_file).arg(GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.right.map->map_file).arg(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.right.map->border.left.map->map_file));
            GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.right.map=NULL;
        }
        index++;
    }

    //resolv the near map
    size=semi_loaded_map.size();
    index=0;
    while(index<size)
    {
        if(GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.bottom.map!=NULL && !GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->near_map.contains(GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.bottom.map))
        {
            GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->near_map << GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.bottom.map;
            if(GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.left.map!=NULL && !GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->near_map.contains(GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.left.map))
                GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->near_map << GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.left.map;
            if(GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.right.map!=NULL && !GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->near_map.contains(GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.right.map))
                GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->near_map << GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.right.map;
        }

        if(GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.top.map!=NULL && !GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->near_map.contains(GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.top.map))
        {
            GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->near_map << GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.top.map;
            if(GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.left.map!=NULL &&  !GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->near_map.contains(GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.left.map))
                GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->near_map << GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.left.map;
            if(GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.right.map!=NULL &&  !GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->near_map.contains(GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.right.map))
                GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->near_map << GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.right.map;
        }

        if(GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.right.map!=NULL && !GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->near_map.contains(GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.right.map))
        {
            GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->near_map << GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.right.map;
            if(GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.top.map!=NULL &&  !GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->near_map.contains(GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.top.map))
                GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->near_map << GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.top.map;
            if(GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.bottom.map!=NULL &&  !GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->near_map.contains(GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.bottom.map))
                GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->near_map << GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.bottom.map;
        }

        if(GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.left.map!=NULL && !GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->near_map.contains(GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.left.map))
        {
            GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->near_map << GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.left.map;
            if(GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.top.map!=NULL &&  !GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->near_map.contains(GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.top.map))
                GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->near_map << GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.top.map;
            if(GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.bottom.map!=NULL &&  !GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->near_map.contains(GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.bottom.map))
                GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->near_map << GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.bottom.map;
        }

        GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border_map=GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->near_map;
        GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->near_map << GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index));
        index++;
    }

    //resolv the offset to change of map
    size=semi_loaded_map.size();
    index=0;
    while(index<size)
    {
        if(GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.bottom.map!=NULL)
            GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.bottom.x_offset=semi_loaded_map.at(map_name.indexOf(semi_loaded_map.at(index).border.bottom.fileName)).border.top.x_offset-semi_loaded_map.at(index).border.bottom.x_offset;
        else
            GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.bottom.x_offset=0;
        if(GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.top.map!=NULL)
            GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.top.x_offset=semi_loaded_map.at(map_name.indexOf(semi_loaded_map.at(index).border.top.fileName)).border.bottom.x_offset-semi_loaded_map.at(index).border.top.x_offset;
        else
            GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.top.x_offset=0;
        if(GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.left.map!=NULL)
            GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.left.y_offset=semi_loaded_map.at(map_name.indexOf(semi_loaded_map.at(index).border.left.fileName)).border.right.y_offset-semi_loaded_map.at(index).border.left.y_offset;
        else
            GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.left.y_offset=0;
        if(GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.right.map!=NULL)
            GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.right.y_offset=semi_loaded_map.at(map_name.indexOf(semi_loaded_map.at(index).border.right.fileName)).border.left.y_offset-semi_loaded_map.at(index).border.right.y_offset;
        else
            GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.right.y_offset=0;
        index++;
    }

    //nead be after the offet
    preload_the_bots(semi_loaded_map);

    //load the rescue
    size=semi_loaded_map.size();
    index=0;
    while(index<size)
    {
        sub_index=0;
        while(sub_index<semi_loaded_map.value(index).old_map.rescue_points.size())
        {
            const Map_to_send::Map_Point &point=semi_loaded_map.value(index).old_map.rescue_points.at(sub_index);
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
        if(GlobalServerData::serverPrivateVariables.map_list.contains(map_name_to_do_id.at(index)))
        {
            GlobalServerData::serverPrivateVariables.map_list[map_name_to_do_id.at(index)]->id=index;
            GlobalServerData::serverPrivateVariables.id_map_to_map[GlobalServerData::serverPrivateVariables.map_list[map_name_to_do_id.at(index)]->id]=map_name_to_do_id.at(index);
        }
        else
            abort();
        index++;
    }

    DebugClass::debugConsole(std::stringLiteral("%1 map(s) loaded").arg(GlobalServerData::serverPrivateVariables.map_list.size()));

    DictionaryServer::dictionary_pointOnMap_internal_to_database.clear();
    botFiles.clear();
    return true;
}

void BaseServer::criticalDatabaseQueryFailed()
{
    unload_the_data();
    quitForCriticalDatabaseQueryFailed();
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

    DebugClass::debugConsole(std::stringLiteral("%1 skin(s) loaded").arg(listsize));
}

void BaseServer::preload_the_datapack()
{
    std::vector<std::string> extensionAllowedTemp=std::string(CATCHCHALLENGER_EXTENSION_ALLOWED+BaseServer::text_dotcomma+CATCHCHALLENGER_EXTENSION_COMPRESSED).split(BaseServer::text_dotcomma);
    BaseServerMasterSendDatapack::extensionAllowed=extensionAllowedTemp.toSet();
    std::vector<std::string> compressedExtensionAllowedTemp=std::string(CATCHCHALLENGER_EXTENSION_COMPRESSED).split(BaseServer::text_dotcomma);
    BaseServerMasterSendDatapack::compressedExtension=compressedExtensionAllowedTemp.toSet();
    Client::datapack_list_cache_timestamp_base=0;
    Client::datapack_list_cache_timestamp_main=0;
    Client::datapack_list_cache_timestamp_sub=0;

    GlobalServerData::serverPrivateVariables.mainDatapackFolder=GlobalServerData::serverSettings.datapack_basePath+std::stringLiteral("map/main/")+CommonSettingsServer::commonSettingsServer.mainDatapackCode+std::stringLiteral("/");
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    {
        if(CommonSettingsServer::commonSettingsServer.mainDatapackCode.isEmpty())
        {
            DebugClass::debugConsole(std::stringLiteral("CommonSettingsServer::commonSettingsServer.mainDatapackCode.isEmpty"));
            abort();
        }
        if(!CommonSettingsServer::commonSettingsServer.mainDatapackCode.contains(QRegularExpression(CATCHCHALLENGER_CHECK_MAINDATAPACKCODE)))
        {
            DebugClass::debugConsole(
                        std::stringLiteral("CommonSettingsServer::commonSettingsServer.mainDatapackCode not match CATCHCHALLENGER_CHECK_MAINDATAPACKCODE %1 not contain %2")
                         .arg(CommonSettingsServer::commonSettingsServer.mainDatapackCode)
                         .arg(std::stringLiteral(CATCHCHALLENGER_CHECK_MAINDATAPACKCODE))
                         );
            abort();
        }
        if(!QDir(GlobalServerData::serverPrivateVariables.mainDatapackFolder).exists())
        {
            DebugClass::debugConsole(GlobalServerData::serverPrivateVariables.mainDatapackFolder+std::stringLiteral(" don't exists"));
            abort();
        }
    }
    #endif
    if(!CommonSettingsServer::commonSettingsServer.subDatapackCode.isEmpty())
    {
        GlobalServerData::serverPrivateVariables.subDatapackFolder=GlobalServerData::serverSettings.datapack_basePath+std::stringLiteral("map/main/")+CommonSettingsServer::commonSettingsServer.mainDatapackCode+std::stringLiteral("/")+
                std::stringLiteral("sub/")+CommonSettingsServer::commonSettingsServer.subDatapackCode+std::stringLiteral("/");
        if(!QDir(GlobalServerData::serverPrivateVariables.subDatapackFolder).exists())
        {
            DebugClass::debugConsole(GlobalServerData::serverPrivateVariables.subDatapackFolder+std::stringLiteral(" don't exists, drop spec"));
            GlobalServerData::serverPrivateVariables.subDatapackFolder.clear();
            CommonSettingsServer::commonSettingsServer.subDatapackCode.clear();
        }
    }

    #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    //do the base
    {
        QCryptographicHash hashBase(QCryptographicHash::Sha224);
        const QHash<std::string,Client::DatapackCacheFile> &pair=Client::datapack_file_list(GlobalServerData::serverSettings.datapack_basePath,false);
        std::vector<std::string> datapack_file_temp=pair.keys();
        datapack_file_temp.sort();
        const QRegularExpression mainDatapackBaseFilter("^map[/\\\\]main[/\\\\]");
        int index=0;
        while(index<datapack_file_temp.size()) {
            QFile file(GlobalServerData::serverSettings.datapack_basePath+datapack_file_temp.at(index));
            if(datapack_file_temp.at(index).contains(GlobalServerData::serverPrivateVariables.datapack_rightFileName))
            {
                if(file.open(QIODevice::ReadOnly))
                {
                    //read and load the file
                    const QByteArray &data=file.readAll();

                    if((1+datapack_file_temp.at(index).size()+4+data.size())>=CATCHCHALLENGER_MAX_PACKET_SIZE)
                    {
                        if(BaseServerMasterSendDatapack::compressedExtension.contains(QFileInfo(file).suffix()))
                        {
                            if(ProtocolParsing::compressionTypeServer==ProtocolParsing::CompressionType::None)
                            {
                                DebugClass::debugConsole(std::stringLiteral("The file %1 is over the maximum packet size, but can be compressed, try enable the compression").arg(GlobalServerData::serverSettings.datapack_basePath+datapack_file_temp.at(index)));
                                abort();
                            }
                        }
                        else
                        {
                            DebugClass::debugConsole(std::stringLiteral("The file %1 is over the maximum packet size").arg(GlobalServerData::serverSettings.datapack_basePath+datapack_file_temp.at(index)));
                            abort();
                        }
                    }

                    //switch the data to correct hash or drop it
                    if(datapack_file_temp.at(index).contains(mainDatapackBaseFilter))
                    {}
                    else
                    {
                        QCryptographicHash hashFile(QCryptographicHash::Sha224);
                        hashFile.addData(data);
                        Client::DatapackCacheFile cacheFile;
                        cacheFile.partialHash=*reinterpret_cast<const int *>(hashFile.result().constData());
                        Client::datapack_file_hash_cache_base[datapack_file_temp.at(index)]=cacheFile;

                        hashBase.addData(data);
                    }

                    file.close();
                }
                else
                {
                    DebugClass::debugConsole(std::stringLiteral("Stop now! Unable to open the file %1 to do the datapack checksum for the mirror").arg(file.fileName()));
                    abort();
                }
            }
            else
                DebugClass::debugConsole(std::stringLiteral("File excluded because don't match the regex: %1").arg(file.fileName()));
            index++;
        }
        CommonSettingsCommon::commonSettingsCommon.datapackHashBase=hashBase.result();
    }
    #endif
    //do the main
    {
        QCryptographicHash hashMain(QCryptographicHash::Sha224);
        const QHash<std::string,Client::DatapackCacheFile> &pair=Client::datapack_file_list(GlobalServerData::serverPrivateVariables.mainDatapackFolder,false);
        std::vector<std::string> datapack_file_temp=pair.keys();
        datapack_file_temp.sort();
        const QRegularExpression mainDatapackFolderFilter("^sub[/\\\\]");
        int index=0;
        while(index<datapack_file_temp.size()) {
            QFile file(GlobalServerData::serverPrivateVariables.mainDatapackFolder+datapack_file_temp.at(index));
            if(datapack_file_temp.at(index).contains(GlobalServerData::serverPrivateVariables.datapack_rightFileName))
            {
                if(file.open(QIODevice::ReadOnly))
                {
                    //read and load the file
                    const QByteArray &data=file.readAll();

                    if((1+datapack_file_temp.at(index).size()+4+data.size())>=CATCHCHALLENGER_MAX_PACKET_SIZE)
                    {
                        if(BaseServerMasterSendDatapack::compressedExtension.contains(QFileInfo(file).suffix()))
                        {
                            if(ProtocolParsing::compressionTypeServer==ProtocolParsing::CompressionType::None)
                            {
                                DebugClass::debugConsole(std::stringLiteral("The file %1 is over the maximum packet size, but can be compressed, try enable the compression").arg(GlobalServerData::serverPrivateVariables.mainDatapackFolder+datapack_file_temp.at(index)));
                                abort();
                            }
                        }
                        else
                        {
                            DebugClass::debugConsole(std::stringLiteral("The file %1 is over the maximum packet size").arg(GlobalServerData::serverPrivateVariables.mainDatapackFolder+datapack_file_temp.at(index)));
                            abort();
                        }
                    }

                    //switch the data to correct hash or drop it
                    if(datapack_file_temp.at(index).contains(mainDatapackFolderFilter))
                    {
                    }
                    else
                    {
                        QCryptographicHash hashFile(QCryptographicHash::Sha224);
                        hashFile.addData(data);
                        Client::DatapackCacheFile cacheFile;
                        cacheFile.partialHash=*reinterpret_cast<const int *>(hashFile.result().constData());
                        Client::datapack_file_hash_cache_main[datapack_file_temp.at(index)]=cacheFile;

                        hashMain.addData(data);
                    }

                    file.close();
                }
                else
                {
                    DebugClass::debugConsole(std::stringLiteral("Stop now! Unable to open the file %1 to do the datapack checksum for the mirror").arg(file.fileName()));
                    abort();
                }
            }
            else
                DebugClass::debugConsole(std::stringLiteral("File excluded because don't match the regex: %1").arg(file.fileName()));
            index++;
        }
        CommonSettingsServer::commonSettingsServer.datapackHashServerMain=hashMain.result();
    }
    //do the sub
    if(!GlobalServerData::serverPrivateVariables.subDatapackFolder.isEmpty())
    {
        QCryptographicHash hashSub(QCryptographicHash::Sha224);
        const QHash<std::string,Client::DatapackCacheFile> &pair=Client::datapack_file_list(GlobalServerData::serverPrivateVariables.subDatapackFolder,false);
        std::vector<std::string> datapack_file_temp=pair.keys();
        datapack_file_temp.sort();
        int index=0;
        while(index<datapack_file_temp.size()) {
            QFile file(GlobalServerData::serverPrivateVariables.subDatapackFolder+datapack_file_temp.at(index));
            if(datapack_file_temp.at(index).contains(GlobalServerData::serverPrivateVariables.datapack_rightFileName))
            {
                if(file.open(QIODevice::ReadOnly))
                {
                    //read and load the file
                    const QByteArray &data=file.readAll();

                    if((1+datapack_file_temp.at(index).size()+4+data.size())>=CATCHCHALLENGER_MAX_PACKET_SIZE)
                    {
                        if(BaseServerMasterSendDatapack::compressedExtension.contains(QFileInfo(file).suffix()))
                        {
                            if(ProtocolParsing::compressionTypeServer==ProtocolParsing::CompressionType::None)
                            {
                                DebugClass::debugConsole(std::stringLiteral("The file %1 is over the maximum packet size, but can be compressed, try enable the compression").arg(GlobalServerData::serverPrivateVariables.subDatapackFolder+datapack_file_temp.at(index)));
                                abort();
                            }
                        }
                        else
                        {
                            DebugClass::debugConsole(std::stringLiteral("The file %1 is over the maximum packet size").arg(GlobalServerData::serverPrivateVariables.subDatapackFolder+datapack_file_temp.at(index)));
                            abort();
                        }
                    }

                    //switch the data to correct hash or drop it
                    QCryptographicHash hashFile(QCryptographicHash::Sha224);
                    hashFile.addData(data);
                    Client::DatapackCacheFile cacheFile;
                    cacheFile.partialHash=*reinterpret_cast<const int *>(hashFile.result().constData());
                    Client::datapack_file_hash_cache_sub[datapack_file_temp.at(index)]=cacheFile;

                    hashSub.addData(data);

                    file.close();
                }
                else
                {
                    DebugClass::debugConsole(std::stringLiteral("Stop now! Unable to open the file %1 to do the datapack checksum for the mirror").arg(file.fileName()));
                    abort();
                }
            }
            else
                DebugClass::debugConsole(std::stringLiteral("File excluded because don't match the regex: %1").arg(file.fileName()));
            index++;
        }
        CommonSettingsServer::commonSettingsServer.datapackHashServerSub=hashSub.result();
    }

    DebugClass::debugConsole(std::stringLiteral("%1 file for datapack loaded, base hash: %2, main hash: %3, sub hash: %4").arg(
                                 Client::datapack_file_hash_cache_base.size()+
                                 Client::datapack_file_hash_cache_main.size()+
                                 Client::datapack_file_hash_cache_sub.size()
                                 )
                             .arg(std::string(CommonSettingsCommon::commonSettingsCommon.datapackHashBase.toHex()))
                             .arg(std::string(CommonSettingsServer::commonSettingsServer.datapackHashServerMain.toHex()))
                             .arg(std::string(CommonSettingsServer::commonSettingsServer.datapackHashServerSub.toHex()))
                             );
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
            int index=0;
            while(index<GlobalServerData::serverSettings.max_players)
            {
                Client::simplifiedIdList << index;
                index++;
            }
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
            DebugClass::debugConsole(std::stringLiteral("Visibility: MapVisibilityAlgorithmSelection_Simple"));
        break;
        case MapVisibilityAlgorithmSelection_WithBorder:
            DebugClass::debugConsole(std::stringLiteral("Visibility: MapVisibilityAlgorithmSelection_WithBorder"));
        break;
        case MapVisibilityAlgorithmSelection_None:
            DebugClass::debugConsole(std::stringLiteral("Visibility: MapVisibilityAlgorithmSelection_None"));
        break;
    }
}

void BaseServer::preload_the_bots(const QList<Map_semi> &semi_loaded_map)
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
        const int &botssize=semi_loaded_map.value(index).old_map.bots.size();
        while(sub_index<botssize)
        {
            bots_number++;
            Map_to_send::Bot_Semi bot_Semi=semi_loaded_map.value(index).old_map.bots.at(sub_index);
            if(!bot_Semi.file.endsWith(BaseServer::text_dotxml))
                bot_Semi.file+=BaseServer::text_dotxml;
            loadBotFile(semi_loaded_map.value(index).map->map_file,bot_Semi.file);
            if(botFiles.contains(bot_Semi.file))
                if(botFiles.value(bot_Semi.file).contains(bot_Semi.id))
                {
                    #ifdef DEBUG_MESSAGE_MAP_LOAD
                    CatchChallenger::DebugClass::debugConsole(std::stringLiteral("Bot %1 (%2) at %3 (%4,%5)").arg(bot_Semi.file).arg(bot_Semi.id).arg(semi_loaded_map.value(index).map->map_file).arg(bot_Semi.point.x).arg(bot_Semi.point.y));
                    #endif
                    QHashIterator<uint8_t,QDomElement> i(botFiles.value(bot_Semi.file).value(bot_Semi.id).step);
                    while (i.hasNext()) {
                        i.next();
                        QDomElement step = i.value();
                        if(step.attribute(BaseServer::text_type)==BaseServer::text_shop)
                        {
                            if(!step.hasAttribute(BaseServer::text_shop))
                                CatchChallenger::DebugClass::debugConsole(std::stringLiteral("Has not attribute \"shop\": for bot id: %1 (%2), spawn at: %3 (%4,%5), for step: %6")
                                    .arg(bot_Semi.id).arg(bot_Semi.file).arg(semi_loaded_map.value(index).map->map_file).arg(bot_Semi.point.x).arg(bot_Semi.point.y).arg(i.key()));
                            else
                            {
                                uint32_t shop=step.attribute(BaseServer::text_shop).toUInt(&ok);
                                if(!ok)
                                    CatchChallenger::DebugClass::debugConsole(std::stringLiteral("shop is not a number: for bot id: %1 (%2), spawn at: %3 (%4,%5), for step: %6")
                                        .arg(bot_Semi.id).arg(bot_Semi.file).arg(semi_loaded_map.value(index).map->map_file).arg(bot_Semi.point.x).arg(bot_Semi.point.y).arg(i.key()));
                                else if(!CommonDatapackServerSpec::commonDatapackServerSpec.shops.contains(shop))
                                    CatchChallenger::DebugClass::debugConsole(std::stringLiteral("shop number is not valid shop: for bot id: %1 (%2), spawn at: %3 (%4,%5), for step: %6")
                                        .arg(bot_Semi.id).arg(bot_Semi.file).arg(semi_loaded_map.value(index).map->map_file).arg(bot_Semi.point.x).arg(bot_Semi.point.y).arg(i.key()));
                                else
                                {
                                    #ifdef DEBUG_MESSAGE_MAP_LOAD
                                    CatchChallenger::DebugClass::debugConsole(std::stringLiteral("shop put at: %1 (%2,%3)")
                                        .arg(semi_loaded_map.value(index).map->map_file).arg(bot_Semi.point.x).arg(bot_Semi.point.y));
                                    #endif
                                    static_cast<MapServer *>(semi_loaded_map.value(index).map)->shops.insert(std::pair<uint8_t,uint8_t>(bot_Semi.point.x,bot_Semi.point.y),shop);
                                    shops_number++;
                                }
                            }
                        }
                        else if(step.attribute(BaseServer::text_type)==BaseServer::text_learn)
                        {
                            if(static_cast<MapServer *>(semi_loaded_map.value(index).map)->learn.contains(std::pair<uint8_t,uint8_t>(bot_Semi.point.x,bot_Semi.point.y)))
                                CatchChallenger::DebugClass::debugConsole(std::stringLiteral("learn point already on the map: for bot id: %1 (%2), spawn at: %3 (%4,%5), for step: %6")
                                    .arg(bot_Semi.id).arg(bot_Semi.file).arg(semi_loaded_map.value(index).map->map_file).arg(bot_Semi.point.x).arg(bot_Semi.point.y).arg(i.key()));
                            else
                            {
                                #ifdef DEBUG_MESSAGE_MAP_LOAD
                                CatchChallenger::DebugClass::debugConsole(std::stringLiteral("learn point put at: %1 (%2,%3)")
                                    .arg(semi_loaded_map.value(index).map->map_file).arg(bot_Semi.point.x).arg(bot_Semi.point.y));
                                #endif
                                static_cast<MapServer *>(semi_loaded_map.value(index).map)->learn.insert(std::pair<uint8_t,uint8_t>(bot_Semi.point.x,bot_Semi.point.y));
                                learnpoint_number++;
                            }
                        }
                        else if(step.attribute(BaseServer::text_type)==BaseServer::text_heal)
                        {
                            if(static_cast<MapServer *>(semi_loaded_map.value(index).map)->heal.contains(std::pair<uint8_t,uint8_t>(bot_Semi.point.x,bot_Semi.point.y)))
                                CatchChallenger::DebugClass::debugConsole(std::stringLiteral("heal point already on the map: for bot id: %1 (%2), spawn at: %3 (%4,%5), for step: %6")
                                    .arg(bot_Semi.id).arg(bot_Semi.file).arg(semi_loaded_map.value(index).map->map_file).arg(bot_Semi.point.x).arg(bot_Semi.point.y).arg(i.key()));
                            else
                            {
                                #ifdef DEBUG_MESSAGE_MAP_LOAD
                                CatchChallenger::DebugClass::debugConsole(std::stringLiteral("heal point put at: %1 (%2,%3)")
                                    .arg(semi_loaded_map.value(index).map->map_file).arg(bot_Semi.point.x).arg(bot_Semi.point.y));
                                #endif
                                static_cast<MapServer *>(semi_loaded_map.value(index).map)->heal.insert(std::pair<uint8_t,uint8_t>(bot_Semi.point.x,bot_Semi.point.y));
                                healpoint_number++;
                            }
                        }
                        else if(step.attribute(BaseServer::text_type)==BaseServer::text_market)
                        {
                            if(static_cast<MapServer *>(semi_loaded_map.value(index).map)->market.contains(std::pair<uint8_t,uint8_t>(bot_Semi.point.x,bot_Semi.point.y)))
                                CatchChallenger::DebugClass::debugConsole(std::stringLiteral("market point already on the map: for bot id: %1 (%2), spawn at: %3 (%4,%5), for step: %6")
                                    .arg(bot_Semi.id).arg(bot_Semi.file).arg(semi_loaded_map.value(index).map->map_file).arg(bot_Semi.point.x).arg(bot_Semi.point.y).arg(i.key()));
                            else
                            {
                                #ifdef DEBUG_MESSAGE_MAP_LOAD
                                CatchChallenger::DebugClass::debugConsole(std::stringLiteral("market point put at: %1 (%2,%3)")
                                    .arg(semi_loaded_map.value(index).map->map_file).arg(bot_Semi.point.x).arg(bot_Semi.point.y));
                                #endif
                                static_cast<MapServer *>(semi_loaded_map.value(index).map)->market.insert(std::pair<uint8_t,uint8_t>(bot_Semi.point.x,bot_Semi.point.y));
                                marketpoint_number++;
                            }
                        }
                        else if(step.attribute(BaseServer::text_type)==BaseServer::text_zonecapture)
                        {
                            if(!step.hasAttribute(BaseServer::text_zone))
                                CatchChallenger::DebugClass::debugConsole(std::stringLiteral("zonecapture point have not the zone attribute: for bot id: %1 (%2), spawn at: %3 (%4,%5), for step: %6")
                                    .arg(bot_Semi.id).arg(bot_Semi.file).arg(semi_loaded_map.value(index).map->map_file).arg(bot_Semi.point.x).arg(bot_Semi.point.y).arg(i.key()));
                            else if(static_cast<MapServer *>(semi_loaded_map.value(index).map)->zonecapture.contains(std::pair<uint8_t,uint8_t>(bot_Semi.point.x,bot_Semi.point.y)))
                                CatchChallenger::DebugClass::debugConsole(std::stringLiteral("zonecapture point already on the map: for bot id: %1 (%2), spawn at: %3 (%4,%5), for step: %6")
                                    .arg(bot_Semi.id).arg(bot_Semi.file).arg(semi_loaded_map.value(index).map->map_file).arg(bot_Semi.point.x).arg(bot_Semi.point.y).arg(i.key()));
                            else
                            {
                                #ifdef DEBUG_MESSAGE_MAP_LOAD
                                CatchChallenger::DebugClass::debugConsole(std::stringLiteral("zonecapture point put at: %1 (%2,%3)")
                                    .arg(semi_loaded_map.value(index).map->map_file).arg(bot_Semi.point.x).arg(bot_Semi.point.y));
                                #endif
                                static_cast<MapServer *>(semi_loaded_map[index].map)->zonecapture[std::pair<uint8_t,uint8_t>(bot_Semi.point.x,bot_Semi.point.y)]=step.attribute(BaseServer::text_zone);
                                zonecapturepoint_number++;
                            }
                        }
                        else if(step.attribute(BaseServer::text_type)==BaseServer::text_fight)
                        {
                            if(static_cast<MapServer *>(semi_loaded_map.value(index).map)->botsFight.contains(std::pair<uint8_t,uint8_t>(bot_Semi.point.x,bot_Semi.point.y)))
                                CatchChallenger::DebugClass::debugConsole(std::stringLiteral("botsFight point already on the map: for bot id: %1 (%2), spawn at: %3 (%4,%5), for step: %6")
                                    .arg(bot_Semi.id).arg(bot_Semi.file).arg(semi_loaded_map.value(index).map->map_file).arg(bot_Semi.point.x).arg(bot_Semi.point.y).arg(i.key()));
                            else
                            {
                                const uint32_t &fightid=step.attribute(BaseServer::text_fightid).toUInt(&ok);
                                if(ok)
                                {
                                    if(CommonDatapackServerSpec::commonDatapackServerSpec.botFights.contains(fightid))
                                    {
                                        if(bot_Semi.property_text.contains(BaseServer::text_lookAt))
                                        {
                                            Direction direction;
                                            if(bot_Semi.property_text.value(BaseServer::text_lookAt)==BaseServer::text_left)
                                                direction=CatchChallenger::Direction_move_at_left;
                                            else if(bot_Semi.property_text.value(BaseServer::text_lookAt)==BaseServer::text_right)
                                                direction=CatchChallenger::Direction_move_at_right;
                                            else if(bot_Semi.property_text.value(BaseServer::text_lookAt)==BaseServer::text_top)
                                                direction=CatchChallenger::Direction_move_at_top;
                                            else
                                            {
                                                if(bot_Semi.property_text.value(BaseServer::text_lookAt)!=BaseServer::text_bottom)
                                                    CatchChallenger::DebugClass::debugConsole(std::stringLiteral("Wrong direction for the bot at %1 (%2,%3)")
                                                        .arg(semi_loaded_map.value(index).map->map_file).arg(bot_Semi.point.x).arg(bot_Semi.point.y));
                                                direction=CatchChallenger::Direction_move_at_bottom;
                                            }
                                            #ifdef DEBUG_MESSAGE_MAP_LOAD
                                            CatchChallenger::DebugClass::debugConsole(std::stringLiteral("botsFight point put at: %1 (%2,%3)")
                                                .arg(semi_loaded_map.value(index).map->map_file).arg(bot_Semi.point.x).arg(bot_Semi.point.y));
                                            #endif
                                            static_cast<MapServer *>(semi_loaded_map[index].map)->botsFight.insert(std::pair<uint8_t,uint8_t>(bot_Semi.point.x,bot_Semi.point.y),fightid);
                                            botfights_number++;

                                            uint32_t fightRange=5;
                                            if(bot_Semi.property_text.contains(BaseServer::text_fightRange))
                                            {
                                                fightRange=bot_Semi.property_text.value(BaseServer::text_fightRange).toUInt(&ok);
                                                if(!ok)
                                                {
                                                    CatchChallenger::DebugClass::debugConsole(std::stringLiteral("fightRange is not a number at %1 (%2,%3): %4")
                                                        .arg(semi_loaded_map.value(index).map->map_file).arg(bot_Semi.point.x).arg(bot_Semi.point.y)
                                                        .arg(bot_Semi.property_text.value(BaseServer::text_fightRange).toString()));
                                                    fightRange=5;
                                                }
                                                else
                                                {
                                                    if(fightRange>10)
                                                    {
                                                        CatchChallenger::DebugClass::debugConsole(std::stringLiteral("fightRange is greater than 10 at %1 (%2,%3): %4")
                                                            .arg(semi_loaded_map.value(index).map->map_file).arg(bot_Semi.point.x).arg(bot_Semi.point.y)
                                                            .arg(fightRange)
                                                            );
                                                        fightRange=5;
                                                    }
                                                }
                                            }
                                            //load the botsFightTrigger
                                            #ifdef DEBUG_MESSAGE_CLIENT_FIGHT_BOT
                                            CatchChallenger::DebugClass::debugConsole(std::stringLiteral("Put bot fight point %1 at %2 (%3,%4) in direction: %5").arg(fightid).arg(semi_loaded_map.value(index).map->map_file).arg(bot_Semi.point.x).arg(bot_Semi.point.y).arg(direction));
                                            #endif
                                            uint8_t temp_x=bot_Semi.point.x,temp_y=bot_Semi.point.y;
                                            uint32_t index_botfight_range=0;
                                            CatchChallenger::CommonMap *map=semi_loaded_map.value(index).map;
                                            CatchChallenger::CommonMap *old_map=map;
                                            while(index_botfight_range<fightRange)
                                            {
                                                if(!CatchChallenger::MoveOnTheMap::canGoTo(direction,*map,temp_x,temp_y,true,false))
                                                    break;
                                                if(!CatchChallenger::MoveOnTheMap::move(direction,&map,&temp_x,&temp_y,true,false))
                                                    break;
                                                if(map!=old_map)
                                                    break;
                                                static_cast<MapServer *>(semi_loaded_map[index].map)->botsFightTrigger.insert(std::pair<uint8_t,uint8_t>(temp_x,temp_y),fightid);
                                                index_botfight_range++;
                                                botfightstigger_number++;
                                            }
                                        }
                                        else
                                            DebugClass::debugConsole(std::stringLiteral("lookAt not found: %1 at %2(%3,%4)").arg(shops_number).arg(semi_loaded_map.value(index).map->map_file).arg(bot_Semi.point.x).arg(bot_Semi.point.y));
                                    }
                                    else
                                        DebugClass::debugConsole(std::stringLiteral("fightid not found into the list: %1 at %2(%3,%4)").arg(shops_number).arg(semi_loaded_map.value(index).map->map_file).arg(bot_Semi.point.x).arg(bot_Semi.point.y));
                                }
                                else
                                    DebugClass::debugConsole(std::stringLiteral("botsFight point have wrong fightid: %1 at %2(%3,%4)").arg(shops_number).arg(semi_loaded_map.value(index).map->map_file).arg(bot_Semi.point.x).arg(bot_Semi.point.y));
                            }
                        }
                    }
                }
            sub_index++;
        }
        index++;
    }
    botIdLoaded.clear();

    DebugClass::debugConsole(std::stringLiteral("%1 learn point(s) on map loaded").arg(learnpoint_number));
    DebugClass::debugConsole(std::stringLiteral("%1 zonecapture point(s) on map loaded").arg(zonecapturepoint_number));
    DebugClass::debugConsole(std::stringLiteral("%1 heal point(s) on map loaded").arg(healpoint_number));
    DebugClass::debugConsole(std::stringLiteral("%1 market point(s) on map loaded").arg(marketpoint_number));
    DebugClass::debugConsole(std::stringLiteral("%1 bot fight(s) on map loaded").arg(botfights_number));
    DebugClass::debugConsole(std::stringLiteral("%1 bot fights tigger(s) on map loaded").arg(botfightstigger_number));
    DebugClass::debugConsole(std::stringLiteral("%1 shop(s) on map loaded").arg(shops_number));
    DebugClass::debugConsole(std::stringLiteral("%1 bots(s) on map loaded").arg(bots_number));
}
