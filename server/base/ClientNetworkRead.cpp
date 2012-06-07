#include "ClientNetworkRead.h"

ClientNetworkRead::ClientNetworkRead(GeneralData *generalData,Player_private_and_public_informations *player_informations,QTcpSocket * socket) :
	ProtocolParsingInput(socket)
{
	is_logged=false;
	have_send_protocol=false;
	is_logging_in_progess=false;
	stopIt=false;
	this->generalData=generalData;
	this->player_informations=player_informations;
	this->socket=socket;
	if(socket!=NULL)
	{
		//this->socket->flush();
		connect(socket,SIGNAL(readyRead()),this,SLOT(readyRead()),Qt::QueuedConnection);
	}
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
	ProtocolParsingInput::parseIncommingData();
}

void ClientNetworkRead::parseInputBeforeLogin(const QByteArray & inputData)
{
	#ifdef DEBUG_MESSAGE_CLIENT_RAW_NETWORK
	emit message(QString("parseInputBeforeLogin(): inputData: %1").arg(QString(inputData.toHex())));
	#endif
	QDataStream in(inputData);
	in.setVersion(QDataStream::Qt_4_4);
	if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
	{
		emit error("wrong size to get the ident");
		return;
	}
	in >> mainCodeType;
	QByteArray outputData;
	QDataStream out(&outputData, QIODevice::WriteOnly);
	out.setVersion(QDataStream::Qt_4_4);
	switch(mainCodeType)
	{
		case 0x02:
		if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)+sizeof(quint32)))
		{
			QString("wrong size with the main ident: %1").arg(mainCodeType);
			return;
		}
		in >> subCodeType;
		in >> queryNumber;
		switch(subCodeType)
		{
			case 0x0001:
				if(!checkStringIntegrity(inputData.right(inputData.size()-in.device()->pos())))
					return;
				{
					QString protocol;
					in >> protocol;
					if(generalData->connected_players>=generalData->max_players)
					{
						out << (quint8)queryNumber;	//query number
						out << (quint8)0x03;		//server full
						out << QString("Server full");
						emit sendPacket(0xC1,0x00,true,outputData);
						emit error(QString("Server full (%1/%2)").arg(generalData->connected_players).arg(generalData->max_players));
						return;
					}
					if(protocol==PROTOCOL_HEADER)
					{
						out << (quint8)queryNumber;	//query number
						out << (quint8)0x01;		//protocol supported
						/*out << (quint8)0x00;		//raw (no compression)
						out << (quint8)0x01;		//upload size type: small (client -> server)
						out << (quint8)0x01;		//upload size type: small (client -> server)*/
						emit sendPacket(0xC1,0x00,true,outputData);
						have_send_protocol=true;
						emit message("Protocol sended and replied");
					}
					else
					{
						out << (quint8)queryNumber;	//query number
						out << (quint8)0x02;		//protocol not supported
						emit sendPacket(0xC1,0x00,true,outputData);
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
						emit error(QString("wrong size with the main ident: %1, because %2 != 20").arg(mainCodeType).arg(inputData.size()-in.device()->pos()));
					else if(is_logging_in_progess)
					{
						out << (quint8)queryNumber;
						out << (quint8)1;
						emit sendPacket(0xC1,0x00,true,outputData);
						emit error("Loggin in progress");
					}
					else if(is_logged)
					{
						out << (quint8)queryNumber;
						out << (quint8)1;
						emit sendPacket(0xC1,0x00,true,outputData);
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
				emit error("wrong data before login with mainIdent: "+QString::number(mainCodeType)+", subIdent: "+QString::number(subCodeType));
			break;
		}
		break;
		default:
			emit error("wrong data before login with mainIdent: "+QString::number(mainCodeType));
		break;
	}
}

void ClientNetworkRead::parseMessage(const quint8 &mainCodeType,const QByteArray &data)
{
}

void ClientNetworkRead::parseMessage(const quint8 &mainCodeType,const quint16 &subCodeType,const QByteArray &data)
{
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
	in >> mainCodeType;
	QByteArray outputData;
	QDataStream out(&outputData, QIODevice::WriteOnly);
	out.setVersion(QDataStream::Qt_4_4);
	switch(mainCodeType)
	{
		case 0x02:
		if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)+sizeof(quint32)))
		{
			QString("wrong size with the main ident: %1").arg(mainCodeType);
			return;
		}
		in >> subCodeType;
		in >> queryNumber;
		switch(subCodeType)
		{
			case 0x0005:
				emit(askRandomSeedList(queryNumber));
				return;
			break;
			case 0x000C:
			{
				if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
				{
					QString("wrong size with the main ident: %1").arg(mainCodeType);
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
							.arg(mainCodeType)
							.arg(subCodeType)
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
					emit error(QString("remaining data: %1, subIdent: %2").arg(mainCodeType).arg(subCodeType));
					return;
				}
				emit datapackList(queryNumber,files,timestamps);
				return;
			}
			break;
			default:
				emit error(QString("ident: %1, unknow sub ident: %2").arg(mainCodeType).arg(subCodeType));
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
			QString("wrong size with the main ident: %1 and subIdent").arg(mainCodeType).arg(subCodeType);
			return;
		}
		in >> subCodeType;
		switch(subCodeType)
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
					QString("wrong size with the main ident: %1").arg(mainCodeType).arg(subCodeType);
					return;
				}
				quint8 type_player_query;
				in >> type_player_query;
				if(type_player_query<1 || type_player_query>2)
				{
					QString("type player query wrong: %1, main ident: %2, sub ident: %3").arg(type_player_query).arg(mainCodeType).arg(subCodeType);
					return;
				}
				if((in.device()->size()-in.device()->pos())<(int)sizeof(quint16))
				{
					QString("wrong size with the main ident: %1, sub ident: %2").arg(mainCodeType).arg(subCodeType);
					return;
				}
				QList<quint32> player_ids;
				quint32 id;
				quint16 list_size;
				in >> list_size;
				if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32)*list_size)
				{
					QString("wrong size with the main ident: %1, sub ident: %2").arg(mainCodeType).arg(subCodeType);
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
					emit error(QString("remaining data: %1, subIdent: %2").arg(mainCodeType).arg(subCodeType));
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
					QString("wrong size with the main ident: %1, sub ident: %2").arg(mainCodeType).arg(subCodeType);
					return;
				}
				QList<quint32> player_ids;
				quint32 id;
				quint16 list_size;
				in >> list_size;
				if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32)*list_size)
				{
					QString("wrong size with the main ident: %1, sub ident: %2").arg(mainCodeType).arg(subCodeType);
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
					emit error(QString("remaining data: %1, subIdent: %2").arg(mainCodeType).arg(subCodeType));
					return;
				}
				emit removePlayersInformationToWatch(player_ids);
				return;
			}
			break;
			default:
				emit error(QString("ident: %1, unknow sub ident: %2").arg(mainCodeType).arg(subCodeType));
			break;
		}
		break;
		default:
			emit error("unknow main ident: "+QString::number(mainCodeType));
		break;
	}
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

void ClientNetworkRead::send_player_informations()
{
	is_logged=true;
}

/// \warning it need be complete protocol trame
void ClientNetworkRead::fake_receive_data(const QByteArray &data)
{
	parseInputAfterLogin(data);
}
