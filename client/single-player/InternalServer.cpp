#include "InternalServer.h"
#include "../../server/base/GlobalData.h"

/*
  When disconnect the fake client, stop the benchmark
  */

using namespace Pokecraft;

bool InternalServer::oneInstanceRunning=false;

InternalServer::InternalServer()
{
	ProtocolParsing::initialiseTheVariable();

    GlobalData::serverPrivateVariables.db                   = NULL;
    GlobalData::serverPrivateVariables.timer_player_map     = NULL;
    lunchInitFunction                                       = NULL;

    GlobalData::serverPrivateVariables.botSpawnIndex=0;
    GlobalData::serverPrivateVariables.datapack_basePath		= QCoreApplication::applicationDirPath()+"/datapack/";
    GlobalData::serverPrivateVariables.datapack_rightFileName	= QRegExp(DATAPACK_FILE_REGEX);

    GlobalData::serverSettings.database.type=ServerSettings::Database::DatabaseType_SQLite;

    GlobalData::serverSettings.mapVisibility.mapVisibilityAlgorithm	= MapVisibilityAlgorithm_none;
    GlobalData::serverSettings.mapVisibility.simple.max		= 30;
    GlobalData::serverSettings.mapVisibility.simple.reshow		= 20;
    GlobalData::serverPrivateVariables.timer_to_send_insert_move_remove.start(POKECRAFT_SERVER_MAP_TIME_TO_SEND_MOVEMENT);

    GlobalData::serverPrivateVariables.eventThreaderList << new EventThreader();//broad cast (0)
    GlobalData::serverPrivateVariables.eventThreaderList << new EventThreader();//map management (1)
    GlobalData::serverPrivateVariables.eventThreaderList << new EventThreader();//network read (2)
    GlobalData::serverPrivateVariables.eventThreaderList << new EventThreader();//heavy load (3)
    GlobalData::serverPrivateVariables.eventThreaderList << new EventThreader();//event dispatcher (4)
    GlobalData::serverPrivateVariables.eventThreaderList << new EventThreader();//local calcule (6)

    GlobalData::serverPrivateVariables.player_updater.moveToThread(GlobalData::serverPrivateVariables.eventThreaderList.at(0));

	QStringList names;
    names << "broad cast" << "map management" << "network read" << "heavy load" << "event dispatcher";

	qRegisterMetaType<Chat_type>("Chat_type");
	qRegisterMetaType<Player_internal_informations>("Player_internal_informations");
	qRegisterMetaType<QList<quint32> >("QList<quint32>");
	qRegisterMetaType<Orientation>("Orientation");
	qRegisterMetaType<QList<QByteArray> >("QList<QByteArray>");
	qRegisterMetaType<Chat_type>("Chat_type");
	qRegisterMetaType<QAbstractSocket::SocketError>("QAbstractSocket::SocketError");
	qRegisterMetaType<Direction>("Direction");
    qRegisterMetaType<Map_server_MapVisibility_simple*>("Map_server_MapVisibility_simple*");
	qRegisterMetaType<Player_public_informations>("Player_public_informations");
	qRegisterMetaType<QSqlQuery>("QSqlQuery");

    connect(&server,SIGNAL(newConnection()),this,SLOT(newConnection()));
	connect(this,SIGNAL(need_be_started()),this,SLOT(start_internal_server()),Qt::QueuedConnection);
	connect(this,SIGNAL(try_stop_server()),this,SLOT(stop_internal_server()),Qt::QueuedConnection);

	stat=Down;
    moveToThread(GlobalData::serverPrivateVariables.eventThreaderList.at(4));

	connect(this,SIGNAL(try_initAll()),this,SLOT(initAll()),Qt::QueuedConnection);
	emit try_initAll();

	srand(time(NULL));
}

/** call only when the server is down
 * \warning this function is thread safe because it quit all thread before remove */
