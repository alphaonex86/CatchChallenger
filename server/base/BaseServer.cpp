#include "BaseServer.h"
#include "GlobalServerData.h"
#include "../../general/base/FacilityLib.h"
#include "../../general/base/CommonDatapack.h"
#include "../../general/base/DatapackGeneralLoader.h"

#include <QFile>
#include <QByteArray>

using namespace CatchChallenger;

BaseServer::BaseServer()
{
    ProtocolParsing::initialiseTheVariable();

    qRegisterMetaType<Chat_type>("Chat_type");
    qRegisterMetaType<Player_internal_informations>("Player_internal_informations");
    qRegisterMetaType<QList<quint64> >("QList<quint64>");
    qRegisterMetaType<Orientation>("Orientation");
    qRegisterMetaType<QList<QByteArray> >("QList<QByteArray>");
    qRegisterMetaType<Chat_type>("Chat_type");
    qRegisterMetaType<QAbstractSocket::SocketError>("QAbstractSocket::SocketError");
    qRegisterMetaType<Map_server_MapVisibility_simple*>("Map_server_MapVisibility_simple*");
    qRegisterMetaType<CatchChallenger::Player_public_informations>("CatchChallenger::Player_public_informations");
    qRegisterMetaType<QSqlQuery>("QSqlQuery");
    qRegisterMetaType<QAbstractSocket::SocketState>("QAbstractSocket::SocketState");
    qRegisterMetaType<CatchChallenger::Player_private_and_public_informations>("CatchChallenger::Player_private_and_public_informations");
    qRegisterMetaType<CatchChallenger::Direction>("CatchChallenger::Direction");
    qRegisterMetaType<Player_private_and_public_informations>("Player_private_and_public_informations");
    qRegisterMetaType<Direction>("Direction");
    qRegisterMetaType<QuestAction>("QuestAction");
    qRegisterMetaType<QList<QPair<quint32,qint32> > >("QList<QPair<quint32,qint32> >");
    qRegisterMetaType<QList<qint32> >("QList<quint32>");

    GlobalServerData::serverPrivateVariables.connected_players	= 0;
    GlobalServerData::serverPrivateVariables.number_of_bots_logged= 0;
    GlobalServerData::serverPrivateVariables.db                   = NULL;
    GlobalServerData::serverPrivateVariables.timer_player_map     = NULL;

    GlobalServerData::serverPrivateVariables.botSpawnIndex=0;
    GlobalServerData::serverPrivateVariables.datapack_basePath		= QCoreApplication::applicationDirPath()+"/datapack/";
    GlobalServerData::serverPrivateVariables.datapack_rightFileName	= QRegExp(DATAPACK_FILE_REGEX);

    GlobalServerData::serverPrivateVariables.timer_to_send_insert_move_remove.start(CATCHCHALLENGER_SERVER_MAP_TIME_TO_SEND_MOVEMENT);

    GlobalServerData::serverSettings.max_players=1;
    GlobalServerData::serverSettings.tolerantMode=false;
    GlobalServerData::serverSettings.commmonServerSettings.sendPlayerNumber = false;

    GlobalServerData::serverSettings.database.type=CatchChallenger::ServerSettings::Database::DatabaseType_SQLite;
    GlobalServerData::serverSettings.database.sqlite.file="";
    GlobalServerData::serverSettings.mapVisibility.mapVisibilityAlgorithm	= CatchChallenger::MapVisibilityAlgorithm_none;

    //to do a bug
    GlobalServerData::serverSettings.rates_gold_premium=1;
    GlobalServerData::serverSettings.rates_shiny_premium=1;
    GlobalServerData::serverSettings.rates_xp_premium=1;
    GlobalServerData::serverSettings.server_ip="";
    GlobalServerData::serverSettings.server_port=42489;
    GlobalServerData::serverSettings.commmonServerSettings.chat_allow_aliance=true;
    GlobalServerData::serverSettings.commmonServerSettings.chat_allow_clan=true;
    GlobalServerData::serverSettings.commmonServerSettings.chat_allow_local=true;
    GlobalServerData::serverSettings.commmonServerSettings.chat_allow_all=true;
    GlobalServerData::serverSettings.commmonServerSettings.chat_allow_private=true;
    GlobalServerData::serverSettings.commmonServerSettings.pvp=true;
    GlobalServerData::serverSettings.commmonServerSettings.rates_gold=1;
    GlobalServerData::serverSettings.commmonServerSettings.rates_shiny=1;
    GlobalServerData::serverSettings.commmonServerSettings.rates_xp=1;
    GlobalServerData::serverSettings.commmonServerSettings.sendPlayerNumber=true;
    GlobalServerData::serverSettings.database.type=ServerSettings::Database::DatabaseType_Mysql;
    GlobalServerData::serverSettings.database.fightSync=ServerSettings::Database::FightSync_AtTheEndOfBattle;
    GlobalServerData::serverSettings.database.positionTeleportSync=true;
    GlobalServerData::serverSettings.database.secondToPositionSync=0;
    GlobalServerData::serverSettings.mapVisibility.mapVisibilityAlgorithm	= MapVisibilityAlgorithm_simple;
    GlobalServerData::serverSettings.mapVisibility.simple.max		= 30;
    GlobalServerData::serverSettings.mapVisibility.simple.reshow		= 20;

    stat=Down;

    connect(&QFakeServer::server,SIGNAL(newConnection()),this,SLOT(newConnection()),Qt::QueuedConnection);
    connect(this,SIGNAL(need_be_started()),this,SLOT(start_internal_server()),Qt::QueuedConnection);
    connect(this,SIGNAL(try_stop_server()),this,SLOT(stop_internal_server()),Qt::QueuedConnection);
    connect(this,SIGNAL(try_initAll()),this,SLOT(initAll()),Qt::QueuedConnection);
    emit try_initAll();

    srand(time(NULL));
}

/** call only when the server is down
 * \warning this function is thread safe because it quit all thread before remove */
BaseServer::~BaseServer()
{
    GlobalServerData::serverPrivateVariables.stopIt=true;
    GlobalServerData::serverPrivateVariables.eventThreaderList.clear();
    closeDB();
}

void BaseServer::closeDB()
{
    if(GlobalServerData::serverPrivateVariables.db!=NULL)
    {
        GlobalServerData::serverPrivateVariables.db->commit();
        GlobalServerData::serverPrivateVariables.db->close();
        QString connectionName=GlobalServerData::serverPrivateVariables.db->connectionName();
        delete GlobalServerData::serverPrivateVariables.db;
        GlobalServerData::serverPrivateVariables.db=NULL;
        QSqlDatabase::removeDatabase(connectionName);
    }
}

