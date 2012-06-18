#ifndef STRUCTURES_CLIENT_H
#define STRUCTURES_CLIENT_H

#include <QObject>
#include <QString>
#include <QPixmap>

/** conversion x,y to position: x+y*width */
struct Map_client
{
	//the index is position (x+y*width)
	struct Map_ParsedLayer
	{
		bool *walkable;
		bool *water;
	};
	Map_ParsedLayer parsed_layer;

	struct Map_Border
	{
		struct Map_BorderContent_TopBottom
		{
			Map_client *map;
			qint32 x_offset;
		};
		struct Map_BorderContent_LeftRight
		{
			Map_client *map;
			qint32 y_offset;
		};
		Map_BorderContent_TopBottom top;
		Map_BorderContent_TopBottom bottom;
		Map_BorderContent_LeftRight left;
		Map_BorderContent_LeftRight right;
	};
	Map_Border border;

	QList<Map_client *> near_map;//not only the border
	struct Teleporter
	{
		quint32 x,y;
		Map_client *map;
	};
	QHash<quint32,Teleporter> teleporter;//the int (x+y*width) is position

	QString map_file;
	quint16 width;
	quint16 height;
	quint32 group;

	QPixmap tile;
};

#endif // STRUCTURES_CLIENT_H
