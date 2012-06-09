#include "ClientHeavyLoad.h"
#include "EventDispatcher.h"

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

void ClientHeavyLoad::setVariable(Player_private_and_public_informations *player_informations)
{
	this->player_informations=player_informations;
}

void ClientHeavyLoad::askLogin(const quint8 &query_id,const QString &login,const QByteArray &hash)
{
	QByteArray outputData;
	QDataStream out(&outputData, QIODevice::WriteOnly);
	out.setVersion(QDataStream::Qt_4_4);
	QSqlQuery loginQuery;
	if(!loginQuery.exec("SELECT * FROM `player` WHERE `login`=\'"+SQL_text_quote(login)+"\' AND `password`=\'"+SQL_text_quote(hash.toHex())+"\'"))
	{
		out << (quint8)0x01;
		out << QString("Mysql error");
		emit postReply(query_id,outputData);
		emit message(loginQuery.lastQuery()+": "+loginQuery.lastError().text());
		return;
	}
	if(loginQuery.size()==0)
	{
		out << (quint8)0x01;
		out << QString("Bad login");
		emit postReply(query_id,outputData);
		emit error("Bad login for: "+login+", hash: "+hash.toHex());
	}
	else
	{
		loginQuery.next();
		if(EventDispatcher::generalData.connected_players_id_list.contains(loginQuery.value(0).toUInt()))
		{
			out << (quint8)0x01;
			out << QString("Already logged");
			emit postReply(query_id,outputData);
			emit error("Already logged");
		}
		else
		{
			EventDispatcher::generalData.connected_players_id_list << loginQuery.value(0).toUInt();
			is_logged=true;
			player_informations->public_informations.clan=loginQuery.value(10).toUInt();
			player_informations->public_informations.description="";
			player_informations->public_informations.id=loginQuery.value(0).toUInt();
			player_informations->public_informations.pseudo=loginQuery.value(1).toString();
			player_informations->public_informations.skin=loginQuery.value(4).toString();
			player_informations->public_informations.type=(Player_type)loginQuery.value(9).toUInt();
			player_informations->cash=0;
			int orentation=loginQuery.value(7).toInt();
			if(orentation<1 || orentation>8)
			{
				emit message(QString("Wrong orientation corrected: %1").arg(orentation));
				orentation=3;
			}
			if(EventDispatcher::generalData.map_list.contains(loginQuery.value(8).toString()))
			{
				out << (quint8)02;
				out << (quint32)loginQuery.value(0).toUInt();
				emit message(QString("Logged: %1").arg(player_informations->public_informations.pseudo));
				emit postReply(query_id,outputData);
				emit send_player_informations();
				emit isLogged();
				emit put_on_the_map(
					player_informations->public_informations.id,
					EventDispatcher::generalData.map_list[loginQuery.value(8).toString()],//map pointer
					loginQuery.value(5).toInt(),//position_x
					loginQuery.value(6).toInt(),//position_y
					(Orientation)orentation,
					POKECRAFT_SERVER_NORMAL_SPEED //speed in ms for each tile (step delay)
				);
			}
			else
			{
				out << (quint8)01;
				out << QString("Map not found");
				emit postReply(query_id,outputData);
				emit error("Map not found: "+loginQuery.value(8).toString());
			}
		}
	}
}

void ClientHeavyLoad::fakeLogin(const quint32 &last_fake_player_id,const quint16 &x,const quint16 &y,Map_final *map,const Orientation &orientation,const QString &skin)
{
	fake_mode=true;
	EventDispatcher::generalData.connected_players_id_list << last_fake_player_id;
	is_logged=true;
	player_informations->public_informations.clan=0;
	player_informations->public_informations.description="Bot";
	player_informations->public_informations.id=last_fake_player_id;
	player_informations->public_informations.pseudo=QString("bot_%1").arg(last_fake_player_id);
	player_informations->public_informations.skin=skin;//useless for serveur benchmark
	player_informations->public_informations.type=Player_type_normal;
	player_informations->cash=0;
	emit send_player_informations();
	emit isLogged();
	//remplace x and y by real spawn point
	emit put_on_the_map(last_fake_player_id,map,x,y,orientation,POKECRAFT_SERVER_NORMAL_SPEED);
}

