#include "BaseServer.h"
#include "GlobalServerData.h"
#include "../../general/base/FacilityLib.h"

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
    qRegisterMetaType<Direction>("Direction");
    qRegisterMetaType<Map_server_MapVisibility_simple*>("Map_server_MapVisibility_simple*");
    qRegisterMetaType<Player_public_informations>("Player_public_informations");
    qRegisterMetaType<QSqlQuery>("QSqlQuery");

    GlobalServerData::serverPrivateVariables.connected_players	= 0;
    GlobalServerData::serverPrivateVariables.number_of_bots_logged= 0;
    GlobalServerData::serverPrivateVariables.db                   = NULL;
    GlobalServerData::serverPrivateVariables.timer_player_map     = NULL;

    GlobalServerData::serverPrivateVariables.botSpawnIndex=0;
    GlobalServerData::serverPrivateVariables.datapack_basePath		= QCoreApplication::applicationDirPath()+"/datapack/";
    GlobalServerData::serverPrivateVariables.datapack_rightFileName	= QRegExp(DATAPACK_FILE_REGEX);

    GlobalServerData::serverSettings.database.type=ServerSettings::Database::DatabaseType_Mysql;
    GlobalServerData::serverSettings.mapVisibility.mapVisibilityAlgorithm	= MapVisibilityAlgorithm_simple;
    GlobalServerData::serverSettings.mapVisibility.simple.max		= 30;
    GlobalServerData::serverSettings.mapVisibility.simple.reshow		= 20;
    GlobalServerData::serverPrivateVariables.timer_to_send_insert_move_remove.start(CATCHCHALLENGER_SERVER_MAP_TIME_TO_SEND_MOVEMENT);

    GlobalServerData::serverSettings.database.fightSync=ServerSettings::Database::FightSync_AtTheEndOfBattle;
    GlobalServerData::serverSettings.database.positionTeleportSync=true;
    GlobalServerData::serverSettings.database.secondToPositionSync=0;

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
    if(GlobalServerData::serverPrivateVariables.db!=NULL)
    {
        GlobalServerData::serverPrivateVariables.db->close();
        delete GlobalServerData::serverPrivateVariables.db;
        GlobalServerData::serverPrivateVariables.db=NULL;
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

    preload_the_datapack();
    preload_the_skin();
    preload_the_items();
    preload_shop();
    preload_the_players();
    preload_the_plant();
    preload_crafting_recipes();
    preload_the_map();
    preload_the_plant_on_map();
    preload_buff();
    preload_skills();
    preload_monsters();
    preload_monsters_drops();
    check_monsters_map();
}

void BaseServer::preload_the_map()
{
    GlobalServerData::serverPrivateVariables.datapack_mapPath=GlobalServerData::serverPrivateVariables.datapack_basePath+DATAPACK_BASE_PATH_MAP;
    #ifdef DEBUG_MESSAGE_MAP_LOAD
    DebugClass::debugConsole(QString("start preload the map, into: %1").arg(GlobalServerData::serverPrivateVariables.datapack_mapPath));
    #endif
    int shops_number=0;
    Map_loader map_temp;
    QList<Map_semi> semi_loaded_map;
    QStringList map_name;
    QStringList full_map_name;
    QStringList returnList=FacilityLib::listFolder(GlobalServerData::serverPrivateVariables.datapack_mapPath);

    //load the map
    int size=returnList.size();
    int index=0;
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

                GlobalServerData::serverPrivateVariables.map_list[fileName];
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

                int sub_index=0;
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
            if(GlobalServerData::serverPrivateVariables.map_list.contains(semi_loaded_map[index].old_map.teleport.at(sub_index).map)
                    && semi_loaded_map[index].old_map.teleport.at(sub_index).destination_x<GlobalServerData::serverPrivateVariables.map_list[semi_loaded_map[index].old_map.teleport.at(sub_index).map]->width
                    && semi_loaded_map[index].old_map.teleport.at(sub_index).destination_y<GlobalServerData::serverPrivateVariables.map_list[semi_loaded_map[index].old_map.teleport.at(sub_index).map]->height
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
                    semi_loaded_map[index].map->teleporter[virtual_position].map=GlobalServerData::serverPrivateVariables.map_list[semi_loaded_map[index].old_map.teleport.at(sub_index).map];
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

    //resolv the shops
    size=semi_loaded_map.size();
    index=0;
    while(index<size)
    {
        int sub_index=0;
        while(sub_index<semi_loaded_map[index].old_map.bots.size())
        {
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
                                bool ok;
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
                        step = step.nextSiblingElement("step");
                    }
                }
            sub_index++;
        }
        index++;
    }


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

    size=full_map_name.size();
    index=0;
    while(index<size)
    {
        if(GlobalServerData::serverPrivateVariables.map_list.contains(full_map_name.at(index)))
            GlobalServerData::serverPrivateVariables.map_list[full_map_name.at(index)]->id=index;
        index++;
    }

    DebugClass::debugConsole(QString("%1 map(s) loaded").arg(GlobalServerData::serverPrivateVariables.map_list.size()));
    DebugClass::debugConsole(QString("%1 shop(s) on map loaded").arg(shops_number));

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