void BaseServer::initAll()
{
    /// \bug QObject::moveToThread: Current thread (0x79e1f0) is not the object's thread (0x6b4690)
    GlobalServerData::serverPrivateVariables.player_updater.moveToThread(GlobalServerData::serverPrivateVariables.eventThreaderList.at(0));
    BroadCastWithoutSender::broadCastWithoutSender.moveToThread(GlobalServerData::serverPrivateVariables.eventThreaderList.at(0));
}

//////////////////////////////////////////// server starting //////////////////////////////////////

void BaseServer::preload_the_data()
{
    GlobalServerData::serverPrivateVariables.stopIt=false;

    CommonDatapack::commonDatapack.parseDatapack(GlobalServerData::serverPrivateVariables.datapack_basePath);
    preload_the_datapack();
    preload_the_skin();
    preload_shop();
    preload_the_players();
    preload_monsters_drops();
    load_monsters_max_id();
    preload_the_map();
    preload_the_plant_on_map();
    check_monsters_map();
    preload_the_visibility_algorithm();
}

void BaseServer::preload_the_map()
{
    GlobalServerData::serverPrivateVariables.datapack_mapPath=GlobalServerData::serverPrivateVariables.datapack_basePath+DATAPACK_BASE_PATH_MAP;
    #ifdef DEBUG_MESSAGE_MAP_LOAD
    DebugClass::debugConsole(QString("start preload the map, into: %1").arg(GlobalServerData::serverPrivateVariables.datapack_mapPath));
    #endif
    Map_loader map_temp;
    QList<Map_semi> semi_loaded_map;
    QStringList map_name;
    QStringList full_map_name;
    QStringList returnList=FacilityLib::listFolder(GlobalServerData::serverPrivateVariables.datapack_mapPath);

    //load the map
    int size=returnList.size();
    int index=0;
    int sub_index;
    QRegExp mapFilter("\\.tmx$");
    while(index<size)
    {
        QString fileName=returnList.at(index);
        fileName.replace('\\','/');
        if(fileName.contains(mapFilter) && GlobalServerData::serverPrivateVariables.filesList.contains(DATAPACK_BASE_PATH_MAP+fileName))
        {
            #ifdef DEBUG_MESSAGE_MAP_LOAD
            DebugClass::debugConsole(QString("load the map: %1").arg(fileName));
            #endif
            full_map_name << fileName;
            if(map_temp.tryLoadMap(GlobalServerData::serverPrivateVariables.datapack_mapPath+fileName))
            {
                switch(GlobalServerData::serverSettings.mapVisibility.mapVisibilityAlgorithm)
                {
                    case MapVisibilityAlgorithm_simple:
                        GlobalServerData::serverPrivateVariables.map_list[fileName]=new Map_server_MapVisibility_simple;
                    break;
                    case MapVisibilityAlgorithm_none:
                    default:
                        GlobalServerData::serverPrivateVariables.map_list[fileName]=new MapServer;
                    break;
                }

                GlobalServerData::serverPrivateVariables.map_list[fileName]->width			= map_temp.map_to_send.width;
                GlobalServerData::serverPrivateVariables.map_list[fileName]->height			= map_temp.map_to_send.height;
                GlobalServerData::serverPrivateVariables.map_list[fileName]->parsed_layer	= map_temp.map_to_send.parsed_layer;
                GlobalServerData::serverPrivateVariables.map_list[fileName]->map_file			= fileName;
                GlobalServerData::serverPrivateVariables.map_list[fileName]->grassMonster     = map_temp.map_to_send.grassMonster;
                GlobalServerData::serverPrivateVariables.map_list[fileName]->waterMonster     = map_temp.map_to_send.waterMonster;
                GlobalServerData::serverPrivateVariables.map_list[fileName]->caveMonster     = map_temp.map_to_send.caveMonster;

                map_name << fileName;

                parseJustLoadedMap(map_temp.map_to_send,fileName);

                Map_semi map_semi;
                map_semi.map				= GlobalServerData::serverPrivateVariables.map_list[fileName];

                if(!map_temp.map_to_send.border.top.fileName.isEmpty())
                {
                    map_semi.border.top.fileName		= Map_loader::resolvRelativeMap(GlobalServerData::serverPrivateVariables.datapack_mapPath+fileName,map_temp.map_to_send.border.top.fileName,GlobalServerData::serverPrivateVariables.datapack_mapPath);
                    map_semi.border.top.x_offset		= map_temp.map_to_send.border.top.x_offset;
                }
                if(!map_temp.map_to_send.border.bottom.fileName.isEmpty())
                {
                    map_semi.border.bottom.fileName		= Map_loader::resolvRelativeMap(GlobalServerData::serverPrivateVariables.datapack_mapPath+fileName,map_temp.map_to_send.border.bottom.fileName,GlobalServerData::serverPrivateVariables.datapack_mapPath);
                    map_semi.border.bottom.x_offset		= map_temp.map_to_send.border.bottom.x_offset;
                }
                if(!map_temp.map_to_send.border.left.fileName.isEmpty())
                {
                    map_semi.border.left.fileName		= Map_loader::resolvRelativeMap(GlobalServerData::serverPrivateVariables.datapack_mapPath+fileName,map_temp.map_to_send.border.left.fileName,GlobalServerData::serverPrivateVariables.datapack_mapPath);
                    map_semi.border.left.y_offset		= map_temp.map_to_send.border.left.y_offset;
                }
                if(!map_temp.map_to_send.border.right.fileName.isEmpty())
                {
                    map_semi.border.right.fileName		= Map_loader::resolvRelativeMap(GlobalServerData::serverPrivateVariables.datapack_mapPath+fileName,map_temp.map_to_send.border.right.fileName,GlobalServerData::serverPrivateVariables.datapack_mapPath);
                    map_semi.border.right.y_offset		= map_temp.map_to_send.border.right.y_offset;
                }

                sub_index=0;
                while(sub_index<map_temp.map_to_send.teleport.size())
                {
                    map_temp.map_to_send.teleport[sub_index].map=Map_loader::resolvRelativeMap(GlobalServerData::serverPrivateVariables.datapack_mapPath+fileName,map_temp.map_to_send.teleport.at(sub_index).map,GlobalServerData::serverPrivateVariables.datapack_mapPath);
                    sub_index++;
                }

                map_semi.old_map=map_temp.map_to_send;

                semi_loaded_map << map_semi;
            }
            else
                DebugClass::debugConsole(QString("error at loading: %1, error: %2").arg(fileName).arg(map_temp.errorString()));
        }
        index++;
    }
    full_map_name.sort();

    //resolv the border map name into their pointer
    size=semi_loaded_map.size();
    index=0;
    while(index<size)
    {
        if(semi_loaded_map.at(index).border.bottom.fileName!="" && GlobalServerData::serverPrivateVariables.map_list.contains(semi_loaded_map.at(index).border.bottom.fileName))
            semi_loaded_map[index].map->border.bottom.map=GlobalServerData::serverPrivateVariables.map_list[semi_loaded_map.at(index).border.bottom.fileName];
        else
            semi_loaded_map[index].map->border.bottom.map=NULL;

        if(semi_loaded_map.at(index).border.top.fileName!="" && GlobalServerData::serverPrivateVariables.map_list.contains(semi_loaded_map.at(index).border.top.fileName))
            semi_loaded_map[index].map->border.top.map=GlobalServerData::serverPrivateVariables.map_list[semi_loaded_map.at(index).border.top.fileName];
        else
            semi_loaded_map[index].map->border.top.map=NULL;

        if(semi_loaded_map.at(index).border.left.fileName!="" && GlobalServerData::serverPrivateVariables.map_list.contains(semi_loaded_map.at(index).border.left.fileName))
            semi_loaded_map[index].map->border.left.map=GlobalServerData::serverPrivateVariables.map_list[semi_loaded_map.at(index).border.left.fileName];
        else
            semi_loaded_map[index].map->border.left.map=NULL;

        if(semi_loaded_map.at(index).border.right.fileName!="" && GlobalServerData::serverPrivateVariables.map_list.contains(semi_loaded_map.at(index).border.right.fileName))
            semi_loaded_map[index].map->border.right.map=GlobalServerData::serverPrivateVariables.map_list[semi_loaded_map.at(index).border.right.fileName];
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
        while(sub_index<semi_loaded_map[index].old_map.teleport.size())
        {
            if(GlobalServerData::serverPrivateVariables.map_list.contains(semi_loaded_map[index].old_map.teleport.at(sub_index).map))
            {
                if(semi_loaded_map[index].old_map.teleport.at(sub_index).destination_x<GlobalServerData::serverPrivateVariables.map_list[semi_loaded_map[index].old_map.teleport.at(sub_index).map]->width
                        && semi_loaded_map[index].old_map.teleport.at(sub_index).destination_y<GlobalServerData::serverPrivateVariables.map_list[semi_loaded_map[index].old_map.teleport.at(sub_index).map]->height)
                {
                    int virtual_position=semi_loaded_map[index].old_map.teleport.at(sub_index).source_x+semi_loaded_map[index].old_map.teleport.at(sub_index).source_y*semi_loaded_map[index].map->width;
                    if(semi_loaded_map[index].map->teleporter.contains(virtual_position))
                    {
                        DebugClass::debugConsole(QString("already found teleporter on the map: %1(%2,%3), to %4 (%5,%6)")
                             .arg(semi_loaded_map[index].map->map_file)
                             .arg(semi_loaded_map[index].old_map.teleport.at(sub_index).source_x)
                             .arg(semi_loaded_map[index].old_map.teleport.at(sub_index).source_y)
                             .arg(semi_loaded_map[index].old_map.teleport.at(sub_index).map)
                             .arg(semi_loaded_map[index].old_map.teleport.at(sub_index).destination_x)
                             .arg(semi_loaded_map[index].old_map.teleport.at(sub_index).destination_y));
                    }
                    else
                    {
                        #ifdef DEBUG_MESSAGE_MAP_LOAD
                        DebugClass::debugConsole(QString("teleporter on the map: %1(%2,%3), to %4 (%5,%6)")
                                     .arg(semi_loaded_map[index].map->map_file)
                                     .arg(semi_loaded_map[index].old_map.teleport.at(sub_index).source_x)
                                     .arg(semi_loaded_map[index].old_map.teleport.at(sub_index).source_y)
                                     .arg(semi_loaded_map[index].old_map.teleport.at(sub_index).map)
                                     .arg(semi_loaded_map[index].old_map.teleport.at(sub_index).destination_x)
                                     .arg(semi_loaded_map[index].old_map.teleport.at(sub_index).destination_y));
                        #endif
                        semi_loaded_map[index].map->teleporter[virtual_position].map=GlobalServerData::serverPrivateVariables.map_list[semi_loaded_map[index].old_map.teleport.at(sub_index).map];
                        semi_loaded_map[index].map->teleporter[virtual_position].x=semi_loaded_map[index].old_map.teleport.at(sub_index).destination_x;
                        semi_loaded_map[index].map->teleporter[virtual_position].y=semi_loaded_map[index].old_map.teleport.at(sub_index).destination_y;
                    }
                }
                else
                    DebugClass::debugConsole(QString("wrong teleporter on the map: %1(%2,%3), to %4 (%5,%6) because the tp is out of range")
                         .arg(semi_loaded_map[index].map->map_file)
                         .arg(semi_loaded_map[index].old_map.teleport.at(sub_index).source_x)
                         .arg(semi_loaded_map[index].old_map.teleport.at(sub_index).source_y)
                         .arg(semi_loaded_map[index].old_map.teleport.at(sub_index).map)
                         .arg(semi_loaded_map[index].old_map.teleport.at(sub_index).destination_x)
                         .arg(semi_loaded_map[index].old_map.teleport.at(sub_index).destination_y));
            }
            else
                DebugClass::debugConsole(QString("wrong teleporter on the map: %1(%2,%3), to %4 (%5,%6) because the map is not found")
                     .arg(semi_loaded_map[index].map->map_file)
                     .arg(semi_loaded_map[index].old_map.teleport.at(sub_index).source_x)
                     .arg(semi_loaded_map[index].old_map.teleport.at(sub_index).source_y)
                     .arg(semi_loaded_map[index].old_map.teleport.at(sub_index).map)
                     .arg(semi_loaded_map[index].old_map.teleport.at(sub_index).destination_x)
                     .arg(semi_loaded_map[index].old_map.teleport.at(sub_index).destination_y));

            sub_index++;
        }
        index++;
    }

    preload_the_bots(semi_loaded_map);

    //clean border balise without another oposite border
    size=semi_loaded_map.size();
    index=0;
    while(index<size)
    {
        if(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.bottom.map!=NULL && GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.bottom.map->border.top.map!=GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)])
        {
            if(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.bottom.map->border.top.map==NULL)
                DebugClass::debugConsole(QString("the map %1 have bottom map: %2, the map %2 have not a top map").arg(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->map_file).arg(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.bottom.map->map_file));
            else
                DebugClass::debugConsole(QString("the map %1 have bottom map: %2, the map %2 have this top map: %3").arg(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->map_file).arg(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.bottom.map->map_file).arg(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.bottom.map->border.top.map->map_file));
            GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.bottom.map=NULL;
        }
        if(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.top.map!=NULL && GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.top.map->border.bottom.map!=GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)])
        {
            if(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.top.map->border.bottom.map==NULL)
                DebugClass::debugConsole(QString("the map %1 have top map: %2, the map %2 have not a bottom map").arg(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->map_file).arg(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.top.map->map_file));
            else
                DebugClass::debugConsole(QString("the map %1 have top map: %2, the map %2 have this bottom map: %3").arg(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->map_file).arg(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.top.map->map_file).arg(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.top.map->border.bottom.map->map_file));
            GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.top.map=NULL;
        }
        if(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.left.map!=NULL && GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.left.map->border.right.map!=GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)])
        {
            if(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.left.map->border.right.map==NULL)
                DebugClass::debugConsole(QString("the map %1 have left map: %2, the map %2 have not a right map").arg(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->map_file).arg(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.left.map->map_file));
            else
                DebugClass::debugConsole(QString("the map %1 have left map: %2, the map %2 have this right map: %3").arg(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->map_file).arg(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.left.map->map_file).arg(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.left.map->border.right.map->map_file));
            GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.left.map=NULL;
        }
        if(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.right.map!=NULL && GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.right.map->border.left.map!=GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)])
        {
            if(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.right.map->border.left.map==NULL)
                DebugClass::debugConsole(QString("the map %1 have right map: %2, the map %2 have not a left map").arg(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->map_file).arg(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.right.map->map_file));
            else
                DebugClass::debugConsole(QString("the map %1 have right map: %2, the map %2 have this left map: %3").arg(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->map_file).arg(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.right.map->map_file).arg(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.right.map->border.left.map->map_file));
            GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.right.map=NULL;
        }
        index++;
    }

    //resolv the near map
    size=semi_loaded_map.size();
    index=0;
    while(index<size)
    {
        GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->near_map << GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)];

        if(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.bottom.map!=NULL && !GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->near_map.contains(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.bottom.map))
        {
            GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->near_map << GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.bottom.map;
            if(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.left.map!=NULL && !GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->near_map.contains(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.left.map))
                GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->near_map << GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.left.map;
            if(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.right.map!=NULL && !GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->near_map.contains(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.right.map))
                GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->near_map << GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.right.map;
        }

        if(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.top.map!=NULL && !GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->near_map.contains(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.top.map))
        {
            GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->near_map << GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.top.map;
            if(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.left.map!=NULL &&  !GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->near_map.contains(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.left.map))
                GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->near_map << GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.left.map;
            if(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.right.map!=NULL &&  !GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->near_map.contains(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.right.map))
                GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->near_map << GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.right.map;
        }

        if(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.right.map!=NULL && !GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->near_map.contains(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.right.map))
        {
            GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->near_map << GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.right.map;
            if(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.top.map!=NULL &&  !GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->near_map.contains(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.top.map))
                GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->near_map << GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.top.map;
            if(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.bottom.map!=NULL &&  !GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->near_map.contains(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.bottom.map))
                GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->near_map << GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.bottom.map;
        }

        if(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.left.map!=NULL && !GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->near_map.contains(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.left.map))
        {
            GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->near_map << GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.left.map;
            if(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.top.map!=NULL &&  !GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->near_map.contains(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.top.map))
                GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->near_map << GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.top.map;
            if(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.bottom.map!=NULL &&  !GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->near_map.contains(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.bottom.map))
                GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->near_map << GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.bottom.map;
        }

        index++;
    }

    //resolv the offset to change of map
    size=semi_loaded_map.size();
    index=0;
    while(index<size)
    {
        if(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.bottom.map!=NULL)
            GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.bottom.x_offset=semi_loaded_map.at(index).border.bottom.x_offset-semi_loaded_map.at(map_name.indexOf(semi_loaded_map.at(index).border.bottom.fileName)).border.top.x_offset;
        else
            GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.bottom.x_offset=0;
        if(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.top.map!=NULL)
            GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.top.x_offset=semi_loaded_map.at(index).border.top.x_offset-semi_loaded_map.at(map_name.indexOf(semi_loaded_map.at(index).border.top.fileName)).border.bottom.x_offset;
        else
            GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.top.x_offset=0;
        if(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.left.map!=NULL)
            GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.left.y_offset=semi_loaded_map.at(index).border.left.y_offset-semi_loaded_map.at(map_name.indexOf(semi_loaded_map.at(index).border.left.fileName)).border.right.y_offset;
        else
            GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.left.y_offset=0;
        if(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.right.map!=NULL)
            GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.right.y_offset=semi_loaded_map.at(index).border.right.y_offset-semi_loaded_map.at(map_name.indexOf(semi_loaded_map.at(index).border.right.fileName)).border.left.y_offset;
        else
            GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.right.y_offset=0;
        index++;
    }

    //load the rescue
    size=semi_loaded_map.size();
    index=0;
    while(index<size)
    {
        sub_index=0;
        while(sub_index<semi_loaded_map[index].old_map.rescue_points.size())
        {
            const Map_to_send::Map_Point &point=semi_loaded_map[index].old_map.rescue_points.at(sub_index);
            QPair<quint8,quint8> coord;
            coord.first=point.x;
            coord.second=point.y;
            static_cast<MapServer *>(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)])->rescue[coord]=Orientation_bottom;
            sub_index++;
        }
        index++;
    }

    size=full_map_name.size();
    index=0;
    while(index<size)
    {
        if(GlobalServerData::serverPrivateVariables.map_list.contains(full_map_name.at(index)))
        {
            GlobalServerData::serverPrivateVariables.map_list[full_map_name.at(index)]->id=index;
            GlobalServerData::serverPrivateVariables.id_map_to_map[GlobalServerData::serverPrivateVariables.map_list[full_map_name.at(index)]->id]=full_map_name.at(index);
        }
        index++;
    }

    DebugClass::debugConsole(QString("%1 map(s) loaded").arg(GlobalServerData::serverPrivateVariables.map_list.size()));

    botFiles.clear();
}

