#include "Map_custom.h"

Map_custom::Map_custom(EventThreader* map_loader_thread,QList<Map_custom *> *map_list)
{
	width=0;
	height=0;
	map_border_left.linked_map_object=NULL;
	map_border_right.linked_map_object=NULL;
	map_border_top.linked_map_object=NULL;
	map_border_bottom.linked_map_object=NULL;
	map_loaded=false;
	this->map_loader_thread=map_loader_thread;
	this->map_list=map_list;
	bool_Walkable=NULL;
	bool_Water=NULL;
}

Map_custom::~Map_custom()
{
	int index=0;
	int linked_map_object_size=linked_map_object.size();
	while(index<linked_map_object_size)
	{
		//unregister on the other map
		if(linked_map_object.at(index)->map_loaded)
		{
			if(linked_map_object.at(index)->map_not_loaded.contains(this->fileName))
				linked_map_object.at(index)->linked_map_object.removeOne(this);
			if(linked_map_object.at(index)->map_border_left.fileName==this->fileName)
				linked_map_object.at(index)->map_border_left.linked_map_object=NULL;
			if(linked_map_object.at(index)->map_border_right.fileName==this->fileName)
				linked_map_object.at(index)->map_border_right.linked_map_object=NULL;
			if(linked_map_object.at(index)->map_border_top.fileName==this->fileName)
				linked_map_object.at(index)->map_border_top.linked_map_object=NULL;
			if(linked_map_object.at(index)->map_border_bottom.fileName==this->fileName)
				linked_map_object.at(index)->map_border_bottom.linked_map_object=NULL;
		}
		index++;
	}
	if(bool_Walkable!=NULL)
	{
		delete bool_Walkable;
		bool_Walkable=NULL;
	}
	if(bool_Water!=NULL)
	{
		delete bool_Water;
		bool_Water=NULL;
	}
	if(map_loader!=NULL)
	{
		delete map_loader;
		map_loader=NULL;
	}
}

void Map_custom::loadMap(const QString &fileName,const QString &basePath)
{
	this->fileName=fileName;
	map_loader=new Map_loader();
	map_loader->moveToThread(map_loader_thread);
	connect(map_loader,	SIGNAL(error(QString)),		this,		SLOT(error(QString)),Qt::QueuedConnection);
	connect(map_loader,	SIGNAL(map_content_loaded()),	this,		SLOT(map_content_loaded()),Qt::QueuedConnection);
	connect(this,		SIGNAL(tryLoadMap(QString)),	map_loader,	SLOT(tryLoadMap(QString)),Qt::QueuedConnection);
	emit tryLoadMap(basePath+fileName);
}

bool Map_custom::is_walkalble(const quint16 &x,const quint16 &y)
{
	#ifdef POKECRAFT_SERVER_EXTRA_CHECK
	if((width-1)<x || (height-1)<y)
	{
		emit error("out of the map, for ask if is walkable");
		return false;
	}
	#endif
	return bool_Walkable[x+y*width];
}

void Map_custom::map_content_loaded()
{
	if(map_loader==NULL)
		return;
	disconnect(map_loader,0,0,0);
	map_final=map_loader->map_to_send;
	width=map_final.width;
	height=map_final.height;
	x_spawn=map_final.x_spawn;
	y_spawn=map_final.y_spawn;
	property_name=map_final.property_name;
	property_value=map_final.property_value;
	bool_Walkable=map_final.bool_Walkable;
	bool_Water=map_final.bool_Water;
	//parse the border
	if(map_final.border_bottom.fileName!="")
	{
		map_border_bottom.fileName=map_final.border_bottom.fileName;
		map_border_bottom.x=map_final.border_bottom.x_offset;
	}
	if(map_final.border_top.fileName!="")
	{
		map_border_top.fileName=map_final.border_top.fileName;
		map_border_top.x=map_final.border_top.x_offset;
	}
	if(map_final.border_right.fileName!="")
	{
		map_border_right.fileName=map_final.border_right.fileName;
		map_border_right.y=map_final.border_right.y_offset;
	}
	if(map_final.border_left.fileName!="")
	{
		map_border_left.fileName=map_final.border_left.fileName;
		map_border_left.y=map_final.border_left.y_offset;
	}
	//parse the other map
	int index=0;
	int content_other_map_size=map_final.other_map.size();
	while(index<content_other_map_size)
	{
		map_not_loaded << map_final.other_map.at(index);
		index++;
	}
	//load the map pointer
	index=0;
	int map_not_loaded_size=map_not_loaded.size();
	while(index<map_not_loaded_size)
	{
		//search into list
		searchOtherMap(map_not_loaded.at(index));
		index++;
	}
	linked_map_object << this;
	map_loaded=true;
	//load the delayed player
	int index_local_player=0;
	//ClientMapManagement *current_client=NULL;
	int clients_size=clients.size();
	index_local_player=0;
	while(index_local_player<clients_size)
	{
		clients.at(index_local_player)->propagate();
		index_local_player++;
	}

	delete map_loader;
	map_loader=NULL;
}

