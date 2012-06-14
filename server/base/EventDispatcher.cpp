#include "EventDispatcher.h"

/*
  When disconnect the fake client, stop the benchmark
  */

GeneralData EventDispatcher::generalData;

EventDispatcher::EventDispatcher()
{
	generalData.serverPrivateVariables.connected_players	= 0;
	generalData.serverPrivateVariables.db			= NULL;
	generalData.serverPrivateVariables.timer_player_map	= NULL;
	server							= NULL;
	lunchInitFunction					= NULL;
	timer_benchmark_stop					= NULL;

	generalData.serverPrivateVariables.datapack_mapPath		= QCoreApplication::applicationDirPath()+"/datapack/map/tmx/";
	generalData.serverPrivateVariables.datapack_basePath		= QCoreApplication::applicationDirPath()+"/datapack/";
	generalData.serverPrivateVariables.datapack_rightFileName	= QRegExp("^[0-9/a-zA-Z\\.\\- _]+\\.[a-z]{3,4}$");

	generalData.serverSettings.database.type=GeneralData::ServerSettings::Database::DatabaseType_Mysql;

	generalData.serverSettings.mapVisibility.mapVisibilityAlgorithm	= MapVisibilityAlgorithm_simple;
	generalData.serverSettings.mapVisibility.simple.max		= 30;
	generalData.serverSettings.mapVisibility.simple.reshow		= 20;
	generalData.serverPrivateVariables.timer_to_send_insert_move_remove.start(POKECRAFT_SERVER_MAP_TIME_TO_SEND_MOVEMENT);

	generalData.serverPrivateVariables.eventThreaderList << new EventThreader();//broad cast (0)
	generalData.serverPrivateVariables.eventThreaderList << new EventThreader();//map management (1)
	generalData.serverPrivateVariables.eventThreaderList << new EventThreader();//network read (2)
	generalData.serverPrivateVariables.eventThreaderList << new EventThreader();//heavy load (3)
	generalData.serverPrivateVariables.eventThreaderList << new EventThreader();//event dispatcher (4)
	generalData.serverPrivateVariables.eventThreaderList << new EventThreader();//bots (5)
	generalData.serverPrivateVariables.eventThreaderList << new EventThreader();//local calcule (6)

	generalData.serverPrivateVariables.player_updater.moveToThread(generalData.serverPrivateVariables.eventThreaderList.at(0));

	ProtocolParsing::packetSizeMode = PacketSizeMode_Small;
	ProtocolParsing::initialiseTheVariable(PacketModeTransmission_Server);

	QStringList names;
	names << "broad cast" << "map management" << "network read" << "heavy load" << "event dispatcher" << "benchmark";
	latencyChecker.setLatencyVariable(generalData.serverPrivateVariables.eventThreaderList,names);

	qRegisterMetaType<Chat_type>("Chat_type");

	connect(this,SIGNAL(try_start_benchmark(quint16,quint16)),this,SLOT(start_internal_benchmark(quint16,quint16)),Qt::QueuedConnection);
	connect(this,SIGNAL(need_be_started()),this,SLOT(start_internal_server()),Qt::QueuedConnection);
	connect(this,SIGNAL(try_stop_server()),this,SLOT(stop_internal_server()),Qt::QueuedConnection);

	in_benchmark_mode=false;
	stat=Down;
	moveToThread(generalData.serverPrivateVariables.eventThreaderList.at(4));

	connect(this,SIGNAL(try_initAll()),this,SLOT(initAll()),Qt::QueuedConnection);
	emit try_initAll();

	nextStep.start(POKECRAFT_SERVER_NORMAL_SPEED);
	srand(time(NULL));
}

/** call only when the server is down
 * \warning this function is thread safe because it quit all thread before remove */
EventDispatcher::~EventDispatcher()
{
	lunchInitFunction->deleteLater();
	timer_benchmark_stop->deleteLater();
	int index=0;
	while(index<generalData.serverPrivateVariables.eventThreaderList.size())
	{
		generalData.serverPrivateVariables.eventThreaderList.at(index)->quit();
		index++;
	}
	while(generalData.serverPrivateVariables.eventThreaderList.size()>0)
	{
		EventThreader * tempThread=generalData.serverPrivateVariables.eventThreaderList.first();
		generalData.serverPrivateVariables.eventThreaderList.removeFirst();
		tempThread->wait();
		delete tempThread;
	}
	if(generalData.serverPrivateVariables.db!=NULL)
	{
		generalData.serverPrivateVariables.db->close();
		delete generalData.serverPrivateVariables.db;
		generalData.serverPrivateVariables.db=NULL;
	}
	if(server!=NULL)
	{
		server->close();
		delete server;
		server=NULL;
	}
}

void EventDispatcher::setSettings(GeneralData::ServerSettings settings)
{
	generalData.serverSettings=settings;
}

