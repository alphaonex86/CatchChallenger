#include "Api_protocol.h"

#ifdef Q_CC_GNU
//this next header is needed to change file time/date under gcc
#include <utime.h>
#include <sys/stat.h>
#endif

//need host + port here to have datapack base

Api_protocol::Api_protocol(QAbstractSocket *socket) :
	ProtocolParsingInput(socket,PacketModeTransmission_Client)
{
	output=new ProtocolParsingOutput(socket,PacketModeTransmission_Client);

	resetAll();
}

Api_protocol::~Api_protocol()
{
	delete output;
}

//have message without reply
void Api_protocol::parseMessage(const quint8 &mainCodeType,const QByteArray &data)
{
	QDataStream in(data);
	in.setVersion(QDataStream::Qt_4_4);
	switch(mainCodeType)
	{
		case 0xC0:
		{
			if((in.device()->size()-in.device()->pos())<(int)sizeof(quint16))
			{
				emit newError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1").arg(mainCodeType));
				return;
			}
			quint16 actionListSize;
			in >> actionListSize;
			if(actionListSize==0)
				break;
			int index=0;
			quint32 player_id;
			quint8 type;
			while(index<actionListSize)
			{
				if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)+sizeof(quint8)))
				{
					emit newError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1").arg(mainCodeType));
					return;
				}
				in >> type;
				in >> player_id;
				switch(type)
				{
					case 0x01:
					{
						if(!checkStringIntegrity(data.right(data.size()-in.device()->pos())))
						{
							emit newError(tr("Procotol wrong or corrupted"),QString("wrong text with main ident: %1").arg(mainCodeType));
							return;
						}
						QString mapName;
						in >> mapName;
						quint16 x,y,speed;
						quint8 direction;
						if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint16)*2+sizeof(quint8)+sizeof(quint16)))
						{
							emit newError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1").arg(mainCodeType));
							return;
						}
						in >> x;
						in >> y;
						in >> direction;
						in >> speed;
						if(direction<1 || direction>8)
						{
							emit newError(tr("Procotol wrong or corrupted"),QString("direction have wrong value: %1, at main ident: %2").arg(direction).arg(mainCodeType));
							return;
						}
						DebugClass::debugConsole(
									QString("player_id: %1, mapName %2, x: %3, y: %4, direction: %5")
									.arg(player_id)
									.arg(mapName)
									.arg(x)
									.arg(y)
									.arg(direction)
									 );
						emit insert_player(player_id,mapName,x,y,direction,speed);
					}
					break;
					case 0x02:
					{
						if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
						{
							emit newError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1").arg(mainCodeType));
							return;
						}
						quint8 list_size;
						in >> list_size;
						if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)*2*list_size))
						{
							emit newError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1").arg(mainCodeType));
							return;
						}
						QList<QPair<quint8,quint8> > movement;
						QPair<quint8,quint8> pair;
						int indexMove=0;
						while(indexMove<list_size)
						{
							in >> pair.first;
							in >> pair.second;
							/*DebugClass::debugConsole(
								QString("player_id: %1, move %2, direction: %3")
								.arg(player_id)
								.arg(pair.first)
								.arg(pair.second)
									 );*/
							movement << pair;
							indexMove++;
						}
						emit move_player(player_id,movement);
					}
					break;
					case 0x03:
						DebugClass::debugConsole(
								QString("player_id: %1 remove")
								.arg(player_id)
								 );
						emit remove_player(player_id);
					break;
					default:
					emit newError(tr("Procotol wrong or corrupted"),QString("wrong type code with main ident: %1").arg(mainCodeType));
					return;
				}
				index++;
			}
		}
		break;
		default:
			emit newError(tr("Procotol wrong or corrupted"),QString("unknow ident reply code: %1").arg(mainCodeType));
			return;
		break;
	}
}

