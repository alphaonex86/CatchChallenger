#include "Map_loader.h"

#include <zlib.h>

static void logZlibError(int error)
{
    switch (error)
    {
	case Z_MEM_ERROR:
	    qDebug() << "Out of memory while (de)compressing data!";
	    break;
	case Z_VERSION_ERROR:
	    qDebug() << "Incompatible zlib version!";
	    break;
	case Z_NEED_DICT:
	case Z_DATA_ERROR:
	    qDebug() << "Incorrect zlib compressed data!";
	    break;
	default:
	    qDebug() << "Unknown error while (de)compressing data!";
    }
}


Map_loader::Map_loader()
{
}

Map_loader::~Map_loader()
{
	error="";
	map_to_send.parsed_layer.walkable=NULL;
	map_to_send.parsed_layer.water=NULL;
}

bool Map_loader::tryLoadMap(const QString &fileName)
{
	Map_to_send map_to_send;

	QFile mapFile(fileName);
	QByteArray xmlContent,Walkable,Collisions,Water;
	if(!mapFile.open(QIODevice::ReadOnly))
	{
		error=mapFile.fileName()+": "+mapFile.errorString();
		return false;
	}
	xmlContent=mapFile.readAll();
	bool ok;
	QDomDocument domDocument;
	QString errorStr;
	int errorLine,errorColumn;
	if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
	{
		error=QString("%1, Parse error at line %2, column %3: %4").arg("informations.xml").arg(errorLine).arg(errorColumn).arg(errorStr);
		return false;
	}
	QDomElement root = domDocument.documentElement();
	if(root.tagName()!="map")
	{
		error=QString("\"map\" root balise not found for the xml file");
		return false;
	}

	//get the width
	if(!root.hasAttribute("width"))
	{
		error=QString("the root node has not the attribute \"width\"");
		return false;
	}
	map_to_send.width=root.attribute("width").toUInt(&ok);
	if(!ok)
	{
		error=QString("the root node has wrong attribute \"width\"");
		return false;
	}

	//get the height
	if(!root.hasAttribute("height"))
	{
		error=QString("the root node has not the attribute \"height\"");
		return false;
	}
	map_to_send.height=root.attribute("height").toUInt(&ok);
	if(!ok)
	{
		error=QString("the root node has wrong attribute \"height\"");
		return false;
	}

	//check the size
	if(map_to_send.width<1 || map_to_send.width>255)
	{
		error=QString("the width should be greater or equal than 1, and lower or equal than 32000");
		return false;
	}
	if(map_to_send.height<1 || map_to_send.height>255)
	{
		error=QString("the height should be greater or equal than 1, and lower or equal than 32000");
		return false;
	}

	//properties
	QDomElement child = root.firstChildElement("properties");
	if(!child.isNull())
	{
		if(child.isElement())
		{
			QDomElement SubChild=child.firstChildElement("property");
			while(!SubChild.isNull())
			{
				if(SubChild.isElement())
				{
					if(SubChild.hasAttribute("name") && SubChild.hasAttribute("value"))
						map_to_send.property[SubChild.attribute("name")]=SubChild.attribute("value");
					else
					{
						error=QString("Missing attribute name or value: child.tagName(): %1").arg(child.tagName());
						return false;
					}
				}
				else
				{
					error=QString("Is not Element: child.tagName(): %1").arg(SubChild.tagName());
					return false;
				}
				SubChild = SubChild.nextSiblingElement("property");
			}
		}
		else
		{
			error=QString("Is Element: child.tagName(): %1").arg(child.tagName());
			return false;
		}
	}

	// objectgroup
	child = root.firstChildElement("objectgroup");
	while(!child.isNull())
	{
		if(!child.hasAttribute("name"))
			DebugClass::debugConsole(QString("Has not attribute \"name\": child.tagName(): %1").arg(child.tagName()));
		else if(!child.isElement())
			DebugClass::debugConsole(QString("Is Element: child.tagName(): %1, name: %2").arg(child.tagName().arg(child.attribute("name"))));
		else
		{
			if(child.attribute("name")=="Tp")
			{
				QDomElement SubChild=child.firstChildElement("object");
				while(!SubChild.isNull())
				{
					if(SubChild.isElement() && SubChild.hasAttribute("x") && SubChild.hasAttribute("x"))
					{
						quint32 object_x=SubChild.attribute("x").toUInt(&ok)/16;
						if(!ok)
							DebugClass::debugConsole(QString("Missing coord: %1").arg(child.tagName()));
						else
						{
							quint32 object_y=SubChild.attribute("y").toUInt(&ok)/16;
							if(!ok)
								DebugClass::debugConsole(QString("Missing coord: %1").arg(child.tagName()));
							else if(object_x>map_to_send.width || object_y>map_to_send.height)
								DebugClass::debugConsole(QString("Object out of the map: %1").arg(child.tagName()));
							else if(SubChild.hasAttribute("type"))
							{
								QString type=SubChild.attribute("type");
								#ifdef DEBUG_MESSAGE_CLIENT_MAP
								DebugClass::debugConsole(QString("type: %1, object_x: %2, object_y: %3")
											 .arg(type)
											 .arg(object_x)
											 .arg(object_y)
											 );
								#endif

								QHash<QString,QVariant> property_text;
								QDomElement prop=SubChild.firstChildElement("properties");
								if(!prop.isNull())
								{

									QDomElement property=prop.firstChildElement("property");
									while(!property.isNull())
									{
										if(property.hasAttribute("name") && property.hasAttribute("value"))
											property_text[property.attribute("name")]=property.attribute("value");
										property = property.nextSiblingElement("property");
									}
								}
								if(type=="border-left" || type=="border-right" || type=="border-top" || type=="border-bottom")
								{
									if(property_text.contains("map"))
									{
										if(type=="border-left")//border left
										{
											map_to_send.border.left.fileName=property_text["map"].toString();
											map_to_send.border.left.y_offset=object_y;
											#ifdef DEBUG_MESSAGE_CLIENT_MAP_BORDER
											DebugClass::debugConsole(QString("map_to_send.border.left.fileName: %1, offset: %2").arg(map_to_send.border.left.fileName).arg(map_to_send.border.left.y_offset));
											#endif
										}
										else if(type=="border-right")//border right
										{
											map_to_send.border.right.fileName=property_text["map"].toString();
											map_to_send.border.right.y_offset=object_y;
											#ifdef DEBUG_MESSAGE_CLIENT_MAP_BORDER
											DebugClass::debugConsole(QString("map_to_send.border.right.fileName: %1, offset: %2").arg(map_to_send.border.right.fileName).arg(map_to_send.border.right.y_offset));
											#endif
										}
										else if(type=="border-top")//border top
										{
											map_to_send.border.top.fileName=property_text["map"].toString();
											map_to_send.border.top.x_offset=object_x;
											#ifdef DEBUG_MESSAGE_CLIENT_MAP_BORDER
											DebugClass::debugConsole(QString("map_to_send.border.top.fileName: %1, offset: %2").arg(map_to_send.border.top.fileName).arg(map_to_send.border.top.x_offset));
											#endif
										}
										else if(type=="border-bottom")//border bottom
										{
											map_to_send.border.bottom.fileName=property_text["map"].toString();
											map_to_send.border.bottom.x_offset=object_x;
											#ifdef DEBUG_MESSAGE_CLIENT_MAP_BORDER
											DebugClass::debugConsole(QString("map_to_send.border.bottom.fileName: %1, offset: %2").arg(map_to_send.border.bottom.fileName).arg(map_to_send.border.bottom.x_offset));
											#endif
										}
										else
											DebugClass::debugConsole(QString("Not at middle of border: child.tagName(): %1, object_x: %2, object_y: %3")
												 .arg(child.tagName())
												 .arg(object_x)
												 .arg(object_y)
												 );
									}
									else
										DebugClass::debugConsole(QString("Missing border properties: child.tagName(): %1").arg(child.tagName()));
								}
								else if(type=="teleport on push" || type=="teleport on it" || type=="door")
								{
									if(property_text.contains("map") && property_text.contains("x") && property_text.contains("y"))
									{
										Map_to_send::Temp_teleport new_tp;
										new_tp.destination_x = property_text["x"].toUInt(&ok);
										if(ok)
										{
											new_tp.destination_y = property_text["y"].toUInt(&ok);
											if(ok)
											{
												new_tp.source_x=object_x;
												new_tp.source_y=object_y;
												new_tp.map=property_text["map"].toString();
												map_to_send.teleport << new_tp;
												#ifdef DEBUG_MESSAGE_CLIENT_MAP_OBJECT
												DebugClass::debugConsole(QString("Teleporter type: %1, to: %2 (%3,%4)").arg(type).arg(new_tp.map).arg(new_tp.source_x).arg(new_tp.source_y));
												#endif
											}
											else
												DebugClass::debugConsole(QString("Bad convertion to int for y, type: %1").arg(type));
										}
										else
											DebugClass::debugConsole(QString("Bad convertion to int for x, type: %1").arg(type));
									}
									else
										DebugClass::debugConsole(QString("Missing map,x or y, type: %1").arg(type));
								}
								else if(type=="rescue")
								{
									Map_to_send::Rescue_Point tempPoint;
									tempPoint.x=object_x;
									tempPoint.y=object_y;
									map_to_send.rescue_points << tempPoint;
								}
								else if(type=="bot spawn")
								{
									Map_to_send::Bot_Spawn_Point tempPoint;
									tempPoint.x=object_x;
									tempPoint.y=object_y;
									map_to_send.bot_spawn_points << tempPoint;
								}

							}
							else
								DebugClass::debugConsole(QString("Missing attribute type missing: child.tagName(): %1").arg(SubChild.tagName()));
						}
					}
					else
						error=QString("Is not Element: child.tagName(): %1").arg(child.tagName());
					SubChild = SubChild.nextSiblingElement("object");
				}
			}
		}
		child = child.nextSiblingElement("objectgroup");
	}



	// layer
	child = root.firstChildElement("layer");
	while(!child.isNull())
	{
		if(!child.isElement())
		{
			error=QString("Is Element: child.tagName(): %1").arg(child.tagName());
			return false;
		}
		else if(!child.hasAttribute("name"))
		{
			error=QString("Has not attribute \"name\": child.tagName(): %1").arg(child.tagName());
			return false;
		}
		else
		{
			QDomElement data = child.firstChildElement("data");
			QString name=child.attribute("name");
			if(data.isNull())
			{
				error=QString("Is Element for layer is null: %1 and name: %2").arg(data.tagName()).arg(name);
				return false;
			}
			else if(!data.isElement())
			{
				error=QString("Is Element for layer child.tagName(): %1").arg(data.tagName());
				return false;
			}
			else if(!data.hasAttribute("encoding"))
			{
				error=QString("Has not attribute \"base64\": child.tagName(): %1").arg(data.tagName());
				return false;
			}
			else if(!data.hasAttribute("compression"))
			{
				error=QString("Has not attribute \"zlib\": child.tagName(): %1").arg(data.tagName());
				return false;
			}
			else if(data.attribute("encoding")!="base64")
			{
				error=QString("only encoding base64 is supported");
				return false;
			}
			else if(!data.hasAttribute("compression"))
			{
				error=QString("Only compression zlib is supported");
				return false;
			}
			else
			{
				QString text=data.text();
				#if QT_VERSION < 0x040800
				    const QString textData = QString::fromRawData(text.unicode(), text.size());
				    const QByteArray latin1Text = textData.toLatin1();
				#else
				    const QByteArray latin1Text = text.toLatin1();
				#endif
				QByteArray data=decompress(QByteArray::fromBase64(latin1Text),map_to_send.height*map_to_send.width*4);
				if((quint32)data.size()!=map_to_send.height*map_to_send.width*4)
				{
					error=QString("map binary size (%1) != %2x%3x4").arg(data.size()).arg(map_to_send.height).arg(map_to_send.width);
					return false;
				}
				if(name=="Walkable")
					Walkable=data;
				else if(name=="Collisions")
					Collisions=data;
				else if(name=="Water")
					Water=data;
			}
		}
		child = child.nextSiblingElement("layer");
	}

	mapFile.close();

	QByteArray null_data;
	null_data.resize(4);
	null_data[0]=0x00;
	null_data[1]=0x00;
	null_data[2]=0x00;
	null_data[3]=0x00;

	map_to_send.parsed_layer.walkable	= new bool[map_to_send.width*map_to_send.height];
	map_to_send.parsed_layer.water		= new bool[map_to_send.width*map_to_send.height];

	quint32 x=0;
	quint32 y=0;
	bool walkable=false,water=false,collisions=false;
	while(x<map_to_send.width)
	{
		y=0;
		while(y<map_to_send.height)
		{
			if(x*4+y*map_to_send.width*4<(quint32)Walkable.size())
				walkable=Walkable.mid(x*4+y*map_to_send.width*4,4)!=null_data;
			else//if layer not found
				walkable=false;

			if(x*4+y*map_to_send.width*4<(quint32)Water.size())
				water=Water.mid(x*4+y*map_to_send.width*4,4)!=null_data;
			else//if layer not found
				water=false;

			if(x*4+y*map_to_send.width*4<(quint32)Collisions.size())
				collisions=Collisions.mid(x*4+y*map_to_send.width*4,4)!=null_data;
			else//if layer not found
				collisions=false;

			map_to_send.parsed_layer.walkable[x+y*map_to_send.width]=walkable && !water && !collisions;
			map_to_send.parsed_layer.water[x+y*map_to_send.width]=water;
			y++;
		}
		x++;
	}

#ifdef DEBUG_MESSAGE_CLIENT_RAW_MAP
	if(Walkable.size()>0 || Water.size()>0 || Collisions.size()>0)
	{
		x=0;
		y=0;
		QStringList layers_name;
		if(Walkable.size()>0)
			layers_name << "Walkable";
		if(Water.size()>0)
			layers_name << "Water";
		if(Collisions.size()>0)
			layers_name << "Collisions";
		DebugClass::debugConsole(layers_name.join(" + ")+" = total");
		while(y<map_to_send.height)
		{
			QString line;
			if(Walkable.size()>0)
			{
				x=0;
				while(x<map_to_send.width)
				{
					line += QString::number(Walkable.mid(x*4+y*map_to_send.width*4,4)!=null_data);
					x++;
				}
				line+=" ";
			}
			if(Water.size()>0)
			{
				x=0;
				while(x<map_to_send.width)
				{
					line += QString::number(Water.mid(x*4+y*map_to_send.width*4,4)!=null_data);
					x++;
				}
				line+=" ";
			}
			if(Collisions.size()>0)
			{
				x=0;
				while(x<map_to_send.width)
				{
					line += QString::number(Collisions.mid(x*4+y*map_to_send.width*4,4)!=null_data);
					x++;
				}
				line+=" ";
			}
			x=0;
			while(x<map_to_send.width)
			{
				line += QString::number(map_to_send.parsed_layer.walkable[x+y*map_to_send.width]);
				x++;
			}
			line.replace("0","_");
			DebugClass::debugConsole(line);
			y++;
		}
	}
	else
		DebugClass::debugConsole("No layer found!");
#endif

	/*QTime toto;
	toto.start();
	while(toto.elapsed()<5000)
	{}*/

	this->map_to_send=map_to_send;
	return true;
}

