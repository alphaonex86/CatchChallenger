#include "ClientHeavyLoad.h"
#include "EventDispatcher.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

QList<quint16> ClientHeavyLoad::simplifiedIdList;

ClientHeavyLoad::ClientHeavyLoad()
{
}

ClientHeavyLoad::~ClientHeavyLoad()
{
}

void ClientHeavyLoad::setVariable(Player_internal_informations *player_informations)
{
	this->player_informations=player_informations;
}

void ClientHeavyLoad::askLogin(const quint8 &query_id,const QString &login,const QByteArray &hash)
{
	QByteArray outputData;
	QDataStream out(&outputData, QIODevice::WriteOnly);
	out.setVersion(QDataStream::Qt_4_4);
	if(player_informations->isFake)
	{
		if(EventDispatcher::generalData.serverPrivateVariables.botSpawn.size()==0)
		{
			out << (quint8)0x01;
			out << QString("Not bot point");
			emit postReply(query_id,outputData);
			emit message(QString("Not bot point"));
		}
		else
		{
			if(!EventDispatcher::generalData.serverPrivateVariables.map_list.contains(EventDispatcher::generalData.serverPrivateVariables.botSpawn.at(EventDispatcher::generalData.serverPrivateVariables.botSpawnIndex).map))
			{
				out << (quint8)0x01;
				out << QString("Bot point not resolved");
				emit postReply(query_id,outputData);
				emit message(QString("Bot point not resolved"));
			}
			else if(simplifiedIdList.size()>0)
			{
				out << (quint8)0x01;
				out << QString("Not free id to login");
				emit postReply(query_id,outputData);
				emit message(QString("Not free id to login"));
			}
			else
			{
				player_informations->is_logged=true;
				player_informations->public_and_private_informations.public_informations.simplifiedId = simplifiedIdList.first();
				simplifiedIdList.removeFirst();
				player_informations->public_and_private_informations.public_informations.clan=0;
				player_informations->id=999999999-EventDispatcher::generalData.serverPrivateVariables.number_of_bots_logged;
				player_informations->public_and_private_informations.public_informations.pseudo=QString("bot_%1").arg(player_informations->id);
				if(!loadTheRawPseudo())
				{
					out << (quint8)01;
					out << QString("Convert into utf8 have wrong size");
					emit postReply(query_id,outputData);
					emit error("Convert into utf8 have wrong size");
					return;
				}
				player_informations->public_and_private_informations.public_informations.skin="";//useless for serveur benchmark
				player_informations->public_and_private_informations.public_informations.type=Player_type_normal;
				player_informations->public_and_private_informations.cash=0;
				EventDispatcher::generalData.serverPrivateVariables.connected_players_id_list << player_informations->id;
				emit send_player_informations();
				emit isLogged();
				//remplace x and y by real spawn point
				emit put_on_the_map(EventDispatcher::generalData.serverPrivateVariables.map_list[EventDispatcher::generalData.serverPrivateVariables.botSpawn.at(EventDispatcher::generalData.serverPrivateVariables.botSpawnIndex).map],
						    EventDispatcher::generalData.serverPrivateVariables.botSpawn.at(EventDispatcher::generalData.serverPrivateVariables.botSpawnIndex).x,
						    EventDispatcher::generalData.serverPrivateVariables.botSpawn.at(EventDispatcher::generalData.serverPrivateVariables.botSpawnIndex).y,
						    Orientation_bottom,POKECRAFT_SERVER_NORMAL_SPEED);
				EventDispatcher::generalData.serverPrivateVariables.botSpawnIndex++;
				if(EventDispatcher::generalData.serverPrivateVariables.botSpawnIndex>=EventDispatcher::generalData.serverPrivateVariables.botSpawn.size())
					EventDispatcher::generalData.serverPrivateVariables.botSpawnIndex=0;
				EventDispatcher::generalData.serverPrivateVariables.number_of_bots_logged++;
				out << (quint8)02;
				out << (quint16)EventDispatcher::generalData.serverSettings.max_players;
				if(EventDispatcher::generalData.serverSettings.max_players<=255)
					out << (quint8)player_informations->public_and_private_informations.public_informations.simplifiedId;
				else
					out << (quint16)player_informations->public_and_private_informations.public_informations.simplifiedId;
				out << (quint32)player_informations->public_and_private_informations.cash;
				emit postReply(query_id,outputData);
			}
		}
		return;
	}
	QSqlQuery loginQuery=EventDispatcher::generalData.serverPrivateVariables.loginQuery;
	loginQuery.bindValue(":login",login);
	loginQuery.bindValue(":password",QString(hash.toHex()));
	if(!loginQuery.exec())
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
		if(EventDispatcher::generalData.serverPrivateVariables.connected_players_id_list.contains(loginQuery.value(0).toUInt()))
		{
			out << (quint8)0x01;
			out << QString("Already logged");
			emit postReply(query_id,outputData);
			emit error("Already logged");
		}
		else if(simplifiedIdList.size()>0)
		{
			out << (quint8)0x01;
			out << QString("Not free id to login");
			emit postReply(query_id,outputData);
			emit message(QString("Not free id to login"));
		}
		else
		{
			player_informations->is_logged=true;
			player_informations->public_and_private_informations.public_informations.simplifiedId = simplifiedIdList.first();
			simplifiedIdList.removeFirst();
			EventDispatcher::generalData.serverPrivateVariables.connected_players_id_list << loginQuery.value(0).toUInt();
			player_informations->public_and_private_informations.public_informations.clan=loginQuery.value(8).toUInt();
			player_informations->id=loginQuery.value(0).toUInt();
			player_informations->public_and_private_informations.public_informations.pseudo=loginQuery.value(1).toString();
			player_informations->public_and_private_informations.public_informations.skin=loginQuery.value(2).toString();
			player_informations->public_and_private_informations.public_informations.type=(Player_type)loginQuery.value(7).toUInt();
			player_informations->public_and_private_informations.cash=0;
			if(!loadTheRawPseudo())
			{
				out << (quint8)01;
				out << QString("Convert into utf8 have wrong size");
				emit postReply(query_id,outputData);
				emit error("Convert into utf8 have wrong size");
				return;
			}
			int orentation=loginQuery.value(5).toInt();
			if(orentation<1 || orentation>8)
			{
				emit message(QString("Wrong orientation corrected: %1").arg(orentation));
				orentation=3;
			}
			if(EventDispatcher::generalData.serverPrivateVariables.map_list.contains(loginQuery.value(8).toString()))
			{
				out << (quint8)02;
				out << (quint16)EventDispatcher::generalData.serverSettings.max_players;
				if(EventDispatcher::generalData.serverSettings.max_players<=255)
					out << (quint8)player_informations->public_and_private_informations.public_informations.simplifiedId;
				else
					out << (quint16)player_informations->public_and_private_informations.public_informations.simplifiedId;
				out << (quint32)player_informations->public_and_private_informations.cash;
				emit message(QString("Logged: %1").arg(player_informations->public_and_private_informations.public_informations.pseudo));
				emit postReply(query_id,outputData);
				emit send_player_informations();
				emit isLogged();
				emit put_on_the_map(
					EventDispatcher::generalData.serverPrivateVariables.map_list[loginQuery.value(6).toString()],//map pointer
					loginQuery.value(3).toInt(),//position_x
					loginQuery.value(4).toInt(),//position_y
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
				return;
			}
		}
	}
}