InternalServer::~InternalServer()
{
    GlobalData::serverPrivateVariables.stopIt=true;
    lunchInitFunction->deleteLater();
	int index=0;
    while(index<GlobalData::serverPrivateVariables.eventThreaderList.size())
	{
        GlobalData::serverPrivateVariables.eventThreaderList.at(index)->quit();
		index++;
	}
    while(GlobalData::serverPrivateVariables.eventThreaderList.size()>0)
	{
        EventThreader * tempThread=GlobalData::serverPrivateVariables.eventThreaderList.first();
        GlobalData::serverPrivateVariables.eventThreaderList.removeFirst();
		tempThread->wait();
		delete tempThread;
	}
    if(GlobalData::serverPrivateVariables.db!=NULL)
	{
        GlobalData::serverPrivateVariables.db->close();
        delete GlobalData::serverPrivateVariables.db;
        GlobalData::serverPrivateVariables.db=NULL;
	}
}

void InternalServer::setSettings(ServerSettings settings)
{
	//check the settings here
	if(settings.max_players<1)
		settings.max_players=200;
	ProtocolParsing::setMaxPlayers(settings.max_players);
	//load it
    GlobalData::serverSettings=settings;
    DebugClass::debugConsole(QString("GlobalData::serverSettings.commmonServerSettings.sendPlayerNumber: %1").arg(GlobalData::serverSettings.commmonServerSettings.sendPlayerNumber));

	quint8 player_list_size;
	if(settings.max_players<=255)
		player_list_size=sizeof(quint8);
	else
		player_list_size=sizeof(quint16);
    GlobalData::serverPrivateVariables.sizeofInsertRequest=player_list_size+sizeof(quint8)+sizeof(quint8)+sizeof(quint8);
}

void InternalServer::initAll()
{
    lunchInitFunction=new QTimer();
}

//////////////////////////////////////////// server starting //////////////////////////////////////

void InternalServer::preload_the_data()
{
    GlobalData::serverPrivateVariables.stopIt=false;

	preload_the_map();
	preload_the_visibility_algorithm();

	ClientHeavyLoad::simplifiedIdList.clear();
	int index=0;
    while(index<GlobalData::serverSettings.max_players)
	{
		ClientHeavyLoad::simplifiedIdList << index;
		index++;
	}
}

