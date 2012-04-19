#include "ClientHeavyLoad.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

ClientHeavyLoad::ClientHeavyLoad()
{
	is_logged=false;
	fake_mode=false;
	basePath=QCoreApplication::applicationDirPath()+"/datapack/map/tmx/";
	datapack_base_name=QCoreApplication::applicationDirPath()+"/datapack/";
	right_file_name=QRegExp("^[0-9/a-zA-Z\\.\\- _]+\\.[a-z]{3,4}$");
}

ClientHeavyLoad::~ClientHeavyLoad()
{
}

void ClientHeavyLoad::setVariable(
	QSqlDatabase *db,
	QStringList *cached_files_name,
	QList<QByteArray> *cached_files_data,
	QList<quint32> *cached_files_mtime,
	qint64 *cache_max_file_size,
	qint64 *cache_max_size,
	qint64 *cache_size,
	QList<quint32> *connected_players_id_list)
{
	this->db=db;
	this->cached_files_name=cached_files_name;
	this->cached_files_data=cached_files_data;
	this->cached_files_mtime=cached_files_mtime;
	this->cache_max_file_size=cache_max_file_size;
	this->cache_max_size=cache_max_size;
	this->cache_size=cache_size;
	this->connected_players_id_list=connected_players_id_list;
}

void ClientHeavyLoad::askLogin(quint8 query_id,QString login,QByteArray hash)
{
	QByteArray outputData;
	QDataStream out(&outputData, QIODevice::WriteOnly);
	out.setVersion(QDataStream::Qt_4_4);
	QSqlQuery loginQuery;
	if(!loginQuery.exec("SELECT * FROM `player` WHERE `login`=\'"+SQL_text_quote(login)+"\' AND `password`=\'"+SQL_text_quote(hash.toHex())+"\'"))
	{
		out << (quint8)0xC1;
		out << (quint8)query_id;
		out << (quint8)01;
		out << QString("Mysql error");
		emit sendPacket(outputData);
		emit message(loginQuery.lastQuery()+": "+loginQuery.lastError().text());
		return;
	}
	if(loginQuery.size()==0)
	{
		out << (quint8)0xC1;
		out << (quint8)query_id;
		out << (quint8)01;
		out << QString("Bad login");
		emit sendPacket(outputData);
		emit error("Bad login for: "+login+", hash: "+hash.toHex());
	}
	else
	{
		loginQuery.next();
		if(connected_players_id_list->contains(loginQuery.value(0).toUInt()))
		{
			out << (quint8)0xC1;
			out << (quint8)query_id;
			out << (quint8)01;
			out << QString("Already logged");
			emit sendPacket(outputData);
			emit error("Already logged");
		}
		else
		{
			(*connected_players_id_list) << loginQuery.value(0).toUInt();
			out << (quint8)0xC1;
			out << (quint8)query_id;
			out << (quint8)02;
			out << (quint32)loginQuery.value(0).toUInt();
			is_logged=true;
			player_informations.public_informations.clan=loginQuery.value(10).toUInt();
			player_informations.public_informations.description="";
			player_informations.public_informations.id=loginQuery.value(0).toUInt();
			player_informations.public_informations.pseudo=loginQuery.value(1).toString();
			player_informations.public_informations.skin=loginQuery.value(4).toString();
			player_informations.public_informations.type=(Player_type)loginQuery.value(9).toUInt();
			player_informations.cash=0;
			int orentation=loginQuery.value(7).toInt();
			if(orentation<1 || orentation>8)
			{
				emit message(QString("Wrong orientation corrected: %1").arg(orentation));
				orentation=3;
			}
			emit sendPacket(outputData);
			emit send_player_informations(player_informations);
			emit isLogged();
			emit put_on_the_map(
				player_informations.public_informations.id,
				loginQuery.value(8).toString(),//map_name
				loginQuery.value(5).toInt(),//position_x
				loginQuery.value(6).toInt(),//position_y
				(Orientation)orentation,
				POKECRAFT_SERVER_NORMAL_SPEED //speed in ms for each tile (step)
			);
		}
	}
}

