#include "BaseServer.hpp"
#include "DictionaryServer.hpp"
#include "GlobalServerData.hpp"

using namespace CatchChallenger;

void BaseServer::preload_map_semi_after_db_id()
{
    if(DictionaryServer::dictionary_map_database_to_internal.size()==0)
    {
        std::cerr << "Need be called after preload_dictionary_map()" << std::endl;
        abort();
    }

    uint16_t datapack_index_temp_for_item=0;
    uint16_t datapack_index_temp_for_plant=0;
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
                if(DictionaryServer::dictionary_pointOnMap_plant_internal_to_database.find(sortFileName)!=DictionaryServer::dictionary_pointOnMap_plant_internal_to_database.end())
                {
                    const std::map<std::pair<uint8_t/*x*/,uint8_t/*y*/>,uint16_t/*db code*/> &subItem=DictionaryServer::dictionary_pointOnMap_plant_internal_to_database.at(sortFileName);
                    if(subItem.find(pair)!=subItem.end())
                        found=true;
                }
                if(found)
                    pointOnMapDbCode=DictionaryServer::dictionary_pointOnMap_plant_internal_to_database.at(sortFileName).at(pair);
                else
                {
                    dictionary_pointOnMap_maxId_plant++;
                    std::string queryText;
                    switch(GlobalServerData::serverPrivateVariables.db_server->databaseType())
                    {
                        default:
                        #if defined(CATCHCHALLENGER_DB_MYSQL) || (not defined(EPOLLCATCHCHALLENGERSERVER))
                        case DatabaseBase::DatabaseType::Mysql:
                            queryText="INSERT INTO `dictionary_pointonmap_plant`(`id`,`map`,`x`,`y`) VALUES("+
                                    std::to_string(dictionary_pointOnMap_maxId_plant)+","+
                                    std::to_string(mapServer->reverse_db_id)+","+
                                    std::to_string(dirt.point.x)+","+
                                    std::to_string(dirt.point.y)+");"
                                    ;
                        break;
                        #endif
                        #ifndef EPOLLCATCHCHALLENGERSERVER
                        case DatabaseBase::DatabaseType::SQLite:
                            queryText="INSERT INTO dictionary_pointonmap_plant(id,map,x,y) VALUES("+
                                    std::to_string(dictionary_pointOnMap_maxId_plant)+","+
                                    std::to_string(mapServer->reverse_db_id)+","+
                                    std::to_string(dirt.point.x)+","+
                                    std::to_string(dirt.point.y)+");"
                                    ;
                        break;
                        #endif
                        #if not defined(EPOLLCATCHCHALLENGERSERVER) || defined(CATCHCHALLENGER_DB_POSTGRESQL)
                        case DatabaseBase::DatabaseType::PostgreSQL:
                            queryText="INSERT INTO dictionary_pointonmap_plant(id,map,x,y) VALUES("+
                                    std::to_string(dictionary_pointOnMap_maxId_plant)+","+
                                    std::to_string(mapServer->reverse_db_id)+","+
                                    std::to_string(dirt.point.x)+","+
                                    std::to_string(dirt.point.y)+");"
                                    ;
                        break;
                        #endif
                    }
                    if(!GlobalServerData::serverPrivateVariables.db_server->asyncWrite(queryText))
                    {
                        std::cerr << "Sql error for: " << queryText << ", error: " << GlobalServerData::serverPrivateVariables.db_server->errorMessage() << std::endl;
                        criticalDatabaseQueryFailed();abort();//stop because can't resolv the name
                    }
                    DictionaryServer::dictionary_pointOnMap_plant_internal_to_database[sortFileName]
                            [std::pair<uint8_t,uint8_t>(dirt.point.x,dirt.point.y)]=static_cast<uint16_t>(dictionary_pointOnMap_maxId_plant);
                    pointOnMapDbCode=static_cast<uint16_t>(dictionary_pointOnMap_maxId_plant);
                }

                MapServer::PlantOnMap plantOnMap;
                plantOnMap.pointOnMapDbCode=pointOnMapDbCode;
                mapServer->plants[pair]=plantOnMap;

                {
                    while((uint32_t)DictionaryServer::dictionary_pointOnMap_plant_database_to_internal.size()<=pointOnMapDbCode)
                    {
                        DictionaryServer::MapAndPointPlant mapAndPoint;
                        mapAndPoint.map=NULL;
                        mapAndPoint.x=0;
                        mapAndPoint.y=0;
                        mapAndPoint.datapack_index_plant=0;
                        DictionaryServer::dictionary_pointOnMap_plant_database_to_internal.push_back(mapAndPoint);
                    }
                    DictionaryServer::MapAndPointPlant &mapAndPoint=DictionaryServer::dictionary_pointOnMap_plant_database_to_internal[pointOnMapDbCode];
                    mapAndPoint.map=mapServer;
                    mapAndPoint.x=dirt.point.x;
                    mapAndPoint.y=dirt.point.y;
                    mapAndPoint.datapack_index_plant=datapack_index_temp_for_plant;
                    datapack_index_temp_for_plant++;
                }
                /*SEGFAULTstd::cout << __FILE__ << ":" << __LINE__ << " DictionaryServer::dictionary_pointOnMap_item_database_to_internal: " << DictionaryServer::dictionary_pointOnMap_item_database_to_internal.size() << std::endl;
                for(unsigned int i=0; i<DictionaryServer::dictionary_pointOnMap_item_database_to_internal.size(); i++)
                {
                    const DictionaryServer::MapAndPointItem &t=DictionaryServer::dictionary_pointOnMap_item_database_to_internal.at(i);
                    std::cerr << t.datapack_index_item << " " << t.map->id << " " << t.x << " " << t.y << " " << std::endl;
                }*/
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
                if(DictionaryServer::dictionary_pointOnMap_item_internal_to_database.find(sortFileName)!=DictionaryServer::dictionary_pointOnMap_item_internal_to_database.end())
                {
                    const std::map<std::pair<uint8_t/*x*/,uint8_t/*y*/>,uint16_t/*db code*/> &subItem=DictionaryServer::dictionary_pointOnMap_item_internal_to_database.at(sortFileName);
                    if(subItem.find(pair)!=subItem.end())
                        found=true;
                }
                if(found)
                    pointOnMapDbCode=DictionaryServer::dictionary_pointOnMap_item_internal_to_database.at(sortFileName).at(pair);
                else
                {
                    dictionary_pointOnMap_maxId_item++;
                    std::string queryText;
                    switch(GlobalServerData::serverPrivateVariables.db_server->databaseType())
                    {
                        default:
                        #if defined(CATCHCHALLENGER_DB_MYSQL) || (not defined(EPOLLCATCHCHALLENGERSERVER))
                        case DatabaseBase::DatabaseType::Mysql:
                            queryText="INSERT INTO `dictionary_pointonmap_item`(`id`,`map`,`x`,`y`) VALUES("+
                                    std::to_string(dictionary_pointOnMap_maxId_item)+","+
                                    std::to_string(mapServer->reverse_db_id)+","+
                                    std::to_string(item.point.x)+","+
                                    std::to_string(item.point.y)+");"
                                    ;
                        break;
                        #endif
                        #ifndef EPOLLCATCHCHALLENGERSERVER
                        case DatabaseBase::DatabaseType::SQLite:
                            queryText="INSERT INTO dictionary_pointonmap_item(id,map,x,y) VALUES("+
                                    std::to_string(dictionary_pointOnMap_maxId_item)+","+
                                    std::to_string(mapServer->reverse_db_id)+","+
                                    std::to_string(item.point.x)+","+
                                    std::to_string(item.point.y)+");"
                                    ;
                        break;
                        #endif
                        #if not defined(EPOLLCATCHCHALLENGERSERVER) || defined(CATCHCHALLENGER_DB_POSTGRESQL)
                        case DatabaseBase::DatabaseType::PostgreSQL:
                            queryText="INSERT INTO dictionary_pointonmap_item(id,map,x,y) VALUES("+
                                    std::to_string(dictionary_pointOnMap_maxId_item)+","+
                                    std::to_string(mapServer->reverse_db_id)+","+
                                    std::to_string(item.point.x)+","+
                                    std::to_string(item.point.y)+");"
                                    ;
                        break;
                        #endif
                    }
                    if(!GlobalServerData::serverPrivateVariables.db_server->asyncWrite(queryText))
                    {
                        std::cerr << "Sql error for: " << queryText << ", error: " << GlobalServerData::serverPrivateVariables.db_server->errorMessage() << std::endl;
                        criticalDatabaseQueryFailed();abort();//stop because can't resolv the name
                    }
                    DictionaryServer::dictionary_pointOnMap_item_internal_to_database[sortFileName]
                            [std::pair<uint8_t,uint8_t>(item.point.x,item.point.y)]=static_cast<uint16_t>(dictionary_pointOnMap_maxId_item);
                    pointOnMapDbCode=static_cast<uint16_t>(dictionary_pointOnMap_maxId_item);
                }

                MapServer::ItemOnMap itemOnMap;
                itemOnMap.infinite=item.infinite;
                itemOnMap.item=item.item;
                itemOnMap.pointOnMapDbCode=pointOnMapDbCode;
                mapServer->pointOnMap_Item[pair]=itemOnMap;

                {
                    while((uint32_t)DictionaryServer::dictionary_pointOnMap_item_database_to_internal.size()<=pointOnMapDbCode)
                    {
                        DictionaryServer::MapAndPointItem mapAndPoint;
                        mapAndPoint.map=NULL;
                        mapAndPoint.x=0;
                        mapAndPoint.y=0;
                        mapAndPoint.datapack_index_item=0;
                        DictionaryServer::dictionary_pointOnMap_item_database_to_internal.push_back(mapAndPoint);
                    }
                    DictionaryServer::MapAndPointItem &mapAndPoint=DictionaryServer::dictionary_pointOnMap_item_database_to_internal[pointOnMapDbCode];
                    mapAndPoint.map=mapServer;
                    mapAndPoint.x=item.point.x;
                    mapAndPoint.y=item.point.y;
                    mapAndPoint.datapack_index_item=datapack_index_temp_for_item;
                    /*std::cerr << "DictionaryServer::dictionary_pointOnMap_item_database_to_internal[" << pointOnMapDbCode << "] "
                        << mapAndPoint.datapack_index_item << " " << mapAndPoint.map->id << " " << std::to_string(mapAndPoint.x) << " " << std::to_string(mapAndPoint.y) << " " << std::endl;*/
                    datapack_index_temp_for_item++;
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