void ClientHeavyLoad::askRandomSeedList(const quint8 &query_id)
{
	QByteArray randomData;
	QDataStream randomDataStream(&randomData, QIODevice::WriteOnly);
	randomDataStream.setVersion(QDataStream::Qt_4_4);
	int index=0;
	while(index<POKECRAFT_SERVER_RANDOM_LIST_SIZE)
	{
		randomDataStream << quint8(rand()%256);
		index++;
	}
	emit setRandomSeedList(randomData);
	emit postReply(query_id,randomData);
}

void ClientHeavyLoad::askIfIsReadyToStop()
{
	if(is_logged)
		EventDispatcher::generalData.connected_players_id_list.remove(player_informations->public_informations.id);
	emit isReadyToStop();
}

void ClientHeavyLoad::stop()
{
	deleteLater();
}

void ClientHeavyLoad::datapackList(const quint8 &query_id,const QStringList &files,const QList<quint32> &timestamps)
{
	QByteArray outputData;
	QDataStream out(&outputData, QIODevice::WriteOnly);
	out.setVersion(QDataStream::Qt_4_4);
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
			if(sendFileIfNeeded(datapack_base_name+fileName,fileName,mtime,true))
				out << (quint8)0x01;
			else
				out << (quint8)0x02;
		}
		loopIndex++;
	}
	//send not in the list
	listDatapack("",files);
	emit postReply(query_id,outputData);
}

bool ClientHeavyLoad::sendFileIfNeeded(const QString &filePath,const QString &fileName,const quint32 &mtime,const bool &checkMtime)
{
	QFile file(filePath);
	quint64 localMtime=QFileInfo(file).lastModified().toTime_t();
	if(localMtime==mtime)
		return false;
	if(file.open(QIODevice::ReadOnly))
	{
		QByteArray content=file.readAll();
		emit message(QString("send the file without cache: %1, checkMtime: %2, mtime: %3, file server mtime: %4")
			     .arg(fileName)
			     .arg(checkMtime)
			     .arg(mtime)
			     .arg(localMtime)
		);
		sendFile(fileName,content,localMtime);
		file.close();
		return true;
	}
	else
	{
		emit message("Unable to open: "+filePath+", error: "+file.errorString());
		return false;
	}
}

void ClientHeavyLoad::listDatapack(const QString &suffix,const QStringList &files)
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

void ClientHeavyLoad::sendFile(const QString &fileName,const QByteArray &content,const quint32 &mtime)
{
	QByteArray outputData;
	QDataStream out(&outputData, QIODevice::WriteOnly);
	out.setVersion(QDataStream::Qt_4_4);
	out << fileName;
	out << (quint32)content.size();
	out << mtime;
	outputData+=content;
	emit sendPacket(0xC2,0x0003,outputData);
}

QString ClientHeavyLoad::SQL_text_quote(QString text)
{
	return text.replace("'","\\'");
}

void ClientHeavyLoad::updatePlayerPosition(const QString &map,const quint16 &x,const quint16 &y,const Orientation &orientation)
{
	if(!is_logged || fake_mode)
		return;
	QSqlQuery loginQuery;
	loginQuery.exec(QString("UPDATE `player` SET `map_name`='%1',`position_x`='%2',`position_y`='%3',`orientation`='%4' WHERE `id`=%5")
			.arg(SQL_text_quote(map))
			.arg(x)
			.arg(y)
			.arg(orientation)
			.arg(player_informations->public_informations.id));
	DebugClass::debugConsole(QString("UPDATE `player` SET `map_name`='%1',`position_x`='%2',`position_y`='%3',`orientation`='%4' WHERE `id`=%5")
			 .arg(SQL_text_quote(map))
			 .arg(x)
			 .arg(y)
			 .arg(orientation)
			 .arg(player_informations->public_informations.id));
}