void Api_protocol::parseMessage(const quint8 &mainCodeType,const quint16 &subCodeType,const QByteArray &data)
{
	QDataStream in(data);
	in.setVersion(QDataStream::Qt_4_4);
	switch(mainCodeType)
	{
		case 0xC2:
		{
			switch(subCodeType)
			{
				//update of the player info
				case 0x0002:
				{
					if((in.device()->size()-in.device()->pos())<(int)sizeof(quint16))
					{
						emit newError(tr("Procotol wrong or corrupted"),QString("wrong size for list size with main ident: %1, subCodeType: %2").arg(mainCodeType).arg(subCodeType));
						return;
					}
					quint16 list_size;
					in >> list_size;
					QList<Player_public_informations> player_informations_list;
					int index=0;
					while(index<list_size)
					{
						Player_public_informations temp;
						if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32)*2)
						{
							emit newError(tr("Procotol wrong or corrupted"),QString("wrong size for id with main ident: %1, subCodeType: %2").arg(mainCodeType).arg(subCodeType));
							return;
						}
						in >> temp.id;
						if(!checkStringIntegrity(data.right(data.size()-in.device()->pos())))
						{
							emit newError(tr("Procotol wrong or corrupted"),QString("wrong string for pseudo with main ident: %1, subCodeType: %2").arg(mainCodeType).arg(subCodeType));
							return;
						}
						in >> temp.pseudo;
						if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
						{
							emit newError(tr("Procotol wrong or corrupted"),QString("wrong size for pseudo with main ident: %1, subCodeType: %2").arg(mainCodeType).arg(subCodeType));
							return;
						}
						if(!checkStringIntegrity(data.right(data.size()-in.device()->pos())))
						{
							emit newError(tr("Procotol wrong or corrupted"),QString("wrong string for description with main ident: %1, subCodeType: %2").arg(mainCodeType).arg(subCodeType));
							return;
						}
						in >> temp.description;
						if((in.device()->size()-in.device()->pos())<=(int)(sizeof(quint16)+sizeof(quint8)))
						{
							emit newError(tr("Procotol wrong or corrupted"),QString("wrong size for clan and status with main ident: %1, subCodeType: %2").arg(mainCodeType).arg(subCodeType));
							return;
						}
						in >> temp.clan;
						quint8 status;
						in >> status;
						temp.type = (Player_type)status;
						if(!checkStringIntegrity(data.right(data.size()-in.device()->pos())))
						{
							emit newError(tr("Procotol wrong or corrupted"),QString("missing skin variable: %1, subCodeType: %2").arg(mainCodeType).arg(subCodeType));
							return;
						}
						in >> temp.skin;

						if(temp.id==player_id)
						{

							if((in.device()->size()-in.device()->pos())<=(int)(sizeof(quint32)))
							{
								emit newError(tr("Procotol wrong or corrupted"),QString("wrong size for clan and status with main ident: %1, subCodeType: %2").arg(mainCodeType).arg(subCodeType));
								return;
							}
							in >> player_informations.cash;
							player_informations.public_informations=temp;
							emit have_current_player_info(player_informations);
						}
						this->player_informations_list << player_informations_list;
						player_informations_list << temp;

						index++;
					}
					if(in.device()->size()!=in.device()->pos())
					{
						emit newError(tr("Procotol wrong or corrupted"),QString("remaining data with main ident: %1, subCodeType: %2").arg(mainCodeType).arg(subCodeType));
						return;
					}
					emit new_player_info(player_informations_list);
				}
				break;
				//file as input
				case 0x0003:
				{
					if(!checkStringIntegrity(data.right(data.size()-in.device()->pos())))
					{
						emit newError(tr("Procotol wrong or corrupted"),QString("wrong string for file name with main ident: %1, subCodeType: %2").arg(mainCodeType).arg(subCodeType));
						return;
					}
					QString fileName;
					in >> fileName;

					if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
					{
						emit newError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType: %2").arg(mainCodeType).arg(subCodeType));
						return;
					}
					qint32 file_size;
					in >> file_size;
					if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
					{
						emit newError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType: %2").arg(mainCodeType).arg(subCodeType));
						return;
					}
					quint32 mtime;
					in >> mtime;
					QDateTime date;
					date.setTime_t(mtime);
					DebugClass::debugConsole(QString("write the file: %1, with date: %2").arg(fileName).arg(date.toString("dd.MM.yyyy hh:mm:ss.zzz")));
					QByteArray data=data.right(data.size()-in.device()->pos());
					if(data.size()!=file_size)
					{
						emit newError(tr("Procotol wrong or corrupted"),QString("wrong size of the file (size != data.size()) with main ident: %1, subCodeType: %2").arg(mainCodeType).arg(subCodeType));
						return;
					}
					emit haveNewFile(fileName,data,mtime);
				}
				break;
				//chat as input
				case 0x0005:
				{
					if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)+sizeof(quint8)))
					{
						emit newError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType: %2").arg(mainCodeType).arg(subCodeType));
						return;
					}
					quint32 player_id;
					quint8 chat_type;
					in >> player_id;
					in >> chat_type;
					if(!checkStringIntegrity(data.right(data.size()-in.device()->pos())))
					{
						emit newError(tr("Procotol wrong or corrupted"),QString("wrong text with main ident: %1, subCodeType: %2").arg(mainCodeType).arg(subCodeType));
						return;
					}
					QString text;
					in >> text;
					emit new_chat_text(player_id,chat_type,text);
				}
				break;
				//kicked/ban and reason
				case 0x0008:
				{
					if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
					{
						emit newError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType: %2").arg(mainCodeType).arg(subCodeType));
						return;
					}
					quint8 code;
					in >> code;
					if(!checkStringIntegrity(data.right(data.size()-in.device()->pos())))
					{
						emit newError(tr("Procotol wrong or corrupted"),QString("wrong string for reason with main ident: %1, subCodeType: %2").arg(mainCodeType).arg(subCodeType));
						return;
					}
					QString text;
					in >> text;
					switch(code)
					{
						case 0x01:
							emit newError(tr("Disconnected by the server"),QString("You have been kicked by the server with the reason: %1").arg(text));
						return;
						case 0x02:
							emit newError(tr("Disconnected by the server"),QString("You have been ban by the server with the reason: %1").arg(text));
						return;
						case 0x03:
							emit newError(tr("Disconnected by the server"),QString("You have been disconnected by the server with the reason: %1").arg(text));
						return;
						default:
							emit newError(tr("Disconnected by the server"),QString("You have been disconnected by strange way by the server with the reason: %1").arg(text));
						return;
					}
				}
				break;
				//Player number
				case 0x0009:
				{
					if((in.device()->size()-in.device()->pos())<(int)sizeof(quint16)*2)
					{
						emit newError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType: %2").arg(mainCodeType).arg(subCodeType));
						return;
					}
					quint16 current,max;
					in >> current;
					in >> max;
					emit number_of_player(current,max);
				}
				break;
				default:
				emit newError(tr("Procotol wrong or corrupted"),QString("unknow subCodeType reply code: %1, subCodeType: %2").arg(mainCodeType).arg(subCodeType));
			}
		}
		break;
		default:
			emit newError(tr("Procotol wrong or corrupted"),QString("unknow ident reply code: %1").arg(mainCodeType));
			return;
		break;
	}
}