bool ClientHeavyLoad::loadTheRawPseudo()
{
	if(player_informations->public_and_private_informations.public_informations.pseudo.size()>255 || player_informations->public_and_private_informations.public_informations.pseudo.size()==0)
		return false;
	QByteArray rawPseudo=player_informations->public_and_private_informations.public_informations.pseudo.toUtf8();
	if(rawPseudo.size()>255 || rawPseudo.size()==0)
		return false;
	player_informations->rawPseudo[0]=rawPseudo.size();
	player_informations->rawPseudo+=rawPseudo;
	return true;
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
	if(player_informations->is_logged)
	{
		simplifiedIdList << player_informations->public_and_private_informations.public_informations.simplifiedId;
		EventDispatcher::generalData.serverPrivateVariables.connected_players_id_list.remove(player_informations->id);
	}
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
		if(!fileName.contains(EventDispatcher::generalData.serverPrivateVariables.datapack_rightFileName))
		{
			//emit error(QString("file have not match the regex: %1").arg(fileName));
			//return;
			out << (quint8)0x02;
		}
		else
		{
			if(sendFileIfNeeded(EventDispatcher::generalData.serverPrivateVariables.datapack_basePath+fileName,fileName,mtime,true))
				out << (quint8)0x01;
			else
				out << (quint8)0x02;
		}
		loopIndex++;
	}
	//send not in the list
	listDatapack("",files);
	emit postReply(query_id,qCompress(outputData,9));
}

bool ClientHeavyLoad::sendFileIfNeeded(const QString &filePath,const QString &fileName,const quint32 &mtime,const bool &checkMtime)
{
	QFile file(filePath);
	if(file.size()>8*1024*1024)
		return false;
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
		bool returnVal=sendFile(fileName,content,localMtime);
		file.close();
		return returnVal;
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
	QDir finalDatapackFolder(EventDispatcher::generalData.serverPrivateVariables.datapack_basePath+suffix);
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
			if(fileName.contains(EventDispatcher::generalData.serverPrivateVariables.datapack_rightFileName))
			{
				if(!files.contains(fileName))
					sendFileIfNeeded(fileName,0,false);
			}
		}
	}
}

bool ClientHeavyLoad::sendFile(const QString &fileName,const QByteArray &content,const quint32 &mtime)
{
	if(fileName.size()>255 || fileName.size()==0)
		return false;
	QByteArray fileNameRaw=fileName.toUtf8();
	if(fileNameRaw.size()>255 || fileNameRaw.size()==0)
		return false;
	QByteArray outputData;
	outputData[0]=fileNameRaw.size();
	outputData+=fileName;
	QDataStream out(&outputData, QIODevice::WriteOnly);
	out.setVersion(QDataStream::Qt_4_4);
	out << (quint32)content.size();
	out << mtime;
	outputData+=content;
	emit sendPacket(0xC2,0x0003,outputData);
	return true;
}

QString ClientHeavyLoad::SQL_text_quote(QString text)
{
	return text.replace("'","\\'");
}

void ClientHeavyLoad::updatePlayerPosition(const QString &map,const quint16 &x,const quint16 &y,const Orientation &orientation)
{
	if(!player_informations->is_logged || player_informations->isFake)
		return;
	QSqlQuery updateMapPositionQuery=EventDispatcher::generalData.serverPrivateVariables.updateMapPositionQuery;
	updateMapPositionQuery.bindValue(":map_name",map);
	updateMapPositionQuery.bindValue(":position_x",x);
	updateMapPositionQuery.bindValue(":position_y",y);
	updateMapPositionQuery.bindValue(":orientation",orientation);
	updateMapPositionQuery.bindValue(":id",player_informations->id);
	if(!updateMapPositionQuery.exec())
		DebugClass::debugConsole(QString("Sql query failed: %1, error: %2").arg(updateMapPositionQuery.lastQuery()).arg(updateMapPositionQuery.lastError().text()));
}