void BaseServer::preload_the_skin()
{
    QStringList skinFolderList=FacilityLib::skinIdList(GlobalServerData::serverPrivateVariables.datapack_basePath+DATAPACK_BASE_PATH_SKIN);
    int index=0;
    while(index<skinFolderList.size())
    {
        GlobalServerData::serverPrivateVariables.skinList[skinFolderList.at(index)]=index;
        index++;
    }

    DebugClass::debugConsole(QString("%1 skin(s) loaded").arg(GlobalServerData::serverPrivateVariables.skinList.size()));
}

void BaseServer::preload_the_datapack()
{
    QStringList returnList=FacilityLib::listFolder(GlobalServerData::serverPrivateVariables.datapack_basePath);
    int index=0;
    int size=returnList.size();
    while(index<size)
    {
        QString fileName=returnList.at(index);
        if(fileName.contains(GlobalServerData::serverPrivateVariables.datapack_rightFileName))
        {
            QFile file(GlobalServerData::serverPrivateVariables.datapack_basePath+returnList.at(index));
            if(file.size()<=8*1024*1024)
            {
                if(file.open(QIODevice::ReadOnly))
                {
                    fileName.replace("\\","/");//remplace if is under windows server
                    GlobalServerData::serverPrivateVariables.filesList[fileName]=QFileInfo(file).lastModified().toTime_t();
                    file.close();
                }
            }
        }
        index++;
    }
    DebugClass::debugConsole(QString("%1 file(s) list for datapack cached").arg(GlobalServerData::serverPrivateVariables.filesList.size()));
}