//have query with reply
void Api_protocol::parseQuery(const quint8 &mainCodeType,const quint8 &queryNumber,const QByteArray &data)
{
	Q_UNUSED(mainCodeType);
	Q_UNUSED(queryNumber);
	Q_UNUSED(data);
	emit newError(tr("Procotol wrong or corrupted"),"have not query of this type");
}

void Api_protocol::parseQuery(const quint8 &mainCodeType,const quint16 &subCodeType,const quint8 &queryNumber,const QByteArray &data)
{
	Q_UNUSED(mainCodeType);
	Q_UNUSED(subCodeType);
	Q_UNUSED(queryNumber);
	Q_UNUSED(data);
	emit newError(tr("Procotol wrong or corrupted"),"have not query of this type");
}

//send reply
void Api_protocol::parseReplyData(const quint8 &mainCodeType,const quint8 &queryNumber,const QByteArray &data)
{
	Q_UNUSED(mainCodeType);
	Q_UNUSED(queryNumber);
	Q_UNUSED(data);
	emit newError(tr("Procotol wrong or corrupted"),"have not query of this type");
}

void Api_protocol::parseReplyData(const quint8 &mainCodeType,const quint16 &subCodeType,const quint8 &queryNumber,const QByteArray &data)
{
	QDataStream in(data);
	in.setVersion(QDataStream::Qt_4_4);
	switch(mainCodeType)
	{
		case 0x02:
		{
			//local the query number to get the type
			switch(subCodeType)
			{
				//Protocol initialization
				case 0x0001:
				{
					if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
					{
						emit newError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, and queryNumber: %2, type: query_type_protocol").arg(mainCodeType).arg(queryNumber));
						return;
					}
					quint8 returnCode;
					in >> returnCode;
					if(returnCode==0x01)
					{
						have_send_protocol=true;
						emit protocol_is_good();
						DebugClass::debugConsole("the protocol is good");
					}
					else if(returnCode==0x02)
					{
						emit newError(tr("Procotol wrong or corrupted"),QString("the server have returned: protocol wrong"));
						return;
					}
					else if(returnCode==0x03)
					{
						if(!checkStringIntegrity(data.right(data.size()-in.device()->pos())))
						{
							emit newError(tr("Procotol wrong or corrupted"),QString("wrong text with main ident: %1").arg(mainCodeType));
							return;
						}
						QString string;
						in >> string;
						DebugClass::debugConsole("disconnect with reason: "+string);
						emit disconnected(string);
						return;
					}
					else
					{
						emit newError(tr("Procotol wrong or corrupted"),QString("bad return code: %1").arg(returnCode));
						return;
					}
				}
				break;
				//Get first data and send the login
				case 0x0002:
				{
					if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
					{
						emit newError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, and queryNumber: %2").arg(mainCodeType).arg(queryNumber));
						return;
					}
					quint8 returnCode;
					in >> returnCode;
					if(returnCode==0x01)
					{
						if(!checkStringIntegrity(data.right(data.size()-in.device()->pos())))
						{
							emit newError(tr("Procotol wrong or corrupted"),QString("wrong text with main ident: %1").arg(mainCodeType));
							return;
						}
						QString string;
						in >> string;
						DebugClass::debugConsole("is not logged, reason: "+string);
						emit notLogged(string);
						return;
					}
					else if(returnCode==0x02)
					{
						if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
						{
							emit newError(tr("Procotol wrong or corrupted"),QString("wrong size to get the player id"));
							return;
						}
						in >> player_id;
						is_logged=true;
						DebugClass::debugConsole("is logged with id: "+QString::number(player_id));
						emit logged();
					}
					else
					{
						emit newError(tr("Procotol wrong or corrupted"),QString("bad return code: %1").arg(returnCode));
						return;
					}
				}
				break;
				default:
				break;
			}
		}
		break;
		default:
			emit newError(tr("Procotol wrong or corrupted"),QString("unknow ident reply code: %1").arg(mainCodeType));
			return;
		break;
	}
}