void InternalServer::preload_the_map()
{
    GlobalData::serverPrivateVariables.datapack_mapPath=GlobalData::serverPrivateVariables.datapack_basePath+"map/";
    DebugClass::debugConsole(QString("start preload the map, into: %1").arg(GlobalData::serverPrivateVariables.datapack_mapPath));
	Map_loader map_temp;
	QList<Map_semi> semi_loaded_map;
	QStringList map_name;
    QStringList returnList=listFolder(GlobalData::serverPrivateVariables.datapack_mapPath,"");

	//load the map
	int size=returnList.size();
    int index=0;
	QRegExp mapFilter("\\.tmx$");
	while(index<size)
	{
		if(returnList.at(index).contains(mapFilter))
		{
			DebugClass::debugConsole(QString("load the map: %1").arg(returnList.at(index)));
            if(map_temp.tryLoadMap(GlobalData::serverPrivateVariables.datapack_mapPath+returnList.at(index)))
			{
                switch(GlobalData::serverSettings.mapVisibility.mapVisibilityAlgorithm)
				{
					case MapVisibilityAlgorithm_simple:
                        GlobalData::serverPrivateVariables.map_list[returnList.at(index)]=new Map_server_MapVisibility_simple;
					break;
					case MapVisibilityAlgorithm_none:
					default:
                        GlobalData::serverPrivateVariables.map_list[returnList.at(index)]=new Map;
					break;
				}

                GlobalData::serverPrivateVariables.map_list[returnList.at(index)]->width			= map_temp.map_to_send.width;
                GlobalData::serverPrivateVariables.map_list[returnList.at(index)]->height			= map_temp.map_to_send.height;
                GlobalData::serverPrivateVariables.map_list[returnList.at(index)]->parsed_layer.walkable	= map_temp.map_to_send.parsed_layer.walkable;
                GlobalData::serverPrivateVariables.map_list[returnList.at(index)]->parsed_layer.water		= map_temp.map_to_send.parsed_layer.water;
                GlobalData::serverPrivateVariables.map_list[returnList.at(index)]->map_file			= returnList.at(index);

				bool continueTheLoading=true;
                switch(GlobalData::serverSettings.mapVisibility.mapVisibilityAlgorithm)
				{
					case MapVisibilityAlgorithm_simple:
                        continueTheLoading=GlobalData::serverPrivateVariables.map_list[returnList.at(index)]->loadInternalVariables();
					break;
					case MapVisibilityAlgorithm_none:
					default:
					break;
				}

				if(continueTheLoading)
				{
					map_name << returnList.at(index);

					Map_semi map_semi;
                    map_semi.map				= GlobalData::serverPrivateVariables.map_list[returnList.at(index)];

					if(!map_temp.map_to_send.border.top.fileName.isEmpty())
					{
                        map_semi.border.top.fileName		= Map_loader::resolvRelativeMap(GlobalData::serverPrivateVariables.datapack_mapPath+returnList.at(index),map_temp.map_to_send.border.top.fileName,GlobalData::serverPrivateVariables.datapack_mapPath);
						map_semi.border.top.x_offset		= map_temp.map_to_send.border.top.x_offset;
					}
					if(!map_temp.map_to_send.border.bottom.fileName.isEmpty())
					{
                        map_semi.border.bottom.fileName		= Map_loader::resolvRelativeMap(GlobalData::serverPrivateVariables.datapack_mapPath+returnList.at(index),map_temp.map_to_send.border.bottom.fileName,GlobalData::serverPrivateVariables.datapack_mapPath);
						map_semi.border.bottom.x_offset		= map_temp.map_to_send.border.bottom.x_offset;
					}
					if(!map_temp.map_to_send.border.left.fileName.isEmpty())
					{
                        map_semi.border.left.fileName		= Map_loader::resolvRelativeMap(GlobalData::serverPrivateVariables.datapack_mapPath+returnList.at(index),map_temp.map_to_send.border.left.fileName,GlobalData::serverPrivateVariables.datapack_mapPath);
                        map_semi.border.left.y_offset		= map_temp.map_to_send.border.left.y_offset;
					}
					if(!map_temp.map_to_send.border.right.fileName.isEmpty())
					{
                        map_semi.border.right.fileName		= Map_loader::resolvRelativeMap(GlobalData::serverPrivateVariables.datapack_mapPath+returnList.at(index),map_temp.map_to_send.border.right.fileName,GlobalData::serverPrivateVariables.datapack_mapPath);
                        map_semi.border.right.y_offset		= map_temp.map_to_send.border.right.y_offset;
					}
					
					int sub_index=0;
					while(sub_index<map_temp.map_to_send.teleport.size())
					{
                        map_temp.map_to_send.teleport[sub_index].map=Map_loader::resolvRelativeMap(GlobalData::serverPrivateVariables.datapack_mapPath+returnList.at(index),map_temp.map_to_send.teleport.at(sub_index).map,GlobalData::serverPrivateVariables.datapack_mapPath);
						sub_index++;
					}

					map_semi.old_map=map_temp.map_to_send;

					semi_loaded_map << map_semi;
				}
			}
			else
				DebugClass::debugConsole(QString("error at loading: %1, error: %2").arg(returnList.at(index)).arg(map_temp.errorString()));
		}
		index++;
	}

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
					DebugClass::debugConsole(QString("teleporter on the map: %1(%2,%3), to %4 (%5,%6)")
								 .arg(semi_loaded_map[index].map->map_file)
								 .arg(semi_loaded_map[index].old_map.teleport.at(sub_index).source_x)
								 .arg(semi_loaded_map[index].old_map.teleport.at(sub_index).source_y)
								 .arg(semi_loaded_map[index].old_map.teleport.at(sub_index).map)
								 .arg(semi_loaded_map[index].old_map.teleport.at(sub_index).destination_x)
								 .arg(semi_loaded_map[index].old_map.teleport.at(sub_index).destination_y));
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

	DebugClass::debugConsole(QString("finish preload the map"));
}