void BaseServer::preload_the_players()
{
    ClientHeavyLoad::simplifiedIdList.clear();
    int index=0;
    while(index<GlobalServerData::serverSettings.max_players)
    {
        ClientHeavyLoad::simplifiedIdList << index;
        index++;
    }
}

void BaseServer::preload_the_visibility_algorithm()
{
    QHash<QString,Map *>::const_iterator i = GlobalServerData::serverPrivateVariables.map_list.constBegin();
    QHash<QString,Map *>::const_iterator i_end = GlobalServerData::serverPrivateVariables.map_list.constEnd();
    switch(GlobalServerData::serverSettings.mapVisibility.mapVisibilityAlgorithm)
    {
        case MapVisibilityAlgorithm_simple:
        while (i != i_end)
        {
            static_cast<Map_server_MapVisibility_simple *>(i.value())->show=true;
            i++;
        }
        break;
        case MapVisibilityAlgorithm_none:
        break;
        default:
        break;
    }
}

void BaseServer::preload_the_bots(const QList<Map_semi> &semi_loaded_map)
{
    int shops_number=0;
    int bots_number=0;
    int learnpoint_number=0;
    int healpoint_number=0;
    int botfights_number=0;
    int botfightstigger_number=0;
    //resolv the shops, learn, heal
    int size=semi_loaded_map.size();
    int index=0;
    bool ok;
    while(index<size)
    {
        int sub_index=0;
        while(sub_index<semi_loaded_map[index].old_map.bots.size())
        {
            bots_number++;
            Map_to_send::Bot_Semi bot_Semi=semi_loaded_map[index].old_map.bots.at(sub_index);
            loadBotFile(bot_Semi.file);
            if(botFiles.contains(bot_Semi.file))
                if(botFiles[bot_Semi.file].contains(bot_Semi.id))
                {
                    #ifdef DEBUG_MESSAGE_MAP_LOAD
                    CatchChallenger::DebugClass::debugConsole(QString("Bot %1 (%2) at %3 (%4,%5)").arg(bot_Semi.file).arg(bot_Semi.id).arg(semi_loaded_map[index].map->map_file).arg(bot_Semi.point.x).arg(bot_Semi.point.y));
                    #endif
                    QHashIterator<quint8,QDomElement> i(botFiles[bot_Semi.file][bot_Semi.id].step);
                    while (i.hasNext()) {
                        i.next();
                        QDomElement step = i.value();
                        if(step.attribute("type")=="shop")
                        {
                            if(!step.hasAttribute("shop"))
                                CatchChallenger::DebugClass::debugConsole(QString("Has not attribute \"shop\": for bot id: %1 (%2), spawn at: %3 (%4,%5), for step: %6")
                                    .arg(bot_Semi.id).arg(bot_Semi.file).arg(semi_loaded_map[index].map->map_file).arg(bot_Semi.point.x).arg(bot_Semi.point.y).arg(i.key()));
                            else
                            {
                                quint32 shop=step.attribute("shop").toUInt(&ok);
                                if(!ok)
                                    CatchChallenger::DebugClass::debugConsole(QString("shop is not a number: for bot id: %1 (%2), spawn at: %3 (%4,%5), for step: %6")
                                        .arg(bot_Semi.id).arg(bot_Semi.file).arg(semi_loaded_map[index].map->map_file).arg(bot_Semi.point.x).arg(bot_Semi.point.y).arg(i.key()));
                                else if(!GlobalServerData::serverPrivateVariables.shops.contains(shop))
                                    CatchChallenger::DebugClass::debugConsole(QString("shop number is not valid shop: for bot id: %1 (%2), spawn at: %3 (%4,%5), for step: %6")
                                        .arg(bot_Semi.id).arg(bot_Semi.file).arg(semi_loaded_map[index].map->map_file).arg(bot_Semi.point.x).arg(bot_Semi.point.y).arg(i.key()));
                                else
                                {
                                    #ifdef DEBUG_MESSAGE_MAP_LOAD
                                    CatchChallenger::DebugClass::debugConsole(QString("shop put at: %1 (%2,%3)")
                                        .arg(semi_loaded_map[index].map->map_file).arg(bot_Semi.point.x).arg(bot_Semi.point.y));
                                    #endif
                                    static_cast<MapServer *>(semi_loaded_map[index].map)->shops.insert(QPair<quint8,quint8>(bot_Semi.point.x,bot_Semi.point.y),shop);
                                    shops_number++;
                                }
                            }
                        }
                        else if(step.attribute("type")=="learn")
                        {
                            if(static_cast<MapServer *>(semi_loaded_map[index].map)->learn.contains(QPair<quint8,quint8>(bot_Semi.point.x,bot_Semi.point.y)))
                                CatchChallenger::DebugClass::debugConsole(QString("learn point already on the map: for bot id: %1 (%2), spawn at: %3 (%4,%5), for step: %6")
                                    .arg(bot_Semi.id).arg(bot_Semi.file).arg(semi_loaded_map[index].map->map_file).arg(bot_Semi.point.x).arg(bot_Semi.point.y).arg(i.key()));
                            else
                            {
                                #ifdef DEBUG_MESSAGE_MAP_LOAD
                                CatchChallenger::DebugClass::debugConsole(QString("learn point put at: %1 (%2,%3)")
                                    .arg(semi_loaded_map[index].map->map_file).arg(bot_Semi.point.x).arg(bot_Semi.point.y));
                                #endif
                                static_cast<MapServer *>(semi_loaded_map[index].map)->learn.insert(QPair<quint8,quint8>(bot_Semi.point.x,bot_Semi.point.y));
                                learnpoint_number++;
                            }
                        }
                        else if(step.attribute("type")=="heal")
                        {
                            if(static_cast<MapServer *>(semi_loaded_map[index].map)->heal.contains(QPair<quint8,quint8>(bot_Semi.point.x,bot_Semi.point.y)))
                                CatchChallenger::DebugClass::debugConsole(QString("heal point already on the map: for bot id: %1 (%2), spawn at: %3 (%4,%5), for step: %6")
                                    .arg(bot_Semi.id).arg(bot_Semi.file).arg(semi_loaded_map[index].map->map_file).arg(bot_Semi.point.x).arg(bot_Semi.point.y).arg(i.key()));
                            else
                            {
                                #ifdef DEBUG_MESSAGE_MAP_LOAD
                                CatchChallenger::DebugClass::debugConsole(QString("heal point put at: %1 (%2,%3)")
                                    .arg(semi_loaded_map[index].map->map_file).arg(bot_Semi.point.x).arg(bot_Semi.point.y));
                                #endif
                                static_cast<MapServer *>(semi_loaded_map[index].map)->heal.insert(QPair<quint8,quint8>(bot_Semi.point.x,bot_Semi.point.y));
                                healpoint_number++;
                            }
                        }
                        else if(step.attribute("type")=="fight")
                        {
                            if(static_cast<MapServer *>(semi_loaded_map[index].map)->botsFight.contains(QPair<quint8,quint8>(bot_Semi.point.x,bot_Semi.point.y)))
                                CatchChallenger::DebugClass::debugConsole(QString("botsFight point already on the map: for bot id: %1 (%2), spawn at: %3 (%4,%5), for step: %6")
                                    .arg(bot_Semi.id).arg(bot_Semi.file).arg(semi_loaded_map[index].map->map_file).arg(bot_Semi.point.x).arg(bot_Semi.point.y).arg(i.key()));
                            else
                            {
                                quint32 fightid=step.attribute("fightid").toUInt(&ok);
                                if(ok)
                                {
                                    if(CommonDatapack::commonDatapack.botFights.contains(fightid))
                                    {
                                        if(bot_Semi.property_text.contains("lookAt"))
                                        {
                                            Direction direction;
                                            if(bot_Semi.property_text["lookAt"]=="left")
                                                direction=CatchChallenger::Direction_move_at_left;
                                            else if(bot_Semi.property_text["lookAt"]=="right")
                                                direction=CatchChallenger::Direction_move_at_right;
                                            else if(bot_Semi.property_text["lookAt"]=="top")
                                                direction=CatchChallenger::Direction_move_at_top;
                                            else
                                            {
                                                if(bot_Semi.property_text["lookAt"]!="bottom")
                                                    CatchChallenger::DebugClass::debugConsole(QString("Wrong direction for the bot at %1 (%2,%3)")
                                                        .arg(semi_loaded_map[index].map->map_file).arg(bot_Semi.point.x).arg(bot_Semi.point.y));
                                                direction=CatchChallenger::Direction_move_at_bottom;
                                            }
                                            #ifdef DEBUG_MESSAGE_MAP_LOAD
                                            CatchChallenger::DebugClass::debugConsole(QString("botsFight point put at: %1 (%2,%3)")
                                                .arg(semi_loaded_map[index].map->map_file).arg(bot_Semi.point.x).arg(bot_Semi.point.y));
                                            #endif
                                            static_cast<MapServer *>(semi_loaded_map[index].map)->botsFight.insert(QPair<quint8,quint8>(bot_Semi.point.x,bot_Semi.point.y),fightid);
                                            botfights_number++;

                                            //load the botsFightTrigger
                                            #ifdef DEBUG_MESSAGE_CLIENT_FIGHT_BOT
                                            CatchChallenger::DebugClass::debugConsole(QString("Put bot fight point %1 at %2 (%3,%4) in direction: %5").arg(fightid).arg(semi_loaded_map[index].map->map_file).arg(bot_Semi.point.x).arg(bot_Semi.point.y).arg(direction));
                                            #endif
                                            quint8 temp_x=bot_Semi.point.x,temp_y=bot_Semi.point.y;
                                            int index_botfight_range=0;
                                            CatchChallenger::Map *map=semi_loaded_map[index].map;
                                            CatchChallenger::Map *old_map=map;
                                            while(index_botfight_range<CATCHCHALLENGER_BOTFIGHT_RANGE)
                                            {
                                                if(!CatchChallenger::MoveOnTheMap::canGoTo(direction,*map,temp_x,temp_y,true,false))
                                                    break;
                                                if(!CatchChallenger::MoveOnTheMap::move(direction,&map,&temp_x,&temp_y,true,false))
                                                    break;
                                                if(map!=old_map)
                                                    break;
                                                static_cast<MapServer *>(semi_loaded_map[index].map)->botsFightTrigger.insert(QPair<quint8,quint8>(temp_x,temp_y),fightid);
                                                index_botfight_range++;
                                                botfightstigger_number++;
                                            }
                                        }
                                        else
                                            DebugClass::debugConsole(QString("lookAt not found at: %1 (%2,%3)").arg(shops_number));
                                    }
                                    else
                                        DebugClass::debugConsole(QString("fightid not found into the list at: %1 (%2,%3)").arg(shops_number));
                                }
                                else
                                    DebugClass::debugConsole(QString("botsFight point have wrong fightid at: %1 (%2,%3)").arg(shops_number));
                            }
                        }
                    }
                }
            sub_index++;
        }
        index++;
    }

    DebugClass::debugConsole(QString("%1 learn point(s) on map loaded").arg(learnpoint_number));
    DebugClass::debugConsole(QString("%1 heal point(s) on map loaded").arg(healpoint_number));
    DebugClass::debugConsole(QString("%1 bot fight(s) on map loaded").arg(botfights_number));
    DebugClass::debugConsole(QString("%1 bot fights tigger(s) on map loaded").arg(botfightstigger_number));
    DebugClass::debugConsole(QString("%1 shop(s) on map loaded").arg(shops_number));
    DebugClass::debugConsole(QString("%1 bots(s) on map loaded").arg(bots_number));
}

