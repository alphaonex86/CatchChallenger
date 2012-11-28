#include "BaseServer.h"
#include "../../server/base/GlobalData.h"
#include "../../general/base/FacilityLib.h"

#include <QFile>
#include <QByteArray>

using namespace Pokecraft;

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
    qRegisterMetaType<Direction>("Direction");
    qRegisterMetaType<Map_server_MapVisibility_simple*>("Map_server_MapVisibility_simple*");
    qRegisterMetaType<Player_public_informations>("Player_public_informations");
    qRegisterMetaType<QSqlQuery>("QSqlQuery");

    GlobalData::serverPrivateVariables.connected_players	= 0;
    GlobalData::serverPrivateVariables.number_of_bots_logged= 0;
    GlobalData::serverPrivateVariables.db                   = NULL;
    GlobalData::serverPrivateVariables.timer_player_map     = NULL;

    GlobalData::serverPrivateVariables.botSpawnIndex=0;
    GlobalData::serverPrivateVariables.datapack_basePath		= QCoreApplication::applicationDirPath()+"/datapack/";
    GlobalData::serverPrivateVariables.datapack_rightFileName	= QRegExp(DATAPACK_FILE_REGEX);

    GlobalData::serverSettings.database.type=ServerSettings::Database::DatabaseType_Mysql;
    GlobalData::serverSettings.mapVisibility.mapVisibilityAlgorithm	= MapVisibilityAlgorithm_simple;
    GlobalData::serverSettings.mapVisibility.simple.max		= 30;
    GlobalData::serverSettings.mapVisibility.simple.reshow		= 20;
    GlobalData::serverPrivateVariables.timer_to_send_insert_move_remove.start(POKECRAFT_SERVER_MAP_TIME_TO_SEND_MOVEMENT);

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
    GlobalData::serverPrivateVariables.stopIt=true;
    GlobalData::serverPrivateVariables.eventThreaderList.clear();
    if(GlobalData::serverPrivateVariables.db!=NULL)
    {
        GlobalData::serverPrivateVariables.db->close();
        delete GlobalData::serverPrivateVariables.db;
        GlobalData::serverPrivateVariables.db=NULL;
    }
}

void BaseServer::initAll()
{
    /// \bug QObject::moveToThread: Current thread (0x79e1f0) is not the object's thread (0x6b4690)
    GlobalData::serverPrivateVariables.player_updater.moveToThread(GlobalData::serverPrivateVariables.eventThreaderList.at(0));
    BroadCastWithoutSender::broadCastWithoutSender.moveToThread(GlobalData::serverPrivateVariables.eventThreaderList.at(0));
}

//////////////////////////////////////////// server starting //////////////////////////////////////

void BaseServer::preload_the_data()
{
    GlobalData::serverPrivateVariables.stopIt=false;

    preload_the_datapack();
    preload_the_map();
    preload_the_skin();
    preload_the_items();
    preload_the_players();
    preload_the_plant();
    preload_the_plant_on_map();
}