void ClientHeavyLoad::fakeLogin(quint32 last_fake_player_id,quint16 x,quint16 y,QString map,Orientation orientation,QString skin)
{
	fake_mode=true;
	(*connected_players_id_list) << last_fake_player_id;
	is_logged=true;
	player_informations.public_informations.clan=0;
	player_informations.public_informations.description="Bot";
	player_informations.public_informations.id=last_fake_player_id;
	player_informations.public_informations.pseudo=QString("bot_%1").arg(last_fake_player_id);
	player_informations.public_informations.skin=skin;//useless for serveur benchmark
	player_informations.public_informations.type=Player_type_normal;
	player_informations.cash=0;
	emit send_player_informations(player_informations);
	emit isLogged();
	//remplace x and y by real spawn point
	emit put_on_the_map(last_fake_player_id,map,x,y,orientation,POKECRAFT_SERVER_NORMAL_SPEED);
}

void ClientHeavyLoad::askRandomSeedList(quint8 query_id)
{
	QByteArray randomData;
	QDataStream randomDataStream(&randomData, QIODevice::WriteOnly);
	randomDataStream.setVersion(QDataStream::Qt_4_4);
	int index=0;
	while(index<512)
	{
		randomDataStream << quint8(rand()%256);
		index++;
	}
	emit setRandomSeedList(randomData);
	QByteArray outputData;
	QDataStream out(&outputData, QIODevice::WriteOnly);
	out.setVersion(QDataStream::Qt_4_4);
	out << (quint8)0xC1;
	out << (quint8)query_id;
	out << randomData;
	emit sendPacket(outputData);
}

void ClientHeavyLoad::askIfIsReadyToStop()
{
	if(is_logged)
		connected_players_id_list->removeOne(player_informations.public_informations.id);
	emit isReadyToStop();
}

void ClientHeavyLoad::datapackList(quint8 query_id,QStringList files,QList<quint32> timestamps)
{
	QByteArray outputData;
	QDataStream out(&outputData, QIODevice::WriteOnly);
	out.setVersion(QDataStream::Qt_4_4);
	out << (quint8)0xC1;
	out << (quint8)query_id;
	int loopIndex=0;
	int loop_size=files.size();
	while(loopIndex<loop_size)
	{
		QString fileName=files.at(loopIndex);
		quint32 mtime=timestamps.at(loopIndex);
		if(fileName.contains("./") || fileName.contains("\\") || fileName.contains("//"))
		{
			emit error(QString("file name contains illegale char: %1").arg(fileName));
			return;
		}
		if(fileName.contains(QRegExp("^[a-zA-Z]:/")) || fileName.startsWith("/"))
		{
			emit error(QString("start with wrong string: %1").arg(fileName));
			return;
		}
		if(!fileName.contains(right_file_name))
		{
			//emit error(QString("file have not match the regex: %1").arg(fileName));
			//return;
			out << (quint8)0x02;
		}
		else
		{
			if(sendFileIfNeeded(fileName,mtime,true))
				out << (quint8)0x01;
			else
				out << (quint8)0x02;
		}
		loopIndex++;
	}
	//send not in the list
	listDatapack("",files);
	emit sendPacket(outputData);
}