void BaseServer::parseJustLoadedMap(const Map_to_send &,const QString &)
{
}

bool BaseServer::initialize_the_database()
{
    if(GlobalServerData::serverPrivateVariables.db!=NULL)
    {
        DebugClass::debugConsole(QString("Disconnected to %1 at %2 (%3), previous connection: %4")
                                 .arg(GlobalServerData::serverPrivateVariables.db_type_string)
                                 .arg(GlobalServerData::serverSettings.database.mysql.host)
                                 .arg(GlobalServerData::serverPrivateVariables.db->isOpen())
                                 .arg(QSqlDatabase::connectionNames().join(";"))
                                 );
        closeDB();
    }
    switch(GlobalServerData::serverSettings.database.type)
    {
        default:
        DebugClass::debugConsole(QString("database type unknow"));
        return false;
        case ServerSettings::Database::DatabaseType_Mysql:
        GlobalServerData::serverPrivateVariables.db = new QSqlDatabase();
        *GlobalServerData::serverPrivateVariables.db = QSqlDatabase::addDatabase("QMYSQL","server");
        GlobalServerData::serverPrivateVariables.db->setConnectOptions("MYSQL_OPT_RECONNECT=1");
        GlobalServerData::serverPrivateVariables.db->setHostName(GlobalServerData::serverSettings.database.mysql.host);
        GlobalServerData::serverPrivateVariables.db->setDatabaseName(GlobalServerData::serverSettings.database.mysql.db);
        GlobalServerData::serverPrivateVariables.db->setUserName(GlobalServerData::serverSettings.database.mysql.login);
        GlobalServerData::serverPrivateVariables.db->setPassword(GlobalServerData::serverSettings.database.mysql.pass);
        GlobalServerData::serverPrivateVariables.db_type_string="mysql";
        break;
        case ServerSettings::Database::DatabaseType_SQLite:
        GlobalServerData::serverPrivateVariables.db = new QSqlDatabase();
        *GlobalServerData::serverPrivateVariables.db = QSqlDatabase::addDatabase("QSQLITE","server");
        GlobalServerData::serverPrivateVariables.db->setDatabaseName(GlobalServerData::serverSettings.database.sqlite.file);
        GlobalServerData::serverPrivateVariables.db_type_string="sqlite";
        break;
    }
    if(!GlobalServerData::serverPrivateVariables.db->open())
    {
        DebugClass::debugConsole(QString("Unable to connect to the database: %1, with the login: %2, database text: %3").arg(GlobalServerData::serverPrivateVariables.db->lastError().driverText()).arg(GlobalServerData::serverSettings.database.mysql.login).arg(GlobalServerData::serverPrivateVariables.db->lastError().databaseText()));
        emit error(QString("Unable to connect to the database: %1, with the login: %2, database text: %3").arg(GlobalServerData::serverPrivateVariables.db->lastError().driverText()).arg(GlobalServerData::serverSettings.database.mysql.login).arg(GlobalServerData::serverPrivateVariables.db->lastError().databaseText()));
        return false;
    }
    DebugClass::debugConsole(QString("Connected to %1 at %2 (%3)")
                             .arg(GlobalServerData::serverPrivateVariables.db_type_string)
                             .arg(GlobalServerData::serverSettings.database.mysql.host)
                             .arg(GlobalServerData::serverPrivateVariables.db->isOpen()));
    return true;
}