void InternalServer::preload_the_visibility_algorithm()
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
		default:
		break;
	}
}

//start with allow real player to connect
void InternalServer::start_internal_server()
{
	if(oneInstanceRunning)
	{
		DebugClass::debugConsole("Other instance already running");
		return;
	}
	if(stat!=Down)
	{
		DebugClass::debugConsole("In wrong stat");
		return;
	}
    stat=InUp;
	QHostAddress address = QHostAddress::Any;

	if(!initialize_the_database())
		return;

    if(GlobalData::serverPrivateVariables.db->isOpen())
        GlobalData::serverPrivateVariables.db->close();
    if(!GlobalData::serverPrivateVariables.db->open())
	{
        DebugClass::debugConsole(QString("Unable to connect to the database: %1, with the login: %2, database text: %3").arg(GlobalData::serverPrivateVariables.db->lastError().driverText()).arg(GlobalData::serverSettings.database.mysql.login).arg(GlobalData::serverPrivateVariables.db->lastError().databaseText()));
		stat=Down;
        emit error(QString("Unable to connect to the database: %1, with the login: %2, database text: %3").arg(GlobalData::serverPrivateVariables.db->lastError().driverText()).arg(GlobalData::serverSettings.database.mysql.login).arg(GlobalData::serverPrivateVariables.db->lastError().databaseText()));
		return;
	}
    DebugClass::debugConsole(QString("Connected to %1 at %2 (%3)").arg(GlobalData::serverPrivateVariables.db_type_string).arg(GlobalData::serverSettings.database.mysql.host).arg(GlobalData::serverPrivateVariables.db->isOpen()));
	preload_the_data();
	stat=Up;
	oneInstanceRunning=true;
	emit is_started(true);
	return;
}

bool InternalServer::initialize_the_database()
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
        case ServerSettings::Database::DatabaseType_Mysql:
        GlobalData::serverPrivateVariables.db = new QSqlDatabase();
        *GlobalData::serverPrivateVariables.db = QSqlDatabase::addDatabase("QMYSQL");
        GlobalData::serverPrivateVariables.db->setConnectOptions("MYSQL_OPT_RECONNECT=1");
        GlobalData::serverPrivateVariables.db->setHostName(GlobalData::serverSettings.database.mysql.host);
        GlobalData::serverPrivateVariables.db->setDatabaseName(GlobalData::serverSettings.database.mysql.db);
        GlobalData::serverPrivateVariables.db->setUserName(GlobalData::serverSettings.database.mysql.login);
        GlobalData::serverPrivateVariables.db->setPassword(GlobalData::serverSettings.database.mysql.pass);
        GlobalData::serverPrivateVariables.db_type_string="mysql";
		return true;
		break;
        case ServerSettings::Database::DatabaseType_SQLite:
        GlobalData::serverPrivateVariables.db = new QSqlDatabase();
        *GlobalData::serverPrivateVariables.db = QSqlDatabase::addDatabase("QSQLITE");
        GlobalData::serverPrivateVariables.db->setDatabaseName(QCoreApplication::applicationDirPath()+"/pokecraft.db.sqlite");
        GlobalData::serverPrivateVariables.db_type_string="sqlite";
		return true;
		break;
	}
}

////////////////////////////////////////////////// server stopping ////////////////////////////////////////////

void InternalServer::unload_the_data()
{
    GlobalData::serverPrivateVariables.stopIt=true;

	unload_the_map();
	unload_the_visibility_algorithm();

	ClientHeavyLoad::simplifiedIdList.clear();
}