bool ClientHeavyLoad::sendFileIfNeeded(const QString &fileName,const quint32 &mtime,const bool &checkMtime)
{
	QString path=datapack_base_name+fileName;
	QFile file(path);
	int index=cached_files_name->indexOf(fileName);
	if(index!=-1)
	{
		if(!checkMtime || mtime!=cached_files_mtime->at(index))
		{
			emit message(QString("send the file: %1, checkMtime: %2, mtime: %3, file server mtime: %4")
				     .arg(fileName)
				     .arg(checkMtime)
				     .arg(mtime)
				     .arg(cached_files_mtime->at(index))
			);
			sendFile(fileName,cached_files_data->at(index),cached_files_mtime->at(index));
		}
		/*else
			emit message(QString("File already update: %1, checkMtime: %2, mtime: %3, file server mtime: %4")
				     .arg(fileName)
				     .arg(checkMtime)
				     .arg(mtime)
				     .arg(cached_files_mtime->at(index))
				     );*/
		return true;
	}
	else
	{
		if(file.open(QIODevice::ReadOnly))
		{
			quint64 localMtime=QFileInfo(file).lastModified().toTime_t();
			QByteArray content=file.readAll();
			bool put_in_cache=false;
			if(file.size()>*cache_max_file_size)
				emit message("File too big: "+fileName);
			else
			{
				if(*cache_size+content.size()>*cache_max_size)
					emit message("Cache too small");
				else
				{
					//emit message(QString("cache server generated for the file: %1").arg(fileName));
					*cached_files_name << fileName;
					*cached_files_data << content;
					*cached_files_mtime << localMtime;
					put_in_cache=true;
				}
			}
			if(!checkMtime || mtime!=localMtime)
			{
				emit message(QString("send the file without cache: %1, checkMtime: %2, mtime: %3, file server mtime: %4")
					     .arg(fileName)
					     .arg(checkMtime)
					     .arg(mtime)
					     .arg(localMtime)
				);
				sendFile(fileName,content,localMtime);
			}
			/*else
				emit message(QString("File already update without cache: %1, checkMtime: %2, mtime: %3, file server mtime: %4")
					     .arg(fileName)
					     .arg(checkMtime)
					     .arg(mtime)
					     .arg(localMtime)
					     );*/
			file.close();
			return true;
		}
		else
		{
			emit message("Unable to open: "+path+", error: "+file.errorString());
			return false;
		}
	}
}

void ClientHeavyLoad::listDatapack(QString suffix,const QStringList &files)
{
	//do source check
	QDir finalDatapackFolder(datapack_base_name+suffix);
	QString fileName;
	QFileInfoList entryList=finalDatapackFolder.entryInfoList(QDir::AllEntries|QDir::NoDotAndDotDot|QDir::Hidden|QDir::System,QDir::DirsFirst);//possible wait time here
	int sizeEntryList=entryList.size();
	for (int index=0;index<sizeEntryList;++index)
	{
		QFileInfo fileInfo=entryList.at(index);
		if(fileInfo.isDir())
			listDatapack(suffix+fileInfo.fileName()+"/",files);//put unix separator because it's transformed into that's under windows too
		else
		{
			fileName=suffix+fileInfo.fileName();
			if(fileName.contains(right_file_name))
			{
				if(!files.contains(fileName))
					sendFileIfNeeded(fileName,0,false);
			}
		}
	}
}

void ClientHeavyLoad::updatePlayerPosition(QString map,quint16 x,quint16 y,Orientation orientation)
{
	if(!is_logged || fake_mode)
		return;
	QSqlQuery loginQuery;
	loginQuery.exec(QString("UPDATE `player` SET `map_name`='%1',`position_x`='%2',`position_y`='%3',`orientation`='%4' WHERE `id`=%5")
			.arg(SQL_text_quote(map))
			.arg(x)
			.arg(y)
			.arg(orientation)
			.arg(player_informations.public_informations.id));
	DebugClass::debugConsole(QString("UPDATE `player` SET `map_name`='%1',`position_x`='%2',`position_y`='%3',`orientation`='%4' WHERE `id`=%5")
			 .arg(SQL_text_quote(map))
			 .arg(x)
			 .arg(y)
			 .arg(orientation)
			 .arg(player_informations.public_informations.id));
}

void ClientHeavyLoad::sendFile(const QString &fileName,const QByteArray &content,const quint32 &mtime)
{
	QByteArray outputData;
	QDataStream out(&outputData, QIODevice::WriteOnly);
	out.setVersion(QDataStream::Qt_4_4);
	out << (quint8)0xC2;
	out << (quint32)0x00000003;
	out << fileName;
	out << (quint32)content.size();
	out << mtime;
	outputData+=content;
	emit sendPacket(outputData);
}

QString ClientHeavyLoad::SQL_text_quote(QString text)
{
	return text.replace("'","\\'");
}