void BaseServer::preload_the_items()
{
    //open and quick check the file
    QFile itemsFile(GlobalServerData::serverPrivateVariables.datapack_basePath+DATAPACK_BASE_PATH_ITEM+"items.xml");
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
                    if(!GlobalServerData::serverPrivateVariables.items.contains(id))
                    {
                        quint32 price=0;
                        if(item.hasAttribute("price"))
                        {
                            price=item.attribute("price").toUInt(&ok);
                            if(!ok)
                            {
                                DebugClass::debugConsole(QString("Unable to open the items file: %1, price is not a number: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                                price=0;
                            }
                        }
                        Item item;
                        item.price=price;
                        GlobalServerData::serverPrivateVariables.items[id]=item;
                    }
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

    DebugClass::debugConsole(QString("%1 item(s) loaded").arg(GlobalServerData::serverPrivateVariables.items.size()));
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

void BaseServer::parseJustLoadedMap(const Map_to_send &,const QString &)
{
}

bool BaseServer::initialize_the_database()
{
    if(GlobalServerData::serverPrivateVariables.db!=NULL)
    {
        GlobalServerData::serverPrivateVariables.db->close();
        delete GlobalServerData::serverPrivateVariables.db;
        GlobalServerData::serverPrivateVariables.db=NULL;
    }
    switch(GlobalServerData::serverSettings.database.type)
    {
        default:
        DebugClass::debugConsole(QString("database type unknow"));
        return false;
        case ServerSettings::Database::DatabaseType_Mysql:
        GlobalServerData::serverPrivateVariables.db = new QSqlDatabase();
        *GlobalServerData::serverPrivateVariables.db = QSqlDatabase::addDatabase("QMYSQL");
        GlobalServerData::serverPrivateVariables.db->setConnectOptions("MYSQL_OPT_RECONNECT=1");
        GlobalServerData::serverPrivateVariables.db->setHostName(GlobalServerData::serverSettings.database.mysql.host);
        GlobalServerData::serverPrivateVariables.db->setDatabaseName(GlobalServerData::serverSettings.database.mysql.db);
        GlobalServerData::serverPrivateVariables.db->setUserName(GlobalServerData::serverSettings.database.mysql.login);
        GlobalServerData::serverPrivateVariables.db->setPassword(GlobalServerData::serverSettings.database.mysql.pass);
        GlobalServerData::serverPrivateVariables.db_type_string="mysql";
        break;
        case ServerSettings::Database::DatabaseType_SQLite:
        GlobalServerData::serverPrivateVariables.db = new QSqlDatabase();
        *GlobalServerData::serverPrivateVariables.db = QSqlDatabase::addDatabase("QSQLITE");
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

    unload_monsters_drops();
    unload_monsters();
    unload_skills();
    unload_buff();
    unload_the_plant_on_map();
    unload_the_map();
    unload_crafting_recipes();
    unload_the_plant();
    unload_shop();
    unload_the_items();
    unload_the_skin();
    unload_the_datapack();
    unload_the_players();
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

void BaseServer::unload_the_items()
{
    GlobalServerData::serverPrivateVariables.items.clear();
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
        GlobalServerData::serverPrivateVariables.db->close();
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
