#include "ClientNetworkRead.h"

ClientNetworkRead::ClientNetworkRead()
{
	is_logged=false;
	have_send_protocol=false;
	is_logging_in_progess=false;
	stopIt=false;
	socket=NULL;
}

void ClientNetworkRead::stopRead()
{
	stopIt=true;
//	if(socket)
//		disconnect(socket,SIGNAL(readyRead()),this,SLOT(readyRead()));
}

void ClientNetworkRead::fake_send_protocol()
{
	have_send_protocol=true;
}

void ClientNetworkRead::setSocket(QTcpSocket * socket)
{
	this->socket=socket;
	if(socket!=NULL)
	{
		dataClear();
		//this->socket->flush();
		connect(socket,SIGNAL(readyRead()),this,SLOT(readyRead()),Qt::QueuedConnection);
	}
}

void ClientNetworkRead::askIfIsReadyToStop()
{
	emit isReadyToStop();
}

void ClientNetworkRead::stop()
{
	deleteLater();
}

void ClientNetworkRead::readyRead()
{
	if(stopIt)
		return;
	while(socket->bytesAvailable()>0)
	{
		if(stopIt)
			return;
		if(!haveData)
		{
			if(socket->bytesAvailable()<(int)sizeof(int))//ignore because first int is cuted!
			{
				/*emit message(QString("Bytes available is not sufficient to do a int: %1").arg(QString(socket->readAll().toHex())));
				dataClear();
				socket->flush();*/
				return;
			}
			QDataStream in(socket);
			in.setVersion(QDataStream::Qt_4_4);
			in >> dataSize;
			if(dataSize<=sizeof(quint32)) // 0KB
			{
				dataClear();
				emit error(QString("Reply size is too small, seam corrupted: %1").arg(dataSize));
				return;
			}
			if(dataSize>256*1024) // 256KB
			{
				dataClear();
				emit error(QString("Reply size is >256KB, seam corrupted: %1").arg(dataSize));
				return;
			}
			dataSize-=sizeof(quint32);
			haveData=true;
		}
		if(stopIt)
			return;
		if((dataSize-data.size())<socket->bytesAvailable())
		{
			//DebugClass::debugConsole(QString("dataSize(%1)-data.size(%2)=%3").arg(dataSize).arg(data.size()).arg(dataSize-data.size()));
			data.append(socket->read(dataSize-data.size()));
		}
		else
			data.append(socket->readAll());
		if(stopIt)
			return;
		if(dataSize==(quint32)data.size())
		{
			if(is_logged)
				parseInputAfterLogin(data);
			else
				parseInputBeforeLogin(data);
			dataClear();
		}
		if(stopIt)
			return;
	}
}

void ClientNetworkRead::parseInputBeforeLogin(const QByteArray & inputData)
{
	emit message(QString("parseInputBeforeLogin(): inputData: %1").arg(QString(inputData.toHex())));
	QDataStream in(inputData);
	in.setVersion(QDataStream::Qt_4_4);
	if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
	{
		emit error("wrong size to get the ident");
		return;
	}
	in >> mainIdent;
	QByteArray outputData;
	QDataStream out(&outputData, QIODevice::WriteOnly);
	out.setVersion(QDataStream::Qt_4_4);
	switch(mainIdent)
	{
		case 0x02:
		if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)+sizeof(quint32)))
		{
			QString("wrong size with the main ident: %1").arg(mainIdent);
			return;
		}
		in >> subIdent;
		in >> queryNumber;
		switch(subIdent)
		{
			case 0x0001:
				if(!checkStringIntegrity(inputData.right(inputData.size()-in.device()->pos())))
					return;
				{
					QString protocol;
					in >> protocol;
					if(generalData->connected_players>=generalData->max_players)
					{
						out << (quint8)0xC1;
						out << (quint8)queryNumber;
						out << (quint8)3;
						out << QString("Server full");
						emit sendPacket(outputData);
						emit error(QString("Server full (%1/%2)").arg(generalData->connected_players).arg(generalData->max_players));
						return;
					}
					if(protocol==PROTOCOL_HEADER)
					{
						out << (quint8)0xC1;
						out << (quint8)queryNumber;
						out << (quint8)1;
						emit sendPacket(outputData);
						have_send_protocol=true;
						emit message("Protocol sended and replied");
					}
					else
					{
						out << (quint8)0xC1;
						out << (quint8)queryNumber;
						out << (quint8)2;
						emit sendPacket(outputData);
						emit error("Wrong protocol");
						return;
					}
				}
			break;
			case 0x0002:
				if(!have_send_protocol)
				{
					emit error("send login before the protocol");
					return;
				}
				if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
				{
					emit error("wrong size");
					return;
				}
				if(!checkStringIntegrity(inputData.right(inputData.size()-in.device()->pos())))
					return;
				{
					QString login;
					in >> login;
					if((inputData.size()-in.device()->pos())!=20)
						emit error(QString("wrong size with the main ident: %1, because %2 != 20").arg(mainIdent).arg(inputData.size()-in.device()->pos()));
					else if(is_logging_in_progess)
					{
						out << (quint8)0xC1;
						out << (quint8)queryNumber;
						out << (quint8)1;
						emit sendPacket(outputData);
						emit error("Loggin in progress");
					}
					else if(is_logged)
					{
						out << (quint8)0xC1;
						out << (quint8)queryNumber;
						out << (quint8)1;
						emit sendPacket(outputData);
						emit error("Already logged");
					}
					else
					{
						is_logging_in_progess=true;
						QByteArray hash;
						hash=inputData.right(inputData.size()-in.device()->pos());
						emit askLogin(queryNumber,login,hash);
						emit message("Logged");
					}
				}
			break;
			default:
				emit error("wrong data before login with mainIdent: "+QString::number(mainIdent)+", subIdent: "+QString::number(subIdent));
			break;
		}
		break;
		default:
			emit error("wrong data before login with mainIdent: "+QString::number(mainIdent));
		break;
	}
}

