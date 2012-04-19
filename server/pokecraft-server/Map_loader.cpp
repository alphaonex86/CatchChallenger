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
	{QMutexLocker lock(&mutex);}
}

void Map_loader::tryLoadMap(QString fileName)
{
	QMutexLocker lock(&mutex);
	QFile mapFile(fileName);
	QByteArray xmlContent,Walkable,Collisions,Water;
	if(!mapFile.open(QIODevice::ReadOnly))
	{
		emit error(mapFile.fileName()+": "+mapFile.errorString());
		return;
	}
	xmlContent=mapFile.readAll();
	bool ok;
	QDomDocument domDocument;
	QString errorStr;
	int errorLine,errorColumn;
	if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
	{
		emit error(QString("%1, Parse error at line %2, column %3: %4").arg("informations.xml").arg(errorLine).arg(errorColumn).arg(errorStr));
		return;
	}
	QDomElement root = domDocument.documentElement();
	if(root.tagName()!="map")
	{
		emit error(QString("\"map\" root balise not found for the xml file"));
		return;
	}
	if(!root.hasAttribute("width"))
	{
		emit error(QString("the root node has not the attribute \"width\""));
		return;
	}
	map_to_send.width=root.attribute("width").toInt(&ok);
	if(!root.hasAttribute("width"))
	{
		emit error(QString("the root node has wrong attribute \"width\""));
		return;
	}
	if(!root.hasAttribute("height"))
	{
		emit error(QString("the root node has not the attribute \"width\""));
		return;
	}
	map_to_send.height=root.attribute("height").toInt(&ok);
	if(!root.hasAttribute("height"))
	{
		emit error(QString("the root node has wrong attribute \"height\""));
		return;
	}
	DebugClass::debugConsole(QString("[%1] map_to_send.width: %2, map_to_send.height: %3").arg(fileName).arg(map_to_send.width).arg(map_to_send.height));
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
					{
						map_to_send.property_name<<SubChild.attribute("name");
						map_to_send.property_value<<SubChild.attribute("value");
					}
					else
					{
						emit error(QString("Missing attribute name or value: child.tagName(): %1").arg(child.tagName()));
						return;
					}
				}
				else
				{
					emit error(QString("Is not Element: child.tagName(): %1").arg(SubChild.tagName()));
					return;
				}
				SubChild = SubChild.nextSiblingElement("property");
			}
		}
		else
		{
			emit error(QString("Is Element: child.tagName(): %1").arg(child.tagName()));
			return;
		}
	}

	bool have_spawn=false;
	// objectgroup
	child = root.firstChildElement("objectgroup");
	while(!child.isNull())
	{
		if(!child.isElement())
		{
			emit error(QString("Is Element: child.tagName(): %1").arg(child.tagName()));
			return;
		}
		else if(!child.hasAttribute("name"))
		{
			emit error(QString("Has not attribute \"name\": child.tagName(): %1").arg(child.tagName()));
			return;
		}
		else
		{
			if(child.attribute("name")=="Tp")
			{
				QDomElement SubChild=child.firstChildElement("object");
				while(!SubChild.isNull())
				{
					if(SubChild.isElement() && SubChild.hasAttribute("x") && SubChild.hasAttribute("x"))
					{
						int object_x=SubChild.attribute("x").toInt(&ok)/16;
						if(!ok)
						{
							emit error(QString("Missing coord: %1").arg(child.tagName()));
							return;
						}
						int object_y=SubChild.attribute("y").toInt(&ok)/16;
						if(!ok)
						{
							emit error(QString("Missing coord: %1").arg(child.tagName()));
							return;
						}
						if(SubChild.hasAttribute("type"))
						{
							QString type=SubChild.attribute("type");
							DebugClass::debugConsole(QString("type: %1, object_x: %2, object_y: %3")
										 .arg(type)
										 .arg(object_x)
										 .arg(object_y)
										 );
							if(type=="border-left" || type=="border-right" || type=="border-top" || type=="border-bottom")
							{
								QDomElement prop=SubChild.firstChildElement("properties");
								if(!prop.isNull())
								{
									QStringList name;
									QList<QVariant> value;
									QDomElement property=prop.firstChildElement("property");
									while(!property.isNull())
									{
										if(property.hasAttribute("name") && property.hasAttribute("value"))
										{
											name << property.attribute("name");
											value << property.attribute("value");
										}
										property = property.nextSiblingElement("property");
									}
									int index_map=name.indexOf("map");
									if(index_map!=-1)
									{
										if(type=="border-left")//border left
										{
											//quint16 y_value=value.at(index_y).toUInt(&ok);
											if(!ok)
												DebugClass::debugConsole(QString("[%1] Not a number: child.tagName(): %2").arg(fileName).arg(child.tagName()));
											else
											{
												map_to_send.border_left.fileName=value.at(index_map).toString();
												map_to_send.border_left.y_offset=object_y;
												map_to_send.border_left.x_offset=0;
												DebugClass::debugConsole(QString("[%1] map_to_send.border_left.fileName: %2, offset: %3").arg(fileName).arg(map_to_send.border_left.fileName).arg(map_to_send.border_left.y_offset));
											}
										}
										else if(type=="border-right")//border right
										{
											//quint16 y_value=value.at(index_y).toUInt(&ok);
											if(!ok)
												DebugClass::debugConsole(QString("[%1] Not a number: child.tagName(): %2").arg(fileName).arg(child.tagName()));
											else
											{
												map_to_send.border_right.fileName=value.at(index_map).toString();
												map_to_send.border_right.y_offset=object_y;
												map_to_send.border_right.x_offset=0;
												DebugClass::debugConsole(QString("[%1] map_to_send.border_right.fileName: %2, offset: %3").arg(fileName).arg(map_to_send.border_right.fileName).arg(map_to_send.border_right.y_offset));
											}
										}
										else if(type=="border-top")//border top
										{

											//quint16 x_value=value.at(index_x).toUInt(&ok);
											if(!ok)
												DebugClass::debugConsole(QString("[%1] Not a number: child.tagName(): %2").arg(fileName).arg(child.tagName()));
											else
											{
												map_to_send.border_top.fileName=value.at(index_map).toString();
												map_to_send.border_top.x_offset=object_x;
												map_to_send.border_top.y_offset=0;
												DebugClass::debugConsole(QString("[%1] map_to_send.border_top.fileName: %2, offset: %3").arg(fileName).arg(map_to_send.border_top.fileName).arg(map_to_send.border_top.x_offset));
											}
										}
										else if(type=="border-bottom")//border bottom
										{

											//quint16 x_value=value.at(index_x).toUInt(&ok);
											if(!ok)
												DebugClass::debugConsole(QString("[%1] Not a number: child.tagName(): %2").arg(fileName).arg(child.tagName()));
											else
											{
												map_to_send.border_bottom.fileName=value.at(index_map).toString();
												map_to_send.border_bottom.x_offset=object_x;
												map_to_send.border_bottom.y_offset=0;
												DebugClass::debugConsole(QString("[%1] map_to_send.border_bottom.fileName: %2, offset: %3").arg(fileName).arg(map_to_send.border_bottom.fileName).arg(map_to_send.border_bottom.x_offset));
											}
										}
										else
											DebugClass::debugConsole(QString("[%1] Not at middle of border: child.tagName(): %2, object_x: %3, object_y: %4")
														 .arg(fileName)
														 .arg(child.tagName())
														 .arg(object_x)
														 .arg(object_y)
														 );
									}
									else
										DebugClass::debugConsole(QString("[%1] Missing border properties: child.tagName(): %2").arg(fileName).arg(child.tagName()));
								}
								else
									DebugClass::debugConsole(QString("[%1] Missing balise properties: child.tagName(): %2").arg(fileName).arg(child.tagName()));
							}
							else if(SubChild.attribute("type")=="spawn" && SubChild.hasAttribute("x") && SubChild.hasAttribute("y"))
							{
								have_spawn=true;
								bool ok;
								map_to_send.x_spawn=SubChild.attribute("x").toUInt(&ok)/16;
								if(!ok)
								{
									emit error(QString("Is not uint at x_spawn"));
									return;
								}
								map_to_send.y_spawn=SubChild.attribute("y").toUInt(&ok)/16;
								if(!ok)
								{
									emit error(QString("Is not uint at y_spawn"));
									return;
								}
							}
						}
						else
							DebugClass::debugConsole(QString("[%1] Missing attribute type missing: child.tagName(): %2").arg(fileName).arg(child.tagName()));
					}
					else
					{
						emit error(QString("Is not Element: child.tagName(): %1").arg(child.tagName()));
						return;
					}
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
			emit error(QString("Is Element: child.tagName(): %1").arg(child.tagName()));
			return;
		}
		else if(!child.hasAttribute("name"))
		{
			emit error(QString("Has not attribute \"name\": child.tagName(): %1").arg(child.tagName()));
			return;
		}
		else
		{
			QDomElement data = child.firstChildElement("data");
			QString name=child.attribute("name");
			if(data.isNull())
			{
				emit error(QString("Is Element for layer is null: %1 and name: %2").arg(data.tagName()).arg(name));
				return;
			}
			else if(!data.isElement())
			{
				emit error(QString("Is Element for layer child.tagName(): %1").arg(data.tagName()));
				return;
			}
			else if(!data.hasAttribute("encoding"))
			{
				emit error(QString("Has not attribute \"base64\": child.tagName(): %1").arg(data.tagName()));
				return;
			}
			else if(!data.hasAttribute("compression"))
			{
				emit error(QString("Has not attribute \"zlib\": child.tagName(): %1").arg(data.tagName()));
				return;
			}
			else if(data.attribute("encoding")!="base64")
			{
				emit error(QString("only encoding base64 is supported"));
				return;
			}
			else if(!data.hasAttribute("compression"))
			{
				emit error(QString("Only compression zlib is supported"));
				return;
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
				if(data.size()!=map_to_send.height*map_to_send.width*4)
				{
					emit error(QString("map binary size (%1) != %2x%3x4").arg(data.size()).arg(map_to_send.height).arg(map_to_send.width));
					return;
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

	map_to_send.y_spawn-=1;
	mapFile.close();
	if(!have_spawn)
	{
		emit error(QString("Have not spawn point"));
		return;
	}

	QByteArray null_data;
	null_data.resize(4);
	null_data[0]=0x00;
	null_data[1]=0x00;
	null_data[2]=0x00;
	null_data[3]=0x00;

	map_to_send.bool_Walkable	= new bool[map_to_send.width*map_to_send.height];
	map_to_send.bool_Water		= new bool[map_to_send.width*map_to_send.height];

	int x=0;
	int y=0;
	bool walkable=false,water=false,collisions=false;
	while(x<map_to_send.width)
	{
		y=0;
		while(y<map_to_send.height)
		{
			if(x*4+y*map_to_send.width*4<Walkable.size())
				walkable=Walkable.mid(x*4+y*map_to_send.width*4,4)!=null_data;
			else//if layer not found
				walkable=false;

			if(x*4+y*map_to_send.width*4<Water.size())
				water=Water.mid(x*4+y*map_to_send.width*4,4)!=null_data;
			else//if layer not found
				water=false;

			if(x*4+y*map_to_send.width*4<Collisions.size())
				collisions=Collisions.mid(x*4+y*map_to_send.width*4,4)!=null_data;
			else//if layer not found
				collisions=false;

			map_to_send.bool_Walkable[x+y*map_to_send.width]=walkable && !water && !collisions;
			map_to_send.bool_Water[x+y*map_to_send.width]=water;
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
				line += QString::number(map_to_send.bool_Walkable[x+y*map_to_send.width]);
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
	emit map_content_loaded();
	return;
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