void BaseServer::preload_the_map()
{
    GlobalData::serverPrivateVariables.datapack_mapPath=GlobalData::serverPrivateVariables.datapack_basePath+DATAPACK_BASE_PATH_MAP;
    #ifdef DEBUG_MESSAGE_MAP_LOAD
    DebugClass::debugConsole(QString("start preload the map, into: %1").arg(GlobalData::serverPrivateVariables.datapack_mapPath));
    #endif
    Map_loader map_temp;
    QList<Map_semi> semi_loaded_map;
    QStringList map_name;
    QStringList full_map_name;
    QStringList returnList=FacilityLib::listFolder(GlobalData::serverPrivateVariables.datapack_mapPath);

    //load the map
    int size=returnList.size();
    int index=0;
    QRegExp mapFilter("\\.tmx$");
    while(index<size)
    {
        QString fileName=returnList.at(index);
        fileName.replace('\\','/');
        if(fileName.contains(mapFilter) && GlobalData::serverPrivateVariables.filesList.contains(DATAPACK_BASE_PATH_MAP+fileName))
        {
            #ifdef DEBUG_MESSAGE_MAP_LOAD
            DebugClass::debugConsole(QString("load the map: %1").arg(fileName));
            #endif
            full_map_name << fileName;
            if(map_temp.tryLoadMap(GlobalData::serverPrivateVariables.datapack_mapPath+fileName))
            {
                switch(GlobalData::serverSettings.mapVisibility.mapVisibilityAlgorithm)
                {
                    case MapVisibilityAlgorithm_simple:
                        GlobalData::serverPrivateVariables.map_list[fileName]=new Map_server_MapVisibility_simple;
                    break;
                    case MapVisibilityAlgorithm_none:
                    default:
                        GlobalData::serverPrivateVariables.map_list[fileName]=new MapServer;
                    break;
                }

                GlobalData::serverPrivateVariables.map_list[fileName]->width			= map_temp.map_to_send.width;
                GlobalData::serverPrivateVariables.map_list[fileName]->height			= map_temp.map_to_send.height;
                GlobalData::serverPrivateVariables.map_list[fileName]->parsed_layer.walkable	= map_temp.map_to_send.parsed_layer.walkable;
                GlobalData::serverPrivateVariables.map_list[fileName]->parsed_layer.water		= map_temp.map_to_send.parsed_layer.water;
                GlobalData::serverPrivateVariables.map_list[fileName]->parsed_layer.grass		= map_temp.map_to_send.parsed_layer.grass;
                GlobalData::serverPrivateVariables.map_list[fileName]->parsed_layer.dirt		= map_temp.map_to_send.parsed_layer.dirt;
                GlobalData::serverPrivateVariables.map_list[fileName]->map_file			= fileName;

                GlobalData::serverPrivateVariables.map_list[fileName];
                map_name << fileName;

                parseJustLoadedMap(map_temp.map_to_send,fileName);

                Map_semi map_semi;
                map_semi.map				= GlobalData::serverPrivateVariables.map_list[fileName];

                if(!map_temp.map_to_send.border.top.fileName.isEmpty())
                {
                    map_semi.border.top.fileName		= Map_loader::resolvRelativeMap(GlobalData::serverPrivateVariables.datapack_mapPath+fileName,map_temp.map_to_send.border.top.fileName,GlobalData::serverPrivateVariables.datapack_mapPath);
                    map_semi.border.top.x_offset		= map_temp.map_to_send.border.top.x_offset;
                }
                if(!map_temp.map_to_send.border.bottom.fileName.isEmpty())
                {
                    map_semi.border.bottom.fileName		= Map_loader::resolvRelativeMap(GlobalData::serverPrivateVariables.datapack_mapPath+fileName,map_temp.map_to_send.border.bottom.fileName,GlobalData::serverPrivateVariables.datapack_mapPath);
                    map_semi.border.bottom.x_offset		= map_temp.map_to_send.border.bottom.x_offset;
                }
                if(!map_temp.map_to_send.border.left.fileName.isEmpty())
                {
                    map_semi.border.left.fileName		= Map_loader::resolvRelativeMap(GlobalData::serverPrivateVariables.datapack_mapPath+fileName,map_temp.map_to_send.border.left.fileName,GlobalData::serverPrivateVariables.datapack_mapPath);
                    map_semi.border.left.y_offset		= map_temp.map_to_send.border.left.y_offset;
                }
                if(!map_temp.map_to_send.border.right.fileName.isEmpty())
                {
                    map_semi.border.right.fileName		= Map_loader::resolvRelativeMap(GlobalData::serverPrivateVariables.datapack_mapPath+fileName,map_temp.map_to_send.border.right.fileName,GlobalData::serverPrivateVariables.datapack_mapPath);
                    map_semi.border.right.y_offset		= map_temp.map_to_send.border.right.y_offset;
                }

                int sub_index=0;
                while(sub_index<map_temp.map_to_send.teleport.size())
                {
                    map_temp.map_to_send.teleport[sub_index].map=Map_loader::resolvRelativeMap(GlobalData::serverPrivateVariables.datapack_mapPath+fileName,map_temp.map_to_send.teleport.at(sub_index).map,GlobalData::serverPrivateVariables.datapack_mapPath);
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
        if(semi_loaded_map.at(index).border.bottom.fileName!="" && GlobalData::serverPrivateVariables.map_list.contains(semi_loaded_map.at(index).border.bottom.fileName))
            semi_loaded_map[index].map->border.bottom.map=GlobalData::serverPrivateVariables.map_list[semi_loaded_map.at(index).border.bottom.fileName];
        else
            semi_loaded_map[index].map->border.bottom.map=NULL;

        if(semi_loaded_map.at(index).border.top.fileName!="" && GlobalData::serverPrivateVariables.map_list.contains(semi_loaded_map.at(index).border.top.fileName))
            semi_loaded_map[index].map->border.top.map=GlobalData::serverPrivateVariables.map_list[semi_loaded_map.at(index).border.top.fileName];
        else
            semi_loaded_map[index].map->border.top.map=NULL;

        if(semi_loaded_map.at(index).border.left.fileName!="" && GlobalData::serverPrivateVariables.map_list.contains(semi_loaded_map.at(index).border.left.fileName))
            semi_loaded_map[index].map->border.left.map=GlobalData::serverPrivateVariables.map_list[semi_loaded_map.at(index).border.left.fileName];
        else
            semi_loaded_map[index].map->border.left.map=NULL;

        if(semi_loaded_map.at(index).border.right.fileName!="" && GlobalData::serverPrivateVariables.map_list.contains(semi_loaded_map.at(index).border.right.fileName))
            semi_loaded_map[index].map->border.right.map=GlobalData::serverPrivateVariables.map_list[semi_loaded_map.at(index).border.right.fileName];
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
            if(GlobalData::serverPrivateVariables.map_list.contains(semi_loaded_map[index].old_map.teleport.at(sub_index).map)
                    && semi_loaded_map[index].old_map.teleport.at(sub_index).destination_x<GlobalData::serverPrivateVariables.map_list[semi_loaded_map[index].old_map.teleport.at(sub_index).map]->width
                    && semi_loaded_map[index].old_map.teleport.at(sub_index).destination_y<GlobalData::serverPrivateVariables.map_list[semi_loaded_map[index].old_map.teleport.at(sub_index).map]->height
                )
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
                    semi_loaded_map[index].map->teleporter[virtual_position].map=GlobalData::serverPrivateVariables.map_list[semi_loaded_map[index].old_map.teleport.at(sub_index).map];
                    semi_loaded_map[index].map->teleporter[virtual_position].x=semi_loaded_map[index].old_map.teleport.at(sub_index).destination_x;
                    semi_loaded_map[index].map->teleporter[virtual_position].y=semi_loaded_map[index].old_map.teleport.at(sub_index).destination_y;
                }
            }
            else
                DebugClass::debugConsole(QString("wrong teleporter on the map: %1(%2,%3), to %4 (%5,%6)")
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

    //clean border balise without another oposite border
    size=semi_loaded_map.size();
    index=0;
    while(index<size)
    {
        if(GlobalData::serverPrivateVariables.map_list[map_name.at(index)]->border.bottom.map!=NULL && GlobalData::serverPrivateVariables.map_list[map_name.at(index)]->border.bottom.map->border.top.map!=GlobalData::serverPrivateVariables.map_list[map_name.at(index)])
        {
            if(GlobalData::serverPrivateVariables.map_list[map_name.at(index)]->border.bottom.map->border.top.map==NULL)
                DebugClass::debugConsole(QString("the map %1 have bottom map: %2, the map %2 have not a top map").arg(GlobalData::serverPrivateVariables.map_list[map_name.at(index)]->map_file).arg(GlobalData::serverPrivateVariables.map_list[map_name.at(index)]->border.bottom.map->map_file));
            else
                DebugClass::debugConsole(QString("the map %1 have bottom map: %2, the map %2 have this top map: %3").arg(GlobalData::serverPrivateVariables.map_list[map_name.at(index)]->map_file).arg(GlobalData::serverPrivateVariables.map_list[map_name.at(index)]->border.bottom.map->map_file).arg(GlobalData::serverPrivateVariables.map_list[map_name.at(index)]->border.bottom.map->border.top.map->map_file));
            GlobalData::serverPrivateVariables.map_list[map_name.at(index)]->border.bottom.map=NULL;
        }
        if(GlobalData::serverPrivateVariables.map_list[map_name.at(index)]->border.top.map!=NULL && GlobalData::serverPrivateVariables.map_list[map_name.at(index)]->border.top.map->border.bottom.map!=GlobalData::serverPrivateVariables.map_list[map_name.at(index)])
        {
            if(GlobalData::serverPrivateVariables.map_list[map_name.at(index)]->border.top.map->border.bottom.map==NULL)
                DebugClass::debugConsole(QString("the map %1 have top map: %2, the map %2 have not a bottom map").arg(GlobalData::serverPrivateVariables.map_list[map_name.at(index)]->map_file).arg(GlobalData::serverPrivateVariables.map_list[map_name.at(index)]->border.top.map->map_file));
            else
                DebugClass::debugConsole(QString("the map %1 have top map: %2, the map %2 have this bottom map: %3").arg(GlobalData::serverPrivateVariables.map_list[map_name.at(index)]->map_file).arg(GlobalData::serverPrivateVariables.map_list[map_name.at(index)]->border.top.map->map_file).arg(GlobalData::serverPrivateVariables.map_list[map_name.at(index)]->border.top.map->border.bottom.map->map_file));
            GlobalData::serverPrivateVariables.map_list[map_name.at(index)]->border.top.map=NULL;
        }
        if(GlobalData::serverPrivateVariables.map_list[map_name.at(index)]->border.left.map!=NULL && GlobalData::serverPrivateVariables.map_list[map_name.at(index)]->border.left.map->border.right.map!=GlobalData::serverPrivateVariables.map_list[map_name.at(index)])
        {
            if(GlobalData::serverPrivateVariables.map_list[map_name.at(index)]->border.left.map->border.right.map==NULL)
                DebugClass::debugConsole(QString("the map %1 have left map: %2, the map %2 have not a right map").arg(GlobalData::serverPrivateVariables.map_list[map_name.at(index)]->map_file).arg(GlobalData::serverPrivateVariables.map_list[map_name.at(index)]->border.left.map->map_file));
            else
                DebugClass::debugConsole(QString("the map %1 have left map: %2, the map %2 have this right map: %3").arg(GlobalData::serverPrivateVariables.map_list[map_name.at(index)]->map_file).arg(GlobalData::serverPrivateVariables.map_list[map_name.at(index)]->border.left.map->map_file).arg(GlobalData::serverPrivateVariables.map_list[map_name.at(index)]->border.left.map->border.right.map->map_file));
            GlobalData::serverPrivateVariables.map_list[map_name.at(index)]->border.left.map=NULL;
        }
        if(GlobalData::serverPrivateVariables.map_list[map_name.at(index)]->border.right.map!=NULL && GlobalData::serverPrivateVariables.map_list[map_name.at(index)]->border.right.map->border.left.map!=GlobalData::serverPrivateVariables.map_list[map_name.at(index)])
        {
            if(GlobalData::serverPrivateVariables.map_list[map_name.at(index)]->border.right.map->border.left.map==NULL)
                DebugClass::debugConsole(QString("the map %1 have right map: %2, the map %2 have not a left map").arg(GlobalData::serverPrivateVariables.map_list[map_name.at(index)]->map_file).arg(GlobalData::serverPrivateVariables.map_list[map_name.at(index)]->border.right.map->map_file));
            else
                DebugClass::debugConsole(QString("the map %1 have right map: %2, the map %2 have this left map: %3").arg(GlobalData::serverPrivateVariables.map_list[map_name.at(index)]->map_file).arg(GlobalData::serverPrivateVariables.map_list[map_name.at(index)]->border.right.map->map_file).arg(GlobalData::serverPrivateVariables.map_list[map_name.at(index)]->border.right.map->border.left.map->map_file));
            GlobalData::serverPrivateVariables.map_list[map_name.at(index)]->border.right.map=NULL;
        }
        index++;
    }

    //resolv the near map
    size=semi_loaded_map.size();
    index=0;
    while(index<size)
    {
        GlobalData::serverPrivateVariables.map_list[map_name.at(index)]->near_map << GlobalData::serverPrivateVariables.map_list[map_name.at(index)];

        if(GlobalData::serverPrivateVariables.map_list[map_name.at(index)]->border.bottom.map!=NULL && !GlobalData::serverPrivateVariables.map_list[map_name.at(index)]->near_map.contains(GlobalData::serverPrivateVariables.map_list[map_name.at(index)]->border.bottom.map))
        {
            GlobalData::serverPrivateVariables.map_list[map_name.at(index)]->near_map << GlobalData::serverPrivateVariables.map_list[map_name.at(index)]->border.bottom.map;
            if(GlobalData::serverPrivateVariables.map_list[map_name.at(index)]->border.left.map!=NULL && !GlobalData::serverPrivateVariables.map_list[map_name.at(index)]->near_map.contains(GlobalData::serverPrivateVariables.map_list[map_name.at(index)]->border.left.map))
                GlobalData::serverPrivateVariables.map_list[map_name.at(index)]->near_map << GlobalData::serverPrivateVariables.map_list[map_name.at(index)]->border.left.map;
            if(GlobalData::serverPrivateVariables.map_list[map_name.at(index)]->border.right.map!=NULL && !GlobalData::serverPrivateVariables.map_list[map_name.at(index)]->near_map.contains(GlobalData::serverPrivateVariables.map_list[map_name.at(index)]->border.right.map))
                GlobalData::serverPrivateVariables.map_list[map_name.at(index)]->near_map << GlobalData::serverPrivateVariables.map_list[map_name.at(index)]->border.right.map;
        }

        if(GlobalData::serverPrivateVariables.map_list[map_name.at(index)]->border.top.map!=NULL && !GlobalData::serverPrivateVariables.map_list[map_name.at(index)]->near_map.contains(GlobalData::serverPrivateVariables.map_list[map_name.at(index)]->border.top.map))
        {
            GlobalData::serverPrivateVariables.map_list[map_name.at(index)]->near_map << GlobalData::serverPrivateVariables.map_list[map_name.at(index)]->border.top.map;
            if(GlobalData::serverPrivateVariables.map_list[map_name.at(index)]->border.left.map!=NULL &&  !GlobalData::serverPrivateVariables.map_list[map_name.at(index)]->near_map.contains(GlobalData::serverPrivateVariables.map_list[map_name.at(index)]->border.left.map))
                GlobalData::serverPrivateVariables.map_list[map_name.at(index)]->near_map << GlobalData::serverPrivateVariables.map_list[map_name.at(index)]->border.left.map;
            if(GlobalData::serverPrivateVariables.map_list[map_name.at(index)]->border.right.map!=NULL &&  !GlobalData::serverPrivateVariables.map_list[map_name.at(index)]->near_map.contains(GlobalData::serverPrivateVariables.map_list[map_name.at(index)]->border.right.map))
                GlobalData::serverPrivateVariables.map_list[map_name.at(index)]->near_map << GlobalData::serverPrivateVariables.map_list[map_name.at(index)]->border.right.map;
        }

        if(GlobalData::serverPrivateVariables.map_list[map_name.at(index)]->border.right.map!=NULL && !GlobalData::serverPrivateVariables.map_list[map_name.at(index)]->near_map.contains(GlobalData::serverPrivateVariables.map_list[map_name.at(index)]->border.right.map))
        {
            GlobalData::serverPrivateVariables.map_list[map_name.at(index)]->near_map << GlobalData::serverPrivateVariables.map_list[map_name.at(index)]->border.right.map;
            if(GlobalData::serverPrivateVariables.map_list[map_name.at(index)]->border.top.map!=NULL &&  !GlobalData::serverPrivateVariables.map_list[map_name.at(index)]->near_map.contains(GlobalData::serverPrivateVariables.map_list[map_name.at(index)]->border.top.map))
                GlobalData::serverPrivateVariables.map_list[map_name.at(index)]->near_map << GlobalData::serverPrivateVariables.map_list[map_name.at(index)]->border.top.map;
            if(GlobalData::serverPrivateVariables.map_list[map_name.at(index)]->border.bottom.map!=NULL &&  !GlobalData::serverPrivateVariables.map_list[map_name.at(index)]->near_map.contains(GlobalData::serverPrivateVariables.map_list[map_name.at(index)]->border.bottom.map))
                GlobalData::serverPrivateVariables.map_list[map_name.at(index)]->near_map << GlobalData::serverPrivateVariables.map_list[map_name.at(index)]->border.bottom.map;
        }

        if(GlobalData::serverPrivateVariables.map_list[map_name.at(index)]->border.left.map!=NULL && !GlobalData::serverPrivateVariables.map_list[map_name.at(index)]->near_map.contains(GlobalData::serverPrivateVariables.map_list[map_name.at(index)]->border.left.map))
        {
            GlobalData::serverPrivateVariables.map_list[map_name.at(index)]->near_map << GlobalData::serverPrivateVariables.map_list[map_name.at(index)]->border.left.map;
            if(GlobalData::serverPrivateVariables.map_list[map_name.at(index)]->border.top.map!=NULL &&  !GlobalData::serverPrivateVariables.map_list[map_name.at(index)]->near_map.contains(GlobalData::serverPrivateVariables.map_list[map_name.at(index)]->border.top.map))
                GlobalData::serverPrivateVariables.map_list[map_name.at(index)]->near_map << GlobalData::serverPrivateVariables.map_list[map_name.at(index)]->border.top.map;
            if(GlobalData::serverPrivateVariables.map_list[map_name.at(index)]->border.bottom.map!=NULL &&  !GlobalData::serverPrivateVariables.map_list[map_name.at(index)]->near_map.contains(GlobalData::serverPrivateVariables.map_list[map_name.at(index)]->border.bottom.map))
                GlobalData::serverPrivateVariables.map_list[map_name.at(index)]->near_map << GlobalData::serverPrivateVariables.map_list[map_name.at(index)]->border.bottom.map;
        }

        index++;
    }

    //resolv the offset to change of map
    size=semi_loaded_map.size();
    index=0;
    while(index<size)
    {
        if(GlobalData::serverPrivateVariables.map_list[map_name.at(index)]->border.bottom.map!=NULL)
            GlobalData::serverPrivateVariables.map_list[map_name.at(index)]->border.bottom.x_offset=semi_loaded_map.at(index).border.bottom.x_offset-semi_loaded_map.at(map_name.indexOf(semi_loaded_map.at(index).border.bottom.fileName)).border.top.x_offset;
        else
            GlobalData::serverPrivateVariables.map_list[map_name.at(index)]->border.bottom.x_offset=0;
        if(GlobalData::serverPrivateVariables.map_list[map_name.at(index)]->border.top.map!=NULL)
            GlobalData::serverPrivateVariables.map_list[map_name.at(index)]->border.top.x_offset=semi_loaded_map.at(index).border.top.x_offset-semi_loaded_map.at(map_name.indexOf(semi_loaded_map.at(index).border.top.fileName)).border.bottom.x_offset;
        else
            GlobalData::serverPrivateVariables.map_list[map_name.at(index)]->border.top.x_offset=0;
        if(GlobalData::serverPrivateVariables.map_list[map_name.at(index)]->border.left.map!=NULL)
            GlobalData::serverPrivateVariables.map_list[map_name.at(index)]->border.left.y_offset=semi_loaded_map.at(index).border.left.y_offset-semi_loaded_map.at(map_name.indexOf(semi_loaded_map.at(index).border.left.fileName)).border.right.y_offset;
        else
            GlobalData::serverPrivateVariables.map_list[map_name.at(index)]->border.left.y_offset=0;
        if(GlobalData::serverPrivateVariables.map_list[map_name.at(index)]->border.right.map!=NULL)
            GlobalData::serverPrivateVariables.map_list[map_name.at(index)]->border.right.y_offset=semi_loaded_map.at(index).border.right.y_offset-semi_loaded_map.at(map_name.indexOf(semi_loaded_map.at(index).border.right.fileName)).border.left.y_offset;
        else
            GlobalData::serverPrivateVariables.map_list[map_name.at(index)]->border.right.y_offset=0;
        index++;
    }

    size=full_map_name.size();
    index=0;
    while(index<size)
    {
        if(GlobalData::serverPrivateVariables.map_list.contains(full_map_name.at(index)))
            GlobalData::serverPrivateVariables.map_list[full_map_name.at(index)]->id=index;
        index++;
    }

    DebugClass::debugConsole(QString("%1 map(s) loaded").arg(GlobalData::serverPrivateVariables.map_list.size()));
}

void BaseServer::preload_the_skin()
{
    QStringList skinFolderList=FacilityLib::skinIdList(GlobalData::serverPrivateVariables.datapack_basePath+DATAPACK_BASE_PATH_SKIN);
    int index=0;
    while(index<skinFolderList.size())
    {
        GlobalData::serverPrivateVariables.skinList[skinFolderList.at(index)]=index;
        index++;
    }

    DebugClass::debugConsole(QString("%1 skin(s) loaded").arg(GlobalData::serverPrivateVariables.skinList.size()));
}

void BaseServer::preload_the_items()
{
    //open and quick check the file
    QFile itemsFile(GlobalData::serverPrivateVariables.datapack_basePath+DATAPACK_BASE_PATH_ITEM+"items.xml");
    QByteArray xmlContent;
    if(!itemsFile.open(QIODevice::ReadOnly))
    {
        DebugClass::debugConsole(QString("Unable to open the items file: %1, error: %2").arg(itemsFile.fileName()).arg(itemsFile.errorString()));
        return;
    }
    xmlContent=itemsFile.readAll();
    itemsFile.close();
    QDomDocument domDocument;
    QString errorStr;
    int errorLine,errorColumn;
    if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
    {
        DebugClass::debugConsole(QString("Unable to open the items file: %1, Parse error at line %2, column %3: %4").arg(itemsFile.fileName()).arg(errorLine).arg(errorColumn).arg(errorStr));
        return;
    }
    QDomElement root = domDocument.documentElement();
    if(root.tagName()!="items")
    {
        DebugClass::debugConsole(QString("Unable to open the items file: %1, \"items\" root balise not found for the xml file").arg(itemsFile.fileName()));
        return;
    }

    //load the content
    bool ok;
    QDomElement item = root.firstChildElement("item");
    while(!item.isNull())
    {
        if(item.isElement())
        {
            if(item.hasAttribute("id"))
            {
                quint32 id=item.attribute("id").toULongLong(&ok);
                if(ok)
                {
                    if(!GlobalData::serverPrivateVariables.itemsId.contains(id))
                        GlobalData::serverPrivateVariables.itemsId << id;
                    else
                        DebugClass::debugConsole(QString("Unable to open the items file: %1, id number already set: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                }
                else
                    DebugClass::debugConsole(QString("Unable to open the items file: %1, id is not a number: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
            }
            else
                DebugClass::debugConsole(QString("Unable to open the items file: %1, have not the item id: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
        }
        else
            DebugClass::debugConsole(QString("Unable to open the items file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
        item = item.nextSiblingElement("item");
    }

    DebugClass::debugConsole(QString("%1 item(s) loaded").arg(GlobalData::serverPrivateVariables.itemsId.size()));
}

void BaseServer::preload_the_datapack()
{
    QStringList returnList=FacilityLib::listFolder(GlobalData::serverPrivateVariables.datapack_basePath);
    int index=0;
    int size=returnList.size();
    while(index<size)
    {
        QString fileName=returnList.at(index);
        if(fileName.contains(GlobalData::serverPrivateVariables.datapack_rightFileName))
        {
            QFile file(GlobalData::serverPrivateVariables.datapack_basePath+returnList.at(index));
            if(file.size()<=8*1024*1024)
            {
                if(file.open(QIODevice::ReadOnly))
                {
                    fileName.replace("\\","/");//remplace if is under windows server
                    GlobalData::serverPrivateVariables.filesList[fileName]=QFileInfo(file).lastModified().toTime_t();
                    file.close();
                }
            }
        }
        index++;
    }
    DebugClass::debugConsole(QString("%1 file(s) list for datapack cached").arg(GlobalData::serverPrivateVariables.filesList.size()));
}

void BaseServer::preload_the_players()
{
    ClientHeavyLoad::simplifiedIdList.clear();
    int index=0;
    while(index<GlobalData::serverSettings.max_players)
    {
        ClientHeavyLoad::simplifiedIdList << index;
        index++;
    }
}

void BaseServer::preload_the_visibility_algorithm()
{
    QHash<QString,Map *>::const_iterator i = GlobalData::serverPrivateVariables.map_list.constBegin();
    QHash<QString,Map *>::const_iterator i_end = GlobalData::serverPrivateVariables.map_list.constEnd();
    switch(GlobalData::serverSettings.mapVisibility.mapVisibilityAlgorithm)
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

void BaseServer::parseJustLoadedMap(const Map_to_send &,const QString &)
{
}

bool BaseServer::initialize_the_database()
{
    if(GlobalData::serverPrivateVariables.db!=NULL)
    {
        GlobalData::serverPrivateVariables.db->close();
        delete GlobalData::serverPrivateVariables.db;
        GlobalData::serverPrivateVariables.db=NULL;
    }
    switch(GlobalData::serverSettings.database.type)
    {
        default:
        DebugClass::debugConsole(QString("database type unknow"));
        return false;
        case ServerSettings::Database::DatabaseType_Mysql:
        GlobalData::serverPrivateVariables.db = new QSqlDatabase();
        *GlobalData::serverPrivateVariables.db = QSqlDatabase::addDatabase("QMYSQL");
        GlobalData::serverPrivateVariables.db->setConnectOptions("MYSQL_OPT_RECONNECT=1");
        GlobalData::serverPrivateVariables.db->setHostName(GlobalData::serverSettings.database.mysql.host);
        GlobalData::serverPrivateVariables.db->setDatabaseName(GlobalData::serverSettings.database.mysql.db);
        GlobalData::serverPrivateVariables.db->setUserName(GlobalData::serverSettings.database.mysql.login);
        GlobalData::serverPrivateVariables.db->setPassword(GlobalData::serverSettings.database.mysql.pass);
        GlobalData::serverPrivateVariables.db_type_string="mysql";
        break;
        case ServerSettings::Database::DatabaseType_SQLite:
        GlobalData::serverPrivateVariables.db = new QSqlDatabase();
        *GlobalData::serverPrivateVariables.db = QSqlDatabase::addDatabase("QSQLITE");
        GlobalData::serverPrivateVariables.db->setDatabaseName(GlobalData::serverSettings.database.sqlite.file);
        GlobalData::serverPrivateVariables.db_type_string="sqlite";
        break;
    }
    if(!GlobalData::serverPrivateVariables.db->open())
    {
        DebugClass::debugConsole(QString("Unable to connect to the database: %1, with the login: %2, database text: %3").arg(GlobalData::serverPrivateVariables.db->lastError().driverText()).arg(GlobalData::serverSettings.database.mysql.login).arg(GlobalData::serverPrivateVariables.db->lastError().databaseText()));
        emit error(QString("Unable to connect to the database: %1, with the login: %2, database text: %3").arg(GlobalData::serverPrivateVariables.db->lastError().driverText()).arg(GlobalData::serverSettings.database.mysql.login).arg(GlobalData::serverPrivateVariables.db->lastError().databaseText()));
        return false;
    }
    DebugClass::debugConsole(QString("Connected to %1 at %2 (%3)")
                             .arg(GlobalData::serverPrivateVariables.db_type_string)
                             .arg(GlobalData::serverSettings.database.mysql.host)
                             .arg(GlobalData::serverPrivateVariables.db->isOpen()));
    return true;
}

////////////////////////////////////////////////// server stopping ////////////////////////////////////////////

void BaseServer::unload_the_data()
{
    GlobalData::serverPrivateVariables.stopIt=true;

    unload_the_plant_on_map();
    unload_the_plant();
    unload_the_items();
    unload_the_skin();
    unload_the_map();
    unload_the_datapack();
    unload_the_players();
}

void BaseServer::unload_the_map()
{
    QHash<QString,Map *>::const_iterator i = GlobalData::serverPrivateVariables.map_list.constBegin();
    QHash<QString,Map *>::const_iterator i_end = GlobalData::serverPrivateVariables.map_list.constEnd();
    while (i != i_end)
    {
        if(i.value()->parsed_layer.walkable!=NULL)
            delete i.value()->parsed_layer.walkable;
        if(i.value()->parsed_layer.water!=NULL)
            delete i.value()->parsed_layer.water;
        if(i.value()->parsed_layer.grass!=NULL)
            delete i.value()->parsed_layer.grass;
        delete i.value();
        i++;
    }
    GlobalData::serverPrivateVariables.map_list.clear();
    GlobalData::serverPrivateVariables.botSpawn.clear();
}

void BaseServer::unload_the_skin()
{
    GlobalData::serverPrivateVariables.skinList.clear();
}

void BaseServer::unload_the_visibility_algorithm()
{
}

void BaseServer::unload_the_items()
{
    GlobalData::serverPrivateVariables.itemsId.clear();
}

void BaseServer::unload_the_datapack()
{
    GlobalData::serverPrivateVariables.filesList.clear();
}

void BaseServer::unload_the_players()
{
    ClientHeavyLoad::simplifiedIdList.clear();
}

void BaseServer::check_if_now_stopped()
{
    if(client_list.size()!=0)
        return;
    if(stat!=InDown)
        return;

    DebugClass::debugConsole("Fully stopped");
    if(GlobalData::serverPrivateVariables.db!=NULL)
        GlobalData::serverPrivateVariables.db->close();
    stat=Down;
    emit is_started(false);

    unload_the_data();
}

void BaseServer::setSettings(ServerSettings settings)
{
    //load it
    GlobalData::serverSettings=settings;

    loadAndFixSettings();
}

void BaseServer::loadAndFixSettings()
{
    //check the settings here
    if(GlobalData::serverSettings.max_players<1)
        GlobalData::serverSettings.max_players=200;
    ProtocolParsing::setMaxPlayers(GlobalData::serverSettings.max_players);

    quint8 player_list_size;
    if(GlobalData::serverSettings.max_players<=255)
        player_list_size=sizeof(quint8);
    else
        player_list_size=sizeof(quint16);
    quint8 map_list_size;
    if(GlobalData::serverPrivateVariables.map_list.size()<=255)
        map_list_size=sizeof(quint8);
    else if(GlobalData::serverPrivateVariables.map_list.size()<=65535)
        map_list_size=sizeof(quint16);
    else
        map_list_size=sizeof(quint32);
    GlobalData::serverPrivateVariables.sizeofInsertRequest=
            //mutualised
            sizeof(quint8)+map_list_size+/*player_list_size same with move, delete, ...*/
            //of the player
            /*player_list_size same with move, delete, ...*/+sizeof(quint8)+sizeof(quint8)+sizeof(quint8)+sizeof(quint8)+sizeof(quint8)+0/*pseudo size put directy*/+sizeof(quint8);

    if(GlobalData::serverSettings.mapVisibility.simple.max>GlobalData::serverSettings.max_players)
        GlobalData::serverSettings.mapVisibility.simple.max=GlobalData::serverSettings.max_players;
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
    switch(GlobalData::serverSettings.mapVisibility.mapVisibilityAlgorithm)
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