void Map_custom::check_client_position(const int &index)
{
	if(clients.at(index)->x>width || clients.at(index)->y>height)
	{
		clients.at(index)->x=x_spawn;
		clients.at(index)->y=y_spawn;
		clients.at(index)->at_start_x=x_spawn;
		clients.at(index)->at_start_y=y_spawn;
	}
}

void Map_custom::error(const QString &errorString)
{
	int index=0;
	int clients_size=clients.size();
	while(index<clients_size)
	{
		clients.at(index)->mapError(errorString);
		index++;
	}
	if(map_loader==NULL)
		return;
	disconnect(map_loader,0,0,0);
	DebugClass::debugConsole("Unable to load the map: "+errorString);
	/// \todo disconnect the client
	delete map_loader;
	map_loader=NULL;
}

QString Map_custom::mapName()
{
	return fileName;
}

Map_custom * Map_custom::searchOtherMap(const QString &fileName)
{
	int index=0;
	int map_list_size=map_list->size();
	while(index<map_list_size)
	{
		if(map_list->at(index)->mapName()==fileName)
		{
			//register on the other map
			if(map_list->at(index)->map_loaded)
			{
				if(map_list->at(index)->map_not_loaded.contains(this->fileName))
					map_list->at(index)->linked_map_object << this;
				if(map_list->at(index)->map_border_left.fileName==this->fileName)
					map_list->at(index)->map_border_left.linked_map_object=this;
				if(map_list->at(index)->map_border_right.fileName==this->fileName)
					map_list->at(index)->map_border_right.linked_map_object=this;
				if(map_list->at(index)->map_border_top.fileName==this->fileName)
					map_list->at(index)->map_border_top.linked_map_object=this;
				if(map_list->at(index)->map_border_bottom.fileName==this->fileName)
					map_list->at(index)->map_border_bottom.linked_map_object=this;
			}
			//register on the current map
			linked_map_object << map_list->at(index);
			if(map_border_left.fileName==fileName)
				map_border_left.linked_map_object=map_list->at(index);
			if(map_border_right.fileName==fileName)
				map_border_right.linked_map_object=map_list->at(index);
			if(map_border_top.fileName==fileName)
				map_border_top.linked_map_object=map_list->at(index);
			if(map_border_bottom.fileName==fileName)
				map_border_bottom.linked_map_object=map_list->at(index);
			return map_list->at(index);
		}
		index++;
	}
	return NULL;
}

QString Map_custom::directionToString(const Direction &direction)
{
	switch(direction)
	{
		case Direction_look_at_top:
			return "look at top";
		break;
		case Direction_look_at_right:
			return "look at right";
		break;
		case Direction_look_at_bottom:
			return "look at bottom";
		break;
		case Direction_look_at_left:
			return "look at left";
		break;
		case Direction_move_at_top:
			return "move at top";
		break;
		case Direction_move_at_right:
			return "move at right";
		break;
		case Direction_move_at_bottom:
			return "move at bottom";
		break;
		case Direction_move_at_left:
			return "move at left";
		break;
		default:
		break;
	}
	return "???";
}