QList<Player_public_informations> Api_protocol::get_player_informations_list()
{
	return player_informations_list;
}

Player_private_and_public_informations Api_protocol::get_player_informations()
{
	return player_informations;
}

QString Api_protocol::getPseudo()
{
	return player_informations.public_informations.pseudo;
}

quint32 Api_protocol::getId()
{
	return player_informations.public_informations.id;
}

quint8 Api_protocol::queryNumber()
{
	if(lastQueryNumber>=254)
		lastQueryNumber=1;
	return lastQueryNumber++;
}

bool Api_protocol::sendProtocol()
{
	if(have_send_protocol)
	{
		emit newError(tr("Internal problem"),QString("Have already send the protocol"));
		return false;
	}
	have_send_protocol=true;
	QByteArray outputData;
	QDataStream out(&outputData, QIODevice::WriteOnly);
	out.setVersion(QDataStream::Qt_4_4);
	quint8 query_number=queryNumber();
	out << QString(PROTOCOL_HEADER);
	output->packOutcommingQuery(0x02,0x0001,query_number,outputData);
	return true;
}

bool Api_protocol::tryLogin(QString login,QString pass)
{
	if(!have_send_protocol)
	{
		emit newError(tr("Internal problem"),QString("Have not send the protocol"));
		return false;
	}
	if(is_logged)
	{
		emit newError(tr("Internal problem"),QString("Is already logged"));
		return false;
	}
	QByteArray outputData;
	QDataStream out(&outputData, QIODevice::WriteOnly);
	out.setVersion(QDataStream::Qt_4_4);
	quint8 query_number=queryNumber();
	out << login;
	QCryptographicHash hash(QCryptographicHash::Sha1);
	hash.addData(pass.toUtf8());
	outputData+=hash.result();
	output->packOutcommingQuery(0x02,0x0002,query_number,outputData);
	return true;
}