void BaseServer::loadBotFile(const QString &fileName)
{
    if(botFiles.contains(fileName))
        return;
    botFiles[fileName];//create the entry
    QFile mapFile(fileName);
    if(!mapFile.open(QIODevice::ReadOnly))
    {
        qDebug() << mapFile.fileName()+": "+mapFile.errorString();
        return;
    }
    QByteArray xmlContent=mapFile.readAll();
    mapFile.close();
    QDomDocument domDocument;
    QString errorStr;
    int errorLine,errorColumn;
    if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
    {
        qDebug() << QString("%1, Parse error at line %2, column %3: %4").arg(mapFile.fileName()).arg(errorLine).arg(errorColumn).arg(errorStr);
        return;
    }
    bool ok;
    QDomElement root = domDocument.documentElement();
    if(root.tagName()!="bots")
    {
        qDebug() << QString("\"bots\" root balise not found for the xml file");
        return;
    }
    //load the bots
    QDomElement child = root.firstChildElement("bot");
    while(!child.isNull())
    {
        if(!child.hasAttribute("id"))
            CatchChallenger::DebugClass::debugConsole(QString("Has not attribute \"id\": child.tagName(): %1 (at line: %2)").arg(child.tagName()).arg(child.lineNumber()));
        else if(!child.isElement())
            CatchChallenger::DebugClass::debugConsole(QString("Is not an element: child.tagName(): %1, name: %2 (at line: %3)").arg(child.tagName().arg(child.attribute("name")).arg(child.lineNumber())));
        else
        {
            quint32 id=child.attribute("id").toUInt(&ok);
            if(ok)
            {
                botFiles[fileName][id];
                QDomElement step = child.firstChildElement("step");
                while(!step.isNull())
                {
                    if(!step.hasAttribute("id"))
                        CatchChallenger::DebugClass::debugConsole(QString("Has not attribute \"type\": bot.tagName(): %1 (at line: %2)").arg(step.tagName()).arg(step.lineNumber()));
                    else if(!step.hasAttribute("type"))
                        CatchChallenger::DebugClass::debugConsole(QString("Has not attribute \"type\": bot.tagName(): %1 (at line: %2)").arg(step.tagName()).arg(step.lineNumber()));
                    else if(!step.isElement())
                        CatchChallenger::DebugClass::debugConsole(QString("Is not an element: bot.tagName(): %1, type: %2 (at line: %3)").arg(step.tagName().arg(step.attribute("type")).arg(step.lineNumber())));
                    else
                    {
                        quint32 stepId=step.attribute("id").toUInt(&ok);
                        if(ok)
                            botFiles[fileName][id].step[stepId]=step;
                    }
                    step = step.nextSiblingElement("step");
                }
                if(!botFiles[fileName][id].step.contains(1))
                    botFiles[fileName].remove(id);
            }
            else
                CatchChallenger::DebugClass::debugConsole(QString("Attribute \"id\" is not a number: bot.tagName(): %1 (at line: %2)").arg(child.tagName()).arg(child.lineNumber()));
        }
        child = child.nextSiblingElement("bot");
    }
}