void InternalServer::unload_the_map()
{
    QHash<QString,Map *>::const_iterator i = GlobalData::serverPrivateVariables.map_list.constBegin();
    QHash<QString,Map *>::const_iterator i_end = GlobalData::serverPrivateVariables.map_list.constEnd();
	while (i != i_end)
	{
		delete i.value()->parsed_layer.walkable;
		delete i.value()->parsed_layer.water;
		delete i.value();
		i++;
	}
    GlobalData::serverPrivateVariables.map_list.clear();
    GlobalData::serverPrivateVariables.botSpawn.clear();
}

void InternalServer::unload_the_visibility_algorithm()
{
}

void InternalServer::check_if_now_stopped()
{
    if(client_list.size()!=0 || GlobalData::serverPrivateVariables.fake_clients.size()!=0)
		return;
	if(stat!=InDown)
		return;
	DebugClass::debugConsole("Fully stopped");
    if(GlobalData::serverPrivateVariables.db!=NULL)
        GlobalData::serverPrivateVariables.db->close();
	//server.close();
	stat=Down;
	oneInstanceRunning=false;

	unload_the_data();
	emit is_started(false);
}

//call by normal stop
void InternalServer::stop_internal_server()
{
	if(stat!=Up && stat!=InDown)
	{
		if(stat!=Down)
			DebugClass::debugConsole("Is in wrong stat for stopping: "+QString::number((int)stat));
		return;
	}
	DebugClass::debugConsole("Try stop");
    stat=InDown;

	int index=0;
	while(index<client_list.size())
	{
		client_list.at(index)->disconnectClient();
		index++;
	}

	/* commented because it need be unloaded after all the player
	unload_the_data();*/

	check_if_now_stopped();
}

/////////////////////////////////////////////////// Object removing /////////////////////////////////////

void InternalServer::removeOneClient()
{
	Client *client=qobject_cast<Client *>(QObject::sender());
	if(client==NULL)
	{
		DebugClass::debugConsole("removeOneClient(): NULL client at disconnection");
		return;
	}
	if(!client->is_ready_to_stop)
	{
		DebugClass::debugConsole("is not ready to stop!");
		return;
	}
	client_list.removeOne(client);
	client->deleteLater();
	check_if_now_stopped();
}

/////////////////////////////////////// player related //////////////////////////////////////
QStringList InternalServer::listFolder(const QString& folder,const QString& suffix)
{
	QStringList returnList;
	QFileInfoList entryList=QDir(folder+suffix).entryInfoList(QDir::AllEntries|QDir::NoDotAndDotDot|QDir::Hidden|QDir::System,QDir::DirsFirst);//possible wait time here
	int sizeEntryList=entryList.size();
	for (int index=0;index<sizeEntryList;++index)
	{
		QFileInfo fileInfo=entryList.at(index);
		if(fileInfo.isDir())
			returnList+=listFolder(folder,suffix+fileInfo.fileName()+"/");//put unix separator because it's transformed into that's under windows too
		else if(fileInfo.isFile())
			returnList+=suffix+fileInfo.fileName();
	}
	return returnList;
}

void InternalServer::newConnection()
{
    while(server.hasPendingConnections())
	{
		DebugClass::debugConsole(QString("new client connected"));
        QFakeSocket *socket = server.nextPendingConnection();
		if(socket!=NULL)
		{
            client_list << new Client(socket,false,new MapVisibilityAlgorithm_None());
			connect_the_last_client();
		}
		else
			DebugClass::debugConsole("NULL client: "+socket->peerAddress().toString());
    }
}

void InternalServer::connect_the_last_client()
{
	connect(client_list.last(),SIGNAL(isReadyToDelete()),this,SLOT(removeOneClient()),Qt::QueuedConnection);
}

bool InternalServer::isListen()
{
	return (stat==Up);
}

bool InternalServer::isStopped()
{
	return (stat==Down);
}

/////////////////////////////////// Async the call ///////////////////////////////////
/// \brief Called when event loop is setup
void InternalServer::start_server()
{
	emit need_be_started();
}

void InternalServer::stop_server()
{
	emit try_stop_server();
}