void Api_protocol::send_player_move(quint8 moved_unit,quint8 direction)
{
	if(direction<1 || direction>8)
	{
		DebugClass::debugConsole(QString("direction given wrong: %1").arg(direction));
		return;
	}
	QByteArray outputData;
	QDataStream out(&outputData, QIODevice::WriteOnly);
	out.setVersion(QDataStream::Qt_4_4);
	out << moved_unit;
	out << direction;
	output->packOutcommingData(0x40,outputData);
}

void Api_protocol::sendChatText(Chat_type chatType,QString text)
{
	if((chatType<2 || chatType>5) && chatType!=6)
	{
		DebugClass::debugConsole("chatType wrong: "+QString::number(chatType));
		return;
	}
	QByteArray outputData;
	QDataStream out(&outputData, QIODevice::WriteOnly);
	out.setVersion(QDataStream::Qt_4_4);
	out << chatType;
	out << text;
	output->packOutcommingData(0x42,0x0003,outputData);
	if(!text.startsWith('/'))
		emit new_chat_text(player_id,chatType,text);
}

void Api_protocol::sendPM(QString text,QString pseudo)
{
	emit new_chat_text(player_id,0x06,text);
	if(pseudo==player_informations.public_informations.pseudo)
		return;
	QByteArray outputData;
	QDataStream out(&outputData, QIODevice::WriteOnly);
	out.setVersion(QDataStream::Qt_4_4);
	out << (quint8)0x06;
	out << text;
	out << pseudo;
	output->packOutcommingData(0x42,0x003,outputData);
}

bool Api_protocol::add_player_watching(quint32 id)
{
	QList<quint32> ids;
	ids << id;
	return add_player_watching(ids);
}

bool Api_protocol::remove_player_watching(quint32 id)
{
	QList<quint32> ids;
	ids << id;
	return remove_player_watching(ids);
}

bool Api_protocol::add_player_watching(QList<quint32> ids)
{
	QList<quint32> new_ids;
	int index=0;
	while(index<ids.size())
	{
		if(ids.at(index)!=player_id)
		{
			if(!player_id_watching.contains(ids.at(index)))
				new_ids << ids.at(index);
			player_id_watching << ids.at(index);
		}
		index++;
	}
	if(new_ids.size()>0)
	{
		QByteArray outputData;
		QDataStream out(&outputData, QIODevice::WriteOnly);
		out.setVersion(QDataStream::Qt_4_4);
		out << (quint8)0x01;
		out << (quint16)new_ids.size();
		int index=0;
		while(index<new_ids.size())
		{
			out << new_ids.at(index);
			index++;
		}
		output->packOutcommingData(0x42,0x000A,outputData);
	}
	return true;
}

bool Api_protocol::remove_player_watching(QList<quint32> ids)
{
	QList<quint32> new_ids;
	int index=0;
	while(index<ids.size())
	{
		if(ids.at(index)!=player_id)
		{
			player_id_watching.removeOne(ids.at(index));
			if(!player_id_watching.contains(ids.at(index)))
				new_ids << ids.at(index);
		}
		index++;
	}
	if(new_ids.size()>0)
	{
		QByteArray outputData;
		QDataStream out(&outputData, QIODevice::WriteOnly);
		out.setVersion(QDataStream::Qt_4_4);
		out << (quint16)new_ids.size();
		int index=0;
		while(index<new_ids.size())
		{
			out << new_ids.at(index);
			index++;
		}
		output->packOutcommingData(0x42,0x000B,outputData);
	}
	return true;
}

//to reset all
void Api_protocol::resetAll()
{
	//status for the query
	is_logged=false;
	have_send_protocol=false;

	player_informations_list.clear();
	player_id_watching.clear();

	//to send trame
	lastQueryNumber=1;
}