////////////////////////////////////////////////// server stopping ////////////////////////////////////////////

void BaseServer::unload_the_data()
{
    GlobalServerData::serverPrivateVariables.stopIt=true;

    unload_the_visibility_algorithm();
    unload_the_plant_on_map();
    unload_the_map();
    unload_the_bots();
    unload_monsters_drops();
    unload_shop();
    unload_the_skin();
    unload_the_datapack();
    unload_the_players();

    CommonDatapack::commonDatapack.unload();
}

void BaseServer::unload_the_bots()
{
}

void BaseServer::unload_the_map()
{
    QHash<QString,Map *>::const_iterator i = GlobalServerData::serverPrivateVariables.map_list.constBegin();
    QHash<QString,Map *>::const_iterator i_end = GlobalServerData::serverPrivateVariables.map_list.constEnd();
    while (i != i_end)
    {
        Map::removeParsedLayer(i.value()->parsed_layer);
        delete i.value();
        i++;
    }
    GlobalServerData::serverPrivateVariables.map_list.clear();
    GlobalServerData::serverPrivateVariables.botSpawn.clear();
}

void BaseServer::unload_the_skin()
{
    GlobalServerData::serverPrivateVariables.skinList.clear();
}

void BaseServer::unload_the_visibility_algorithm()
{
}