void EventDispatcher::initAll()
{
	lunchInitFunction=new QTimer();
	timer_benchmark_stop=new QTimer();
	timer_benchmark_stop->setInterval(60000);//1min
	timer_benchmark_stop->setSingleShot(true);
	connect(timer_benchmark_stop,SIGNAL(timeout()),this,SLOT(stop_benchmark()),Qt::QueuedConnection);
}

//////////////////////////////////////////// server starting //////////////////////////////////////

void EventDispatcher::preload_the_data()
{
	preload_the_map();
	preload_the_visibility_algorithm();
}

void EventDispatcher::preload_the_map()
{
	DebugClass::debugConsole(QString("start preload the map"));
	Map_loader map_temp;
	QList<Map_semi> semi_loaded_map;
	QStringList map_name;
	QStringList returnList=listFolder(generalData.serverPrivateVariables.mapBasePath,"");

	//load the map
	int size=returnList.size();
	int index=0;
	QRegExp mapFilter("\\.tmx$");
	while(index<size)
	{
		if(returnList.at(index).contains(mapFilter))
		{
			DebugClass::debugConsole(QString("load the map: %1").arg(returnList.at(index)));
			if(map_temp.tryLoadMap(generalData.serverPrivateVariables.mapBasePath+returnList.at(index)))
			{
				generalData.serverPrivateVariables.map_list[returnList.at(index)]=new Map_final;
				generalData.serverPrivateVariables.map_list[returnList.at(index)]->width			= map_temp.map_to_send.width;
				generalData.serverPrivateVariables.map_list[returnList.at(index)]->height			= map_temp.map_to_send.height;
				generalData.serverPrivateVariables.map_list[returnList.at(index)]->parsed_layer.walkable	= map_temp.map_to_send.parsed_layer.walkable;
				generalData.serverPrivateVariables.map_list[returnList.at(index)]->parsed_layer.water		= map_temp.map_to_send.parsed_layer.water;
				generalData.serverPrivateVariables.map_list[returnList.at(index)]->map_file			= returnList.at(index);
				map_name << returnList.at(index);

				Map_semi map_semi;
				map_semi.map				= generalData.serverPrivateVariables.map_list[returnList.at(index)];

				map_semi.border.top.fileName		= map_temp.map_to_send.border.top.fileName;
				map_semi.border.top.x_offset		= map_temp.map_to_send.border.top.x_offset;

				map_semi.border.bottom.fileName		= map_temp.map_to_send.border.bottom.fileName;
				map_semi.border.bottom.x_offset		= map_temp.map_to_send.border.bottom.x_offset;

				map_semi.border.left.fileName		= map_temp.map_to_send.border.left.fileName;
				map_semi.border.left.y_offset		= map_temp.map_to_send.border.left.y_offset;

				map_semi.border.right.fileName		= map_temp.map_to_send.border.right.fileName;
				map_semi.border.right.y_offset		= map_temp.map_to_send.border.right.y_offset;

				map_semi.old_map=map_temp.map_to_send;

				semi_loaded_map << map_semi;
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
		if(semi_loaded_map.at(index).border.bottom.fileName!="" && generalData.serverPrivateVariables.map_list.contains(semi_loaded_map.at(index).border.bottom.fileName))
			semi_loaded_map[index].map->border.bottom.map=generalData.serverPrivateVariables.map_list[semi_loaded_map.at(index).border.bottom.fileName];
		else
			semi_loaded_map[index].map->border.bottom.map=NULL;

		if(semi_loaded_map.at(index).border.top.fileName!="" && generalData.serverPrivateVariables.map_list.contains(semi_loaded_map.at(index).border.top.fileName))
			semi_loaded_map[index].map->border.top.map=generalData.serverPrivateVariables.map_list[semi_loaded_map.at(index).border.top.fileName];
		else
			semi_loaded_map[index].map->border.top.map=NULL;

		if(semi_loaded_map.at(index).border.left.fileName!="" && generalData.serverPrivateVariables.map_list.contains(semi_loaded_map.at(index).border.left.fileName))
			semi_loaded_map[index].map->border.left.map=generalData.serverPrivateVariables.map_list[semi_loaded_map.at(index).border.left.fileName];
		else
			semi_loaded_map[index].map->border.left.map=NULL;

		if(semi_loaded_map.at(index).border.right.fileName!="" && generalData.serverPrivateVariables.map_list.contains(semi_loaded_map.at(index).border.right.fileName))
			semi_loaded_map[index].map->border.right.map=generalData.serverPrivateVariables.map_list[semi_loaded_map.at(index).border.right.fileName];
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
			if(generalData.serverPrivateVariables.map_list.contains(semi_loaded_map[index].old_map.teleport.at(sub_index).map)
					&& semi_loaded_map[index].old_map.teleport.at(sub_index).destination_x<generalData.serverPrivateVariables.map_list[semi_loaded_map[index].old_map.teleport.at(sub_index).map]->width
					&& semi_loaded_map[index].old_map.teleport.at(sub_index).destination_y<generalData.serverPrivateVariables.map_list[semi_loaded_map[index].old_map.teleport.at(sub_index).map]->height
				)
			{
				int virtual_position=semi_loaded_map[index].old_map.teleport.at(sub_index).source_x+semi_loaded_map[index].old_map.teleport.at(sub_index).source_y*semi_loaded_map[index].map->width;
				semi_loaded_map[index].map->teleporter[virtual_position].map=generalData.serverPrivateVariables.map_list[semi_loaded_map[index].old_map.teleport.at(sub_index).map];
				semi_loaded_map[index].map->teleporter[virtual_position].x=semi_loaded_map[index].old_map.teleport.at(sub_index).destination_x;
				semi_loaded_map[index].map->teleporter[virtual_position].y=semi_loaded_map[index].old_map.teleport.at(sub_index).destination_y;
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
		if(generalData.serverPrivateVariables.map_list[map_name.at(index)]->border.bottom.map!=NULL && generalData.serverPrivateVariables.map_list[map_name.at(index)]->border.bottom.map->border.top.map!=generalData.serverPrivateVariables.map_list[map_name.at(index)])
		{
			if(generalData.serverPrivateVariables.map_list[map_name.at(index)]->border.bottom.map->border.top.map==NULL)
				DebugClass::debugConsole(QString("the map %1 have bottom map: %2, the map %2 have not a top map").arg(generalData.serverPrivateVariables.map_list[map_name.at(index)]->map_file).arg(generalData.serverPrivateVariables.map_list[map_name.at(index)]->border.bottom.map->map_file));
			else
				DebugClass::debugConsole(QString("the map %1 have bottom map: %2, the map %2 have this top map: %3").arg(generalData.serverPrivateVariables.map_list[map_name.at(index)]->map_file).arg(generalData.serverPrivateVariables.map_list[map_name.at(index)]->border.bottom.map->map_file).arg(generalData.serverPrivateVariables.map_list[map_name.at(index)]->border.bottom.map->border.top.map->map_file));
			generalData.serverPrivateVariables.map_list[map_name.at(index)]->border.bottom.map=NULL;
		}
		if(generalData.serverPrivateVariables.map_list[map_name.at(index)]->border.top.map!=NULL && generalData.serverPrivateVariables.map_list[map_name.at(index)]->border.top.map->border.bottom.map!=generalData.serverPrivateVariables.map_list[map_name.at(index)])
		{
			if(generalData.serverPrivateVariables.map_list[map_name.at(index)]->border.top.map->border.bottom.map==NULL)
				DebugClass::debugConsole(QString("the map %1 have top map: %2, the map %2 have not a bottom map").arg(generalData.serverPrivateVariables.map_list[map_name.at(index)]->map_file).arg(generalData.serverPrivateVariables.map_list[map_name.at(index)]->border.top.map->map_file));
			else
				DebugClass::debugConsole(QString("the map %1 have top map: %2, the map %2 have this bottom map: %3").arg(generalData.serverPrivateVariables.map_list[map_name.at(index)]->map_file).arg(generalData.serverPrivateVariables.map_list[map_name.at(index)]->border.top.map->map_file).arg(generalData.serverPrivateVariables.map_list[map_name.at(index)]->border.top.map->border.bottom.map->map_file));
			generalData.serverPrivateVariables.map_list[map_name.at(index)]->border.top.map=NULL;
		}
		if(generalData.serverPrivateVariables.map_list[map_name.at(index)]->border.left.map!=NULL && generalData.serverPrivateVariables.map_list[map_name.at(index)]->border.left.map->border.right.map!=generalData.serverPrivateVariables.map_list[map_name.at(index)])
		{
			if(generalData.serverPrivateVariables.map_list[map_name.at(index)]->border.left.map->border.right.map==NULL)
				DebugClass::debugConsole(QString("the map %1 have left map: %2, the map %2 have not a right map").arg(generalData.serverPrivateVariables.map_list[map_name.at(index)]->map_file).arg(generalData.serverPrivateVariables.map_list[map_name.at(index)]->border.left.map->map_file));
			else
				DebugClass::debugConsole(QString("the map %1 have left map: %2, the map %2 have this right map: %3").arg(generalData.serverPrivateVariables.map_list[map_name.at(index)]->map_file).arg(generalData.serverPrivateVariables.map_list[map_name.at(index)]->border.left.map->map_file).arg(generalData.serverPrivateVariables.map_list[map_name.at(index)]->border.left.map->border.right.map->map_file));
			generalData.serverPrivateVariables.map_list[map_name.at(index)]->border.left.map=NULL;
		}
		if(generalData.serverPrivateVariables.map_list[map_name.at(index)]->border.right.map!=NULL && generalData.serverPrivateVariables.map_list[map_name.at(index)]->border.right.map->border.left.map!=generalData.serverPrivateVariables.map_list[map_name.at(index)])
		{
			if(generalData.serverPrivateVariables.map_list[map_name.at(index)]->border.right.map->border.left.map==NULL)
				DebugClass::debugConsole(QString("the map %1 have right map: %2, the map %2 have not a left map").arg(generalData.serverPrivateVariables.map_list[map_name.at(index)]->map_file).arg(generalData.serverPrivateVariables.map_list[map_name.at(index)]->border.right.map->map_file));
			else
				DebugClass::debugConsole(QString("the map %1 have right map: %2, the map %2 have this left map: %3").arg(generalData.serverPrivateVariables.map_list[map_name.at(index)]->map_file).arg(generalData.serverPrivateVariables.map_list[map_name.at(index)]->border.right.map->map_file).arg(generalData.serverPrivateVariables.map_list[map_name.at(index)]->border.right.map->border.left.map->map_file));
			generalData.serverPrivateVariables.map_list[map_name.at(index)]->border.right.map=NULL;
		}
		index++;
	}

	//resolv the near map
	size=semi_loaded_map.size();
	index=0;
	while(index<size)
	{
		generalData.serverPrivateVariables.map_list[map_name.at(index)]->near_map << generalData.serverPrivateVariables.map_list[map_name.at(index)];

		if(generalData.serverPrivateVariables.map_list[map_name.at(index)]->border.bottom.map!=NULL && !generalData.serverPrivateVariables.map_list[map_name.at(index)]->near_map.contains(generalData.serverPrivateVariables.map_list[map_name.at(index)]->border.bottom.map))
		{
			generalData.serverPrivateVariables.map_list[map_name.at(index)]->near_map << generalData.serverPrivateVariables.map_list[map_name.at(index)]->border.bottom.map;
			if(generalData.serverPrivateVariables.map_list[map_name.at(index)]->border.left.map!=NULL && !generalData.serverPrivateVariables.map_list[map_name.at(index)]->near_map.contains(generalData.serverPrivateVariables.map_list[map_name.at(index)]->border.left.map))
				generalData.serverPrivateVariables.map_list[map_name.at(index)]->near_map << generalData.serverPrivateVariables.map_list[map_name.at(index)]->border.left.map;
			if(generalData.serverPrivateVariables.map_list[map_name.at(index)]->border.right.map!=NULL && !generalData.serverPrivateVariables.map_list[map_name.at(index)]->near_map.contains(generalData.serverPrivateVariables.map_list[map_name.at(index)]->border.right.map))
				generalData.serverPrivateVariables.map_list[map_name.at(index)]->near_map << generalData.serverPrivateVariables.map_list[map_name.at(index)]->border.right.map;
		}

		if(generalData.serverPrivateVariables.map_list[map_name.at(index)]->border.top.map!=NULL && !generalData.serverPrivateVariables.map_list[map_name.at(index)]->near_map.contains(generalData.serverPrivateVariables.map_list[map_name.at(index)]->border.top.map))
		{
			generalData.serverPrivateVariables.map_list[map_name.at(index)]->near_map << generalData.serverPrivateVariables.map_list[map_name.at(index)]->border.top.map;
			if(generalData.serverPrivateVariables.map_list[map_name.at(index)]->border.left.map!=NULL &&  !generalData.serverPrivateVariables.map_list[map_name.at(index)]->near_map.contains(generalData.serverPrivateVariables.map_list[map_name.at(index)]->border.left.map))
				generalData.serverPrivateVariables.map_list[map_name.at(index)]->near_map << generalData.serverPrivateVariables.map_list[map_name.at(index)]->border.left.map;
			if(generalData.serverPrivateVariables.map_list[map_name.at(index)]->border.right.map!=NULL &&  !generalData.serverPrivateVariables.map_list[map_name.at(index)]->near_map.contains(generalData.serverPrivateVariables.map_list[map_name.at(index)]->border.right.map))
				generalData.serverPrivateVariables.map_list[map_name.at(index)]->near_map << generalData.serverPrivateVariables.map_list[map_name.at(index)]->border.right.map;
		}

		if(generalData.serverPrivateVariables.map_list[map_name.at(index)]->border.right.map!=NULL && !generalData.serverPrivateVariables.map_list[map_name.at(index)]->near_map.contains(generalData.serverPrivateVariables.map_list[map_name.at(index)]->border.right.map))
		{
			generalData.serverPrivateVariables.map_list[map_name.at(index)]->near_map << generalData.serverPrivateVariables.map_list[map_name.at(index)]->border.right.map;
			if(generalData.serverPrivateVariables.map_list[map_name.at(index)]->border.top.map!=NULL &&  !generalData.serverPrivateVariables.map_list[map_name.at(index)]->near_map.contains(generalData.serverPrivateVariables.map_list[map_name.at(index)]->border.top.map))
				generalData.serverPrivateVariables.map_list[map_name.at(index)]->near_map << generalData.serverPrivateVariables.map_list[map_name.at(index)]->border.top.map;
			if(generalData.serverPrivateVariables.map_list[map_name.at(index)]->border.bottom.map!=NULL &&  !generalData.serverPrivateVariables.map_list[map_name.at(index)]->near_map.contains(generalData.serverPrivateVariables.map_list[map_name.at(index)]->border.bottom.map))
				generalData.serverPrivateVariables.map_list[map_name.at(index)]->near_map << generalData.serverPrivateVariables.map_list[map_name.at(index)]->border.bottom.map;
		}

		if(generalData.serverPrivateVariables.map_list[map_name.at(index)]->border.left.map!=NULL && !generalData.serverPrivateVariables.map_list[map_name.at(index)]->near_map.contains(generalData.serverPrivateVariables.map_list[map_name.at(index)]->border.left.map))
		{
			generalData.serverPrivateVariables.map_list[map_name.at(index)]->near_map << generalData.serverPrivateVariables.map_list[map_name.at(index)]->border.left.map;
			if(generalData.serverPrivateVariables.map_list[map_name.at(index)]->border.top.map!=NULL &&  !generalData.serverPrivateVariables.map_list[map_name.at(index)]->near_map.contains(generalData.serverPrivateVariables.map_list[map_name.at(index)]->border.top.map))
				generalData.serverPrivateVariables.map_list[map_name.at(index)]->near_map << generalData.serverPrivateVariables.map_list[map_name.at(index)]->border.top.map;
			if(generalData.serverPrivateVariables.map_list[map_name.at(index)]->border.bottom.map!=NULL &&  !generalData.serverPrivateVariables.map_list[map_name.at(index)]->near_map.contains(generalData.serverPrivateVariables.map_list[map_name.at(index)]->border.bottom.map))
				generalData.serverPrivateVariables.map_list[map_name.at(index)]->near_map << generalData.serverPrivateVariables.map_list[map_name.at(index)]->border.bottom.map;
		}

		index++;
	}

	//resolv the offset to change of map
	size=semi_loaded_map.size();
	index=0;
	while(index<size)
	{
		if(generalData.serverPrivateVariables.map_list[map_name.at(index)]->border.bottom.map!=NULL)
			generalData.serverPrivateVariables.map_list[map_name.at(index)]->border.bottom.x_offset=semi_loaded_map.at(index).border.bottom.x_offset-semi_loaded_map.at(map_name.indexOf(semi_loaded_map.at(index).border.bottom.fileName)).border.top.x_offset;
		else
			generalData.serverPrivateVariables.map_list[map_name.at(index)]->border.bottom.x_offset=0;
		if(generalData.serverPrivateVariables.map_list[map_name.at(index)]->border.top.map!=NULL)
			generalData.serverPrivateVariables.map_list[map_name.at(index)]->border.top.x_offset=semi_loaded_map.at(index).border.top.x_offset-semi_loaded_map.at(map_name.indexOf(semi_loaded_map.at(index).border.top.fileName)).border.bottom.x_offset;
		else
			generalData.serverPrivateVariables.map_list[map_name.at(index)]->border.top.x_offset=0;
		if(generalData.serverPrivateVariables.map_list[map_name.at(index)]->border.left.map!=NULL)
			generalData.serverPrivateVariables.map_list[map_name.at(index)]->border.left.y_offset=semi_loaded_map.at(index).border.left.y_offset-semi_loaded_map.at(map_name.indexOf(semi_loaded_map.at(index).border.left.fileName)).border.right.y_offset;
		else
			generalData.serverPrivateVariables.map_list[map_name.at(index)]->border.left.y_offset=0;
		if(generalData.serverPrivateVariables.map_list[map_name.at(index)]->border.right.map!=NULL)
			generalData.serverPrivateVariables.map_list[map_name.at(index)]->border.right.y_offset=semi_loaded_map.at(index).border.right.y_offset-semi_loaded_map.at(map_name.indexOf(semi_loaded_map.at(index).border.right.fileName)).border.left.y_offset;
		else
			generalData.serverPrivateVariables.map_list[map_name.at(index)]->border.right.y_offset=0;
		index++;
	}

	DebugClass::debugConsole(QString("finish preload the map"));
}

void EventDispatcher::preload_the_visibility_algorithm()
{
	QHash<QString,Map_final *>::const_iterator i = generalData.serverPrivateVariables.map_list.constBegin();
	QHash<QString,Map_final *>::const_iterator i_end = generalData.serverPrivateVariables.map_list.constEnd();
	switch(generalData.serverSettings.mapVisibility.mapVisibilityAlgorithm)
	{
		case MapVisibilityAlgorithm_simple:
		while (i != i_end)
		{
			i.value()->mapVisibility.simple.show=true;
			i++;
		}
		break;
		default:
		break;
	}
}

void EventDispatcher::load_settings()
{
}

//start with allow real player to connect
void EventDispatcher::start_internal_server()
{
	if(server==NULL)
		server=new QTcpServer();
	if(server->isListening())
	{
		DebugClass::debugConsole(QString("Already listening on %1").arg(listenIpAndPort(server->serverAddress().toString(),server->serverPort())));
		emit error(QString("Already listening on %1").arg(listenIpAndPort(server->serverAddress().toString(),server->serverPort())));
		return;
	}
	if(stat!=Down)
	{
		DebugClass::debugConsole("In wrong stat");
		return;
	}
	stat=InUp;
	load_settings();
	QHostAddress address = QHostAddress::Any;
	if(!server_ip.isEmpty())
		address.setAddress(generalData.serverSettings.server_ip);
	if(!server->listen(address,generalData.serverSettings.server_port))
	{
		DebugClass::debugConsole(QString("Unable to listen: %1, errror: %2").arg(listenIpAndPort(server_ip,generalData.serverSettings.server_port)).arg(server->errorString()));
		stat=Down;
		emit error(QString("Unable to listen: %1, errror: %2").arg(listenIpAndPort(server_ip,generalData.serverSettings.server_port)).arg(server->errorString()));
		return;
	}
	if(server_ip.isEmpty())
		DebugClass::debugConsole(QString("Listen *:%1").arg(generalData.serverSettings.server_port));
	else
		DebugClass::debugConsole("Listen "+server_ip+":"+QString::number(generalData.serverSettings.server_port));
	DebugClass::debugConsole("Connecting to mysql...");

	if(!initialize_the_database())
		return;

	if(!generalData.serverPrivateVariables.db->open())
	{
		DebugClass::debugConsole(QString("Unable to connect to the database: %1, with the login: %2, database text: %3").arg(generalData.serverPrivateVariables.db->lastError().driverText()).arg(generalData.serverSettings.database.mysql.login).arg(generalData.serverPrivateVariables.db->lastError().databaseText()));
		server->close();
		stat=Down;
		emit error(QString("Unable to connect to the database: %1, with the login: %2, database text: %3").arg(generalData.serverPrivateVariables.db->lastError().driverText()).arg(generalData.serverSettings.database.mysql.login).arg(generalData.serverPrivateVariables.db->lastError().databaseText()));
		return;
	}
	DebugClass::debugConsole(QString("Connected to mysql at %1").arg(generalData.serverSettings.database.mysql.host));
	preload_the_data();
	stat=Up;
	connect(server,SIGNAL(newConnection()),this,SLOT(newConnection()));
	emit is_started(true);
	return;
}

bool EventDispatcher::initialize_the_database()
{
	switch(generalData.serverSettings.database.type)
	{
		default:
		case GeneralData::ServerSettings::Database::DatabaseType_Mysql:
		generalData.serverPrivateVariables.loginQuery.prepare("SELECT * FROM player WHERE login=:login AND password=:password");
		generalData.serverPrivateVariables.updateMapPositionQuery.prepare("UPDATE player SET map_name=:map_name,position_x=:position_x,position_y=:position_y,orientation=:orientation WHERE id=:id");
		if(generalData.serverPrivateVariables.db==NULL)
			generalData.serverPrivateVariables.db=new QSqlDatabase();
		*generalData.serverPrivateVariables.db = QSqlDatabase::addDatabase("QMYSQL");
		generalData.serverPrivateVariables.db->setConnectOptions("MYSQL_OPT_RECONNECT=1");
		generalData.serverPrivateVariables.db->setHostName(generalData.serverSettings.database.mysql.host);
		generalData.serverPrivateVariables.db->setDatabaseName(generalData.serverSettings.database.mysql.db);
		generalData.serverPrivateVariables.db->setUserName(generalData.serverSettings.database.mysql.login);
		generalData.serverPrivateVariables.db->setPassword(generalData.serverSettings.database.mysql.pass);
		return true;
		break;
		case GeneralData::ServerSettings::Database::DatabaseType_SQLite:
			return false;
		break;
	}
}

//start without real player possibility
void EventDispatcher::start_internal_benchmark(quint16 second,quint16 number_of_client)
{
	if(stat!=Down)
	{
		DebugClass::debugConsole("Is in wrong stat for benchmark");
		return;
	}
	timer_benchmark_stop->setInterval(second*1000);
	benchmark_latency=0;
	stat=InUp;
	load_settings();
	quint16 x=12,y=17;
	//firstly get the spawn point
	DebugClass::debugConsole(QString("benchmark spawn: x: %1, y: %2").arg(x).arg(y));
	generalData.serverPrivateVariables.mapBasePath=":/internal/benchmark-map/";
	preload_the_data();

	Map_final * benchmark_map=generalData.serverPrivateVariables.map_list["map.tmx"];
	int index=0;
	while(index<number_of_client)
	{
		addBot(x,y,benchmark_map);
		if(index==0)
		{
			//fake_clients.last()->show_details();
			time_benchmark_first_client.start();
		}
		index++;
	}
	timer_benchmark_stop->start();
	stat=Up;
}

////////////////////////////////////////////////// server stopping ////////////////////////////////////////////

void EventDispatcher::unload_the_data()
{
	unload_the_map();
	unload_the_visibility_algorithm();
}

void EventDispatcher::unload_the_map()
{
	QHash<QString,Map_final *>::const_iterator i = generalData.serverPrivateVariables.map_list.constBegin();
	QHash<QString,Map_final *>::const_iterator i_end = generalData.serverPrivateVariables.map_list.constEnd();
	while (i != i_end)
	{
		delete i.value()->parsed_layer.walkable;
		delete i.value()->parsed_layer.water;
		delete i.value();
		i++;
	}
	generalData.serverPrivateVariables.map_list.clear();
}

void EventDispatcher::unload_the_visibility_algorithm()
{
}

//call by stop benchmark
void EventDispatcher::stop_benchmark()
{
	if(in_benchmark_mode==false)
	{
		DebugClass::debugConsole("Double stop_benchmark detected!");
		return;
	}
	DebugClass::debugConsole("Stop the benchmark");
	generalData.serverPrivateVariables.mapBasePath=QCoreApplication::applicationDirPath()+"/datapack/map/tmx/";
	double TX_speed=0;
	double RX_speed=0;
	double TX_size=0;
	double RX_size=0;
	double second=0;
	if(fake_clients.size()>=1)
	{
		second=((double)time_benchmark_first_client.elapsed()/1000);
		if(second>0)
		{
			TX_size=fake_clients.first()->get_TX_size();
			RX_size=fake_clients.first()->get_RX_size();
			TX_speed=((double)TX_size)/second;
			RX_speed=((double)RX_size)/second;
		}
	}
	stat=Up;
	stop_internal_server();
	stat=Down;
	in_benchmark_mode=false;
	emit benchmark_result(benchmark_latency,TX_speed,RX_speed,TX_size,RX_size,second);
}

void EventDispatcher::check_if_now_stopped()
{
	if(client_list.size()!=0 || fake_clients.size()!=0)
		return;
	if(stat!=InDown)
		return;
	DebugClass::debugConsole("Fully stopped");
	if(generalData.serverPrivateVariables.db!=NULL)
		generalData.serverPrivateVariables.db->close();
	//server.close();
	stat=Down;
	if(server!=NULL)
	{
		server->close();
		delete server;
		server=NULL;
	}
	unload_the_data();
	emit is_started(false);
}

//call by normal stop
void EventDispatcher::stop_internal_server()
{
	if(stat!=Up && stat!=InDown)
	{
		if(stat!=Down)
			DebugClass::debugConsole("Is in wrong stat for stopping: "+QString::number((int)stat));
		return;
	}
	DebugClass::debugConsole("Try stop");
	stat=InDown;
	removeBots();
	if(server!=NULL)
	{
		disconnect(server,SIGNAL(newConnection()),this,SLOT(newConnection()));
		server->close();
	}
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

void EventDispatcher::removeOneClient()
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

void EventDispatcher::removeOneBot()
{
	FakeBot *client=qobject_cast<FakeBot *>(QObject::sender());
	if(client==NULL)
	{
		DebugClass::debugConsole("removeOneBot(): NULL client at disconnection");
		return;
	}
	fake_clients.removeOne(client);
	client->deleteLater();
	check_if_now_stopped();
}

void EventDispatcher::removeBots()
{
	int list_size=fake_clients.size();
	int index=0;
	while(index<list_size)
	{
		fake_clients.at(index)->stop();
		fake_clients.at(index)->disconnect();
		disconnect(&nextStep,SIGNAL(timeout()),fake_clients.last(),SLOT(doStep()));
		connect(fake_clients.last(),SIGNAL(disconnected()),this,SLOT(removeOneBot()),Qt::QueuedConnection);
		//delete after the signal
		index++;
	}
}

/////////////////////////////////////// Add object //////////////////////////////////////

void EventDispatcher::addBot(quint16 x,quint16 y,Map_final *map,QString skin)
{
	client_list << new Client(NULL);
	client_list.last()->fakeLogin(65535-fake_clients.size(),x,y,map,(Orientation)Direction_look_at_top,skin);
	fake_clients << new FakeBot(x,y,map,Direction_look_at_top);
	connect(&nextStep,SIGNAL(timeout()),fake_clients.last(),SLOT(doStep()),Qt::QueuedConnection);
	connect(client_list.last(),SIGNAL(fake_send_data(QByteArray)),fake_clients.last(),SLOT(fake_send_data(QByteArray)),Qt::QueuedConnection);
	connect(client_list.last(),SIGNAL(isReadyToDelete()),this,SLOT(removeOneClient()),Qt::QueuedConnection);
	//connect(client_list.last(),SIGNAL(player_is_disconnected(QString)),this,SLOT(removeOneClient()),Qt::QueuedConnection);
	connect(fake_clients.last(),SIGNAL(fake_receive_data(QByteArray)),client_list.last(),SLOT(fake_receive_data(QByteArray)));
	connect_the_last_client();
	fake_clients.last()->moveToThread(generalData.serverPrivateVariables.eventThreaderList.at(5));
	fake_clients.last()->start_step();
}

///////////////////////////////////// Generic command //////////////////////////////////

void EventDispatcher::serverCommand(QString command,QString extraText)
{
	Client *client=qobject_cast<Client *>(QObject::sender());
	if(client==NULL)
	{
		DebugClass::debugConsole("NULL client at serverCommand()");
		return;
	}
	if(command=="restart")
		emit need_be_restarted();
	else if(command=="stop")
		emit need_be_stopped();
	else if(command=="addbots" || command=="removebots")
	{
		if(stat!=Up)
		{
			DebugClass::debugConsole("Is in wrong stat for bots manipulation");
			return;
		}
		if(in_benchmark_mode)
		{
			DebugClass::debugConsole("Is in benchmark mode, unable to do bots manipulation");
			return;
		}
		if(command=="addbots")
		{
			if(!fake_clients.empty())
			{
				//client->local_sendPM(client->getPseudo(),"Remove previous bots firstly");
				DebugClass::debugConsole("Remove previous bots firstly");
				return;
			}
			Map_player_info map_player_info=client->getMapPlayerInfo();
			quint16 number_player=2;
			if(extraText!="")
			{
				bool ok;
				number_player=extraText.toUInt(&ok);
				if(!ok)
				{
					DebugClass::debugConsole("the arguement is not as number!");
					return;
				}
			}
			if(number_player>(generalData.serverSettings.max_players-client_list.size()))
				number_player=generalData.serverSettings.max_players-client_list.size();
			int index=0;
			while(index<number_player && client_list.size()<generalData.serverSettings.max_players)
			{
				addBot(map_player_info.x,map_player_info.y,map_player_info.map,map_player_info.skin);
				index++;
			}
		}
		else if(command=="removebots")
		{
			removeBots();
		}
		else
			DebugClass::debugConsole(QString("unknow command in bots case: %1").arg(command));
	}
	else
		DebugClass::debugConsole(QString("unknow command: %1").arg(command));
}

//////////////////////////////////// Function secondary //////////////////////////////
QString EventDispatcher::listenIpAndPort(QString server_ip,quint16 server_port)
{
	if(server_ip=="")
		server_ip="*";
	return server_ip+":"+QString::number(server_port);
}

QStringList EventDispatcher::listFolder(const QString& folder,const QString& suffix)
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

void EventDispatcher::newConnection()
{
	while(server->hasPendingConnections())
	{
		DebugClass::debugConsole(QString("new client connected"));
		QTcpSocket *socket = server->nextPendingConnection();
		if(socket!=NULL)
		{
			client_list << new Client(socket);
			connect_the_last_client();
		}
		else
			DebugClass::debugConsole("NULL client: "+socket->peerAddress().toString());
	}
}

void EventDispatcher::connect_the_last_client()
{
	connect(client_list.last(),SIGNAL(isReadyToDelete()),this,SLOT(removeOneClient()),Qt::QueuedConnection);
	if(!in_benchmark_mode)
	{
		connect(client_list.last(),SIGNAL(emit_serverCommand(QString,QString)),this,SLOT(serverCommand(QString,QString)),Qt::QueuedConnection);
		connect(client_list.last(),SIGNAL(new_player_is_connected(Player_private_and_public_informations)),this,SIGNAL(new_player_is_connected(Player_private_and_public_informations)),Qt::QueuedConnection);
		connect(client_list.last(),SIGNAL(player_is_disconnected(QString)),this,SIGNAL(player_is_disconnected(QString)),Qt::QueuedConnection);
		connect(client_list.last(),SIGNAL(new_chat_message(QString,Chat_type,QString)),this,SIGNAL(new_chat_message(QString,Chat_type,QString)),Qt::QueuedConnection);
	}
}

bool EventDispatcher::isListen()
{
	if(in_benchmark_mode)
		return false;
	return (stat==Up);
}

bool EventDispatcher::isStopped()
{
	if(in_benchmark_mode)
		return false;
	return (stat==Down);
}

bool EventDispatcher::isInBenchmark()
{
	return in_benchmark_mode;
}

quint16 EventDispatcher::player_current()
{
	return generalData.serverPrivateVariables.connected_players;
}

quint16 EventDispatcher::player_max()
{
	return generalData.serverSettings.max_players;
}

QStringList EventDispatcher::getLatency()
{
	return latencyChecker.getLatency();
}

quint32 EventDispatcher::getTotalLatency()
{
	return latencyChecker.getTotalLatency();
}

/////////////////////////////////// Async the call ///////////////////////////////////
/// \brief Called when event loop is setup
void EventDispatcher::start_server()
{
	emit need_be_started();
}

void EventDispatcher::stop_server()
{
	emit try_stop_server();
}

void EventDispatcher::start_benchmark(quint16 second,quint16 number_of_client)
{
	if(in_benchmark_mode)
	{
		DebugClass::debugConsole("Already in benchmark");
		return;
	}
	in_benchmark_mode=true;
	emit try_start_benchmark(second,number_of_client);
}