void ClientNetworkRead::parseInputAfterLogin(const QByteArray & inputData)
{
	#ifdef DEBUG_MESSAGE_CLIENT_RAW_NETWORK
	emit message(QString("parseInputAfterLogin(): inputData: %1").arg(QString(inputData.toHex())));
	#endif
	QDataStream in(inputData);
	in.setVersion(QDataStream::Qt_4_4);
	if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
	{
		emit error("wrong size to get the ident");
		return;
	}
	in >> mainIdent;
	QByteArray outputData;
	QDataStream out(&outputData, QIODevice::WriteOnly);
	out.setVersion(QDataStream::Qt_4_4);
	switch(mainIdent)
	{
		case 0x02:
		if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)+sizeof(quint32)))
		{
			QString("wrong size with the main ident: %1").arg(mainIdent);
			return;
		}
		in >> subIdent;
		in >> queryNumber;
		switch(subIdent)
		{
			case 0x0005:
				emit(askRandomSeedList(queryNumber));
				return;
			break;
			case 0x000C:
			{
				if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
				{
					QString("wrong size with the main ident: %1").arg(mainIdent);
					return;
				}
				quint32 number_of_file;
				in >> number_of_file;
				QStringList files;
				QList<quint32> timestamps;
				QString tempFileName;
				quint32 tempTimestamps;
				quint32 index=0;
				while(index<number_of_file)
				{
					if(!checkStringIntegrity(inputData.right(inputData.size()-in.device()->pos())))
						return;
					in >> tempFileName;
					files << tempFileName;
					if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
					{
						emit error(QString("wrong size for id with main ident: %1, subIdent: %2, remaining: %3, lower than: %4")
							.arg(mainIdent)
							.arg(subIdent)
							.arg(in.device()->size()-in.device()->pos())
							.arg((int)sizeof(quint32))
							);
						return;
					}
					in >> tempTimestamps;
					timestamps << tempTimestamps;
					index++;
				}
				if(in.device()->size()!=in.device()->pos())
				{
					emit error(QString("remaining data: %1, subIdent: %2").arg(mainIdent).arg(subIdent));
					return;
				}
				emit datapackList(queryNumber,files,timestamps);
				return;
			}
			break;
			default:
				emit error(QString("ident: %1, unknow sub ident: %2").arg(mainIdent).arg(subIdent));
			break;
		}
		break;
		case 0x40:
		if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8)*2)
		{
			emit error("Wrong size in move packet");
			return;
		}
		in >> previousMovedUnit;
		in >> direction;
		if(direction<1 || direction>8)
		{
			emit error(QString("Bad direction number: %1").arg(direction));
			return;
		}
		emit moveThePlayer(previousMovedUnit,(Direction)direction);
		break;
		case 0x42:
		if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
		{
			QString("wrong size with the main ident: %1 and subIdent").arg(mainIdent).arg(subIdent);
			return;
		}
		in >> subIdent;
		switch(subIdent)
		{
			case 0x0003:
			{
				if((inputData.size()-in.device()->pos())<=((int)sizeof(quint8)))
				{
					emit error("wrong remaining size for chat");
					return;
				}
				quint8 chatType;
				in >> chatType;
				if(chatType!=Chat_type_all && chatType!=Chat_type_clan && chatType!=Chat_type_pm)
				{
					emit error("chat type error: "+QString::number(chatType));
					return;
				}
				if(chatType==Chat_type_pm)
				{
					if(!checkStringIntegrity(inputData.right(inputData.size()-in.device()->pos())))
						return;
					QString text;
					in >> text;
					if(!checkStringIntegrity(inputData.right(inputData.size()-in.device()->pos())))
						return;
					QString pseudo;
					in >> pseudo;
					emit message("/pm "+pseudo+" "+text);
					emit sendPM(text,pseudo);
				}
				else
				{
					if(!checkStringIntegrity(inputData.right(inputData.size()-in.device()->pos())))
						return;
					QString text;
					in >> text;
					if(!text.startsWith('/'))
						emit sendChatText((Chat_type)chatType,text);
					else
					{
						QRegExp commandRegExp("^/([a-z]+)( [^ ].*)$");
						if(text.contains(commandRegExp))
						{
							if(player_informations->public_informations.type==Player_type_gm || player_informations->public_informations.type==Player_type_dev)
							{
								QString command=text;
								command.replace(commandRegExp,"\\1");
								if(text.contains(commandRegExp))
								{
									text.replace(commandRegExp,"\\2");
									text.replace(QRegExp("^ (.*)$"),"\\1");
								}
								else
									text="";
								if(command=="kick")
								{
									emit sendBroadCastCommand(command,text);
									emit message("send command: /"+command+" "+text);
								}
								else if(command=="stop" || command=="restart")
								{
									emit serverCommand(command,text);
									emit message("send command: /"+command+" "+text);
								}
								else if(command=="addbots" || command=="removebots")
								{
									emit serverCommand(command,text);
									emit message("send command: /"+command+" "+text);
								}
								else
									emit message("unknow send command: /"+command+" and \""+text+"\"");
							}
							else
								emit message("command ignored because have not the rights: "+text);
						}
						else
							emit message("commands seem not right: \""+text+"\"");
					}
				}
				return;
			}
			break;
			case 0x000A:
			{
				if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
				{
					QString("wrong size with the main ident: %1").arg(mainIdent).arg(subIdent);
					return;
				}
				quint8 type_player_query;
				in >> type_player_query;
				if(type_player_query<1 || type_player_query>2)
				{
					QString("type player query wrong: %1, main ident: %2, sub ident: %3").arg(type_player_query).arg(mainIdent).arg(subIdent);
					return;
				}
				if((in.device()->size()-in.device()->pos())<(int)sizeof(quint16))
				{
					QString("wrong size with the main ident: %1, sub ident: %2").arg(mainIdent).arg(subIdent);
					return;
				}
				QList<quint32> player_ids;
				quint32 id;
				quint16 list_size;
				in >> list_size;
				if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32)*list_size)
				{
					QString("wrong size with the main ident: %1, sub ident: %2").arg(mainIdent).arg(subIdent);
					return;
				}
				int index=0;
				while(index<list_size)
				{
					in >> id;
					player_ids << id;
					index++;
				}
				if(in.device()->size()!=in.device()->pos())
				{
					emit error(QString("remaining data: %1, subIdent: %2").arg(mainIdent).arg(subIdent));
					return;
				}
				emit addPlayersInformationToWatch(player_ids,type_player_query);
				return;
			}
			break;
			case 0x000B:
			{
				if((in.device()->size()-in.device()->pos())<(int)sizeof(quint16))
				{
					QString("wrong size with the main ident: %1, sub ident: %2").arg(mainIdent).arg(subIdent);
					return;
				}
				QList<quint32> player_ids;
				quint32 id;
				quint16 list_size;
				in >> list_size;
				if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32)*list_size)
				{
					QString("wrong size with the main ident: %1, sub ident: %2").arg(mainIdent).arg(subIdent);
					return;
				}
				int index=0;
				while(index<list_size)
				{
					in >> id;
					player_ids << id;
					index++;
				}
				if(in.device()->size()!=in.device()->pos())
				{
					emit error(QString("remaining data: %1, subIdent: %2").arg(mainIdent).arg(subIdent));
					return;
				}
				emit removePlayersInformationToWatch(player_ids);
				return;
			}
			break;
			default:
				emit error(QString("ident: %1, unknow sub ident: %2").arg(mainIdent).arg(subIdent));
			break;
		}
		break;
		default:
			emit error("unknow main ident: "+QString::number(mainIdent));
		break;
	}
}

void ClientNetworkRead::dataClear()
{
	data.clear();
	dataSize=0;
	haveData=false;
}

bool ClientNetworkRead::checkStringIntegrity(const QByteArray & data)
{
	if(data.size()<(int)sizeof(qint32))
	{
		emit error("header size not suffisient");
		return false;
	}
	qint32 stringSize;
	QDataStream in(data);
	in.setVersion(QDataStream::Qt_4_4);
	in >> stringSize;
	if(stringSize>65535)
	{
		emit error(QString("String size is wrong: %1").arg(stringSize));
		return false;
	}
	if(data.size()<stringSize)
	{
		emit error(QString("String size is greater than the data: %1>%2").arg(data.size()).arg(stringSize));
		return false;
	}
	return true;
}

void ClientNetworkRead::setVariable(GeneralData *generalData,Player_private_and_public_informations *player_informations)
{
	this->generalData=generalData;
	this->player_informations=player_informations;
}

void ClientNetworkRead::send_player_informations()
{
	is_logged=true;
}

/// \warning it need be complete protocol trame
void ClientNetworkRead::fake_receive_data(const QByteArray &data)
{
	parseInputAfterLogin(data);
}