void BaseServer::unload_the_datapack()
{
    GlobalServerData::serverPrivateVariables.filesList.clear();
}

void BaseServer::unload_the_players()
{
    ClientHeavyLoad::simplifiedIdList.clear();
}

bool BaseServer::check_if_now_stopped()
{
    if(client_list.size()!=0)
        return false;
    if(stat!=InDown)
        return false;

    DebugClass::debugConsole("Fully stopped");
    if(GlobalServerData::serverPrivateVariables.db!=NULL)
    {
        DebugClass::debugConsole(QString("Disconnected to %1 at %2 (%3)")
                                 .arg(GlobalServerData::serverPrivateVariables.db_type_string)
                                 .arg(GlobalServerData::serverSettings.database.mysql.host)
                                 .arg(GlobalServerData::serverPrivateVariables.db->isOpen()));
        closeDB();
    }
    stat=Down;
    emit is_started(false);

    unload_the_data();
    return true;
}

void BaseServer::setSettings(ServerSettings settings)
{
    //load it
    GlobalServerData::serverSettings=settings;

    loadAndFixSettings();
}

ServerSettings BaseServer::getSettings()
{
    return GlobalServerData::serverSettings;
}

void BaseServer::loadAndFixSettings()
{
    //check the settings here
    if(GlobalServerData::serverSettings.max_players<1)
        GlobalServerData::serverSettings.max_players=200;
    ProtocolParsing::setMaxPlayers(GlobalServerData::serverSettings.max_players);

/*    quint8 player_list_size;
    if(GlobalServerData::serverSettings.max_players<=255)
        player_list_size=sizeof(quint8);
    else
        player_list_size=sizeof(quint16);*/
    quint8 map_list_size;
    if(GlobalServerData::serverPrivateVariables.map_list.size()<=255)
        map_list_size=sizeof(quint8);
    else if(GlobalServerData::serverPrivateVariables.map_list.size()<=65535)
        map_list_size=sizeof(quint16);
    else
        map_list_size=sizeof(quint32);
    GlobalServerData::serverPrivateVariables.sizeofInsertRequest=
            //mutualised
            sizeof(quint8)+map_list_size+/*player_list_size same with move, delete, ...*/
            //of the player
            /*player_list_size same with move, delete, ...*/+sizeof(quint8)+sizeof(quint8)+sizeof(quint8)+sizeof(quint8)+sizeof(quint8)+0/*pseudo size put directy*/+sizeof(quint8);

    if(GlobalServerData::serverSettings.mapVisibility.simple.max>GlobalServerData::serverSettings.max_players)
        GlobalServerData::serverSettings.mapVisibility.simple.max=GlobalServerData::serverSettings.max_players;

    if(GlobalServerData::serverSettings.database.secondToPositionSync==0)
        GlobalServerData::serverPrivateVariables.positionSync.stop();
    else
        GlobalServerData::serverPrivateVariables.positionSync.start(GlobalServerData::serverSettings.database.secondToPositionSync*1000);

    switch(GlobalServerData::serverSettings.database.type)
    {
        case CatchChallenger::ServerSettings::Database::DatabaseType_SQLite:
        case CatchChallenger::ServerSettings::Database::DatabaseType_Mysql:
        break;
        default:
            qDebug() << "Wrong db type";
        break;
    }
    switch(GlobalServerData::serverSettings.mapVisibility.mapVisibilityAlgorithm)
    {
        case CatchChallenger::MapVisibilityAlgorithm_none:
        case CatchChallenger::MapVisibilityAlgorithm_simple:
        break;
        default:
            qDebug() << "Wrong visibility algorithm";
        break;
    }
}

//call by normal stop
void BaseServer::stop_internal_server()
{
    if(stat!=Up && stat!=InDown)
    {
        if(stat!=Down)
            DebugClass::debugConsole("Is in wrong stat for stopping: "+QString::number((int)stat));
        return;
    }
    DebugClass::debugConsole("Try stop");
    stat=InDown;

    QSetIterator<Client *> i(client_list);
     while (i.hasNext())
         i.next()->disconnectClient();
    QFakeServer::server.close();

    check_if_now_stopped();
}

/////////////////////////////////////// player related //////////////////////////////////////
ClientMapManagement * BaseServer::getClientMapManagement()
{
    switch(GlobalServerData::serverSettings.mapVisibility.mapVisibilityAlgorithm)
    {
        default:
        case MapVisibilityAlgorithm_simple:
            return new MapVisibilityAlgorithm_Simple();
        break;
        case MapVisibilityAlgorithm_none:
            return new MapVisibilityAlgorithm_None();
        break;
    }
}


/////////////////////////////////////////////////// Object removing /////////////////////////////////////

void BaseServer::removeOneClient()
{
    Client *client=qobject_cast<Client *>(QObject::sender());
    if(client==NULL)
    {
        DebugClass::debugConsole("removeOneClient(): NULL client at disconnection");
        return;
    }
    client_list.remove(client);
    client->deleteLater();
}

/////////////////////////////////////// player related //////////////////////////////////////

void BaseServer::newConnection()
{
    while(QFakeServer::server.hasPendingConnections())
    {
        QFakeSocket *socket = QFakeServer::server.nextPendingConnection();
        if(socket!=NULL)
        {
            DebugClass::debugConsole(QString("newConnection(): new client connected by fake socket"));
            connect_the_last_client(new Client(new ConnectedSocket(socket),false,getClientMapManagement()));
        }
        else
            DebugClass::debugConsole("NULL client at BaseServer::newConnection()");
    }
}

void BaseServer::connect_the_last_client(Client * client)
{
    client_list << client;
    connect(client,SIGNAL(isReadyToDelete()),this,SLOT(removeOneClient()),Qt::QueuedConnection);
}

bool BaseServer::isListen()
{
    return (stat==Up);
}

bool BaseServer::isStopped()
{
    return (stat==Down);
}

void BaseServer::stop()
{
    emit try_stop_server();
}