QString Map_loader::errorString()
{
	return error;
}

QByteArray Map_loader::decompress(const QByteArray &data, int expectedSize)
{
    QByteArray out;
    out.resize(expectedSize);
    z_stream strm;

    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.next_in = (Bytef *) data.data();
    strm.avail_in = data.length();
    strm.next_out = (Bytef *) out.data();
    strm.avail_out = out.size();

    int ret = inflateInit2(&strm, 15 + 32);

    if (ret != Z_OK) {
	logZlibError(ret);
	return QByteArray();
    }

    do {
	ret = inflate(&strm, Z_SYNC_FLUSH);

	switch (ret) {
	    case Z_NEED_DICT:
	    case Z_STREAM_ERROR:
		ret = Z_DATA_ERROR;
	    case Z_DATA_ERROR:
	    case Z_MEM_ERROR:
		inflateEnd(&strm);
		logZlibError(ret);
		return QByteArray();
	}

	if (ret != Z_STREAM_END) {
	    int oldSize = out.size();
	    out.resize(out.size() * 2);

	    strm.next_out = (Bytef *)(out.data() + oldSize);
	    strm.avail_out = oldSize;
	}
    }
    while (ret != Z_STREAM_END);

    if (strm.avail_in != 0) {
	logZlibError(Z_DATA_ERROR);
	return QByteArray();
    }

    const int outLength = out.size() - strm.avail_out;
    inflateEnd(&strm);

    out.resize(outLength);
    return out;
}

