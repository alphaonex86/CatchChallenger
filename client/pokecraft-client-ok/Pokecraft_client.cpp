#include "Pokecraft_client.h"

#include <QCryptographicHash>

Pokecraft_client::Pokecraft_client()
{
	settings=new QSettings(QCoreApplication::applicationDirPath()+"/server.conf",QSettings::IniFormat);
	if(settings->contains("host"))
		host=settings->value("host").toString();
	else
	{
		settings->setValue("host","localhost");
		host="localhost";
	}
	if(settings->contains("port"))
		port=settings->value("port").toUInt();
	else
	{
		settings->setValue("port",42489);
		port=42489;
	}
	connect(&socket,SIGNAL(stateChanged(QAbstractSocket::SocketState)),	this,SIGNAL(stateChanged(QAbstractSocket::SocketState)));
	connect(&socket,SIGNAL(error(QAbstractSocket::SocketError)),		this,SIGNAL(error(QAbstractSocket::SocketError)));
	connect(&socket,SIGNAL(readyRead()),					this,SLOT(readyRead()));
	connect(&socket,SIGNAL(disconnected()),					this,SLOT(disconnected()));
	disconnected();
}

Pokecraft_client::~Pokecraft_client()
{
	socket.abort();
	socket.disconnectFromHost();
	if(socket.state()!=QAbstractSocket::UnconnectedState)
		socket.waitForDisconnected();
	delete settings;
}

void Pokecraft_client::dataClear()
{
	haveData=false;
	dataSize=0;
	data.clear();
}

void Pokecraft_client::tryConnect()
{
	DebugClass::debugConsole(QString("Try connect on: %1:%2").arg(host).arg(port));
	socket.connectToHost(host,port);
}

void Pokecraft_client::add_to_query_list(quint8 id,query_type type)
{
	query_send temp;
	temp.id=id;
	temp.type=type;
	query_list << temp;
}

quint8 Pokecraft_client::queryNumber()
{
	if(lastQueryNumber>=254)
		lastQueryNumber=1;
	return lastQueryNumber++;
}

QString Pokecraft_client::errorString()
{
	return error_string;
}

void Pokecraft_client::sendData(QByteArray data)
{
	QByteArray block;
	QDataStream out(&block, QIODevice::WriteOnly);
	out << qint32(data.size()+sizeof(qint32));
	block+=data;
	socket.write(block);
}

void Pokecraft_client::readyRead()
{
	QTcpSocket *socket=qobject_cast<QTcpSocket *>(QObject::sender());
	if(socket==NULL)
	{
		errorOutput(tr("Unlocated client socket"));
		return;
	}
	while(socket->bytesAvailable()>0)
	{
		if(!haveData)
		{
			if(socket->bytesAvailable()<(int)sizeof(int))
			{
				errorOutput(tr("Bytes available is not sufficient to do a int"));
				return;
			}
			QDataStream in(socket);
			in.setVersion(QDataStream::Qt_4_4);
			in >> dataSize;
			dataSize-=sizeof(int);
			if(dataSize>256*1024) // 256KB
			{
				errorOutput(tr("Reply size is >256KB, seam corrupted"));
				return;
			}
			haveData=true;
		}
		if(dataSize<(data.size()+socket->bytesAvailable()))
			data.append(socket->read(dataSize-data.size()));
		else
			data.append(socket->readAll());
		if(dataSize==(quint32)data.size())
		{
			parseInput(data);
			dataClear();
		}
	}
}

void Pokecraft_client::disconnected()
{
	lastQueryNumber=1;
	dataClear();
	resetAll();
}

bool Pokecraft_client::checkStringIntegrity(QByteArray data)
{
	if(data.size()<(int)sizeof(qint32))
	{
		errorOutput(tr("header size not suffisient"));
		return false;
	}
	qint32 stringSize;
	QDataStream in(data);
	in.setVersion(QDataStream::Qt_4_4);
	in >> stringSize;
	if(stringSize>65535)
	{
		errorOutput(tr("String size is wrong"));
		return false;
	}
	if(data.size()<stringSize)
	{
		errorOutput(tr("String size is greater than the data: %1>%2").arg(data.size()).arg(stringSize));
		return false;
	}
	return true;
}

void Pokecraft_client::errorOutput(QString error,QString detailedError)
{
	error_string=error;
	emit haveNewError();
	DebugClass::debugConsole("User message: "+error);
	socket.disconnectFromHost();
	if(!detailedError.isEmpty())
		DebugClass::debugConsole(detailedError);
}

void Pokecraft_client::parseInput(QByteArray inputData)
{
	quint8 mainIdent;
	quint8 queryNumber;
	QDataStream in(inputData);
	in.setVersion(QDataStream::Qt_4_4);
	if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
	{
		errorOutput(tr("wrong size at parse input"));
		return;
	}
	in >> mainIdent;
	QByteArray outputData;
	QDataStream out(&outputData, QIODevice::WriteOnly);
	out.setVersion(QDataStream::Qt_4_4);
	if(mainIdent!=0xC1)
	{
		if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
		{
			errorOutput(tr("wrong size to get the main ident"));
			return;
		}
		if(!have_send_protocol)
		{
			errorOutput("have not send the protocol");
			return;
		}
		else if(!is_logged)
		{
			errorOutput("have not send the login");
			return;
		}
	}
	switch(mainIdent)
	{
		case 0xC0:
		{
			if((in.device()->size()-in.device()->pos())<(int)sizeof(quint16))
			{
				errorOutput(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1").arg(mainIdent));
				return;
			}
			quint16 actionListSize;
			in >> actionListSize;
			if(actionListSize==0)
				return;
			int index=0;
			quint32 player_id;
			quint8 type;
			while(index<actionListSize)
			{
				if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)+sizeof(quint8)))
				{
					errorOutput(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1").arg(mainIdent));
					return;
				}
				in >> player_id;
				in >> type;
				switch(type)
				{
					case 0x01:
					{
						if(!checkStringIntegrity(inputData.right(inputData.size()-in.device()->pos())))
							return;
						QString mapName;
						in >> mapName;
						quint16 x,y;
						quint8 direction;
						if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint16)*2+sizeof(quint8)))
						{
							errorOutput(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1").arg(mainIdent));
							return;
						}
						in >> x;
						in >> y;
						in >> direction;
						if(direction<1 || direction>8)
						{
							errorOutput(tr("Procotol wrong or corrupted"),QString("direction have wrong value: %1, at main ident: %2").arg(direction).arg(mainIdent));
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
						emit insert_player(player_id,mapName,x,y,direction);
					}
					break;
					case 0x02:
					{
						if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
						{
							errorOutput(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1").arg(mainIdent));
							return;
						}
						quint8 list_size;
						in >> list_size;
						if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)*2*list_size))
						{
							errorOutput(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1").arg(mainIdent));
							return;
						}
						QList<QPair<quint8,quint8> > movement;
						QPair<quint8,quint8> pair;
						int indexMove=0;
						while(indexMove<list_size)
						{
							in >> pair.first;
							in >> pair.second;
							DebugClass::debugConsole(
								QString("player_id: %1, move %2, direction: %3")
								.arg(player_id)
								.arg(pair.first)
								.arg(pair.second)
									 );
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
					errorOutput(tr("Procotol wrong or corrupted"),QString("wrong type code with main ident: %1").arg(mainIdent));
					return;
				}
				index++;
			}
		}
		break;
		case 0xC1:
		{
			if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
			{
				errorOutput(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1").arg(mainIdent));
				return;
			}
			in >> queryNumber;
			//local the query number to get the type
			switch(remove_to_query_list(queryNumber))
			{
				case query_type_protocol:
				{
					if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
					{
						errorOutput(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, and queryNumber: %2, type: query_type_protocol").arg(mainIdent).arg(queryNumber));
						return;
					}
					quint8 returnCode;
					in >> returnCode;
					if(returnCode==0x01)
					{
						have_send_protocol=true;
						emit protocol_is_good();
						DebugClass::debugConsole("the protocol is good");
						return;
					}
					else if(returnCode==0x02)
					{
						errorOutput(tr("protocol wrong"));
						return;
					}
					else if(returnCode==0x03)
					{
						if(!checkStringIntegrity(inputData.right(inputData.size()-in.device()->pos())))
							return;
						QString string;
						in >> string;
						DebugClass::debugConsole("disconnect with reason: "+string);
						emit disconnected(string);
						return;
					}
					else
					{
						errorOutput(tr("Procotol wrong or corrupted"),QString("bad return code: %1").arg(returnCode));
						return;
					}
				}
				case query_type_login:
				{
					if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)+sizeof(quint32)))
					{
						errorOutput(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, and queryNumber: %2").arg(mainIdent).arg(queryNumber));
						return;
					}
					quint8 returnCode;
					in >> returnCode;
					if(returnCode==0x01)
					{
						if(!checkStringIntegrity(inputData.right(inputData.size()-in.device()->pos())))
							return;
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
							errorOutput(tr("wrong size to get the player id"));
							return;
						}
						in >> player_id;
						is_logged=true;
						DebugClass::debugConsole("is logged with id: "+QString::number(player_id));
						emit logged();
						return;
					}
					else
					{
						errorOutput(tr("Procotol wrong or corrupted"),QString("bad return code: %1").arg(returnCode));
						return;
					}
				}
				break;
				case query_type_file:
				{
					quint8 reply_code;
					int index=0;
					while(index<query_files_list.size())
					{
						if(query_files_list.at(index).id==queryNumber)
						{
							int index_files=0;
							while(index_files<query_files_list.at(index).filesName.size())
							{
								if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
								{
									errorOutput(tr("wrong size to get the player id"));
									return;
								}
								in >> reply_code;
								if(reply_code==0x01)
									emit filesIsAlreadyInCache(query_files_list.at(index).filesName.at(index_files));
								else if(reply_code==0x03)
									emit filesIsInError(query_files_list.at(index).filesName.at(index_files));
								index_files++;
							}
							return;
						}
						index++;
					}
				}
				break;
				default:
				break;
			}
		}
		break;
		case 0xC2:
		{
			if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
			{
				errorOutput(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1").arg(mainIdent));
				return;
			}
			quint32 subIdent;
			in >> subIdent;
			switch(subIdent)
			{
				//update of the player info
				case 0x00000002:
				{
					if((in.device()->size()-in.device()->pos())<=(int)sizeof(quint16))
					{
						errorOutput(tr("Procotol wrong or corrupted"),QString("wrong size for list size with main ident: %1, subIdent: %2").arg(mainIdent).arg(subIdent));
						return;
					}
					quint16 list_size;
					in >> list_size;
					QList<Player_informations> player_informations_list;
					int index=0;
					while(index<list_size)
					{
						Player_informations temp;
						if((in.device()->size()-in.device()->pos())<=(int)sizeof(quint32)*2)
						{
							errorOutput(tr("Procotol wrong or corrupted"),QString("wrong size for id with main ident: %1, subIdent: %2").arg(mainIdent).arg(subIdent));
							return;
						}
						in >> temp.id;
						if(!checkStringIntegrity(inputData.right(inputData.size()-in.device()->pos())))
							return;
						in >> temp.pseudo;
						if((in.device()->size()-in.device()->pos())<=(int)sizeof(quint32))
						{
							errorOutput(tr("Procotol wrong or corrupted"),QString("wrong size for pseudo with main ident: %1, subIdent: %2").arg(mainIdent).arg(subIdent));
							return;
						}
						if(!checkStringIntegrity(inputData.right(inputData.size()-in.device()->pos())))
							return;
						in >> temp.description;
						if((in.device()->size()-in.device()->pos())<=(int)(sizeof(quint16)+sizeof(quint8)+sizeof(quint32)))
						{
							errorOutput(tr("Procotol wrong or corrupted"),QString("wrong size for description with main ident: %1, subIdent: %2").arg(mainIdent).arg(subIdent));
							return;
						}
						in >> temp.clan;
						in >> temp.status;
						if(!checkStringIntegrity(inputData.right(inputData.size()-in.device()->pos())))
							return;
						in >> temp.skin;
						player_informations_list << temp;
						index++;
					}
					if(in.device()->size()!=in.device()->pos())
					{
						errorOutput(tr("Procotol wrong or corrupted"),QString("remaining data with main ident: %1, subIdent: %2").arg(mainIdent).arg(subIdent));
						return;
					}
					this->player_informations_list << player_informations_list;
					index=0;
					while(index<list_size)
					{
						if(player_informations_list.at(index).id==player_id)
						{
							player_informations=player_informations_list.at(index);
							emit have_current_player_info();
							break;
						}
						index++;
					}
					emit new_player_info();
				}
				break;
				//file as input
				case 0x00000003:
				{
					if(!checkStringIntegrity(inputData.right(inputData.size()-in.device()->pos())))
						return;
					QString fileName;
					in >> fileName;
					if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
					{
						errorOutput(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subIdent: %2").arg(mainIdent).arg(subIdent));
						return;
					}
					quint32 file_size;
					in >> file_size;
					QByteArray data=inputData.right(inputData.size()-in.device()->pos());
					emit filesContent(fileName,data);
				}
				break;
				//chat as input
				case 0x00000005:
				{
					if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)+sizeof(quint8)))
					{
						errorOutput(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subIdent: %2").arg(mainIdent).arg(subIdent));
						return;
					}
					quint32 player_id;
					quint8 chat_type;
					in >> player_id;
					in >> chat_type;
					if(!checkStringIntegrity(inputData.right(inputData.size()-in.device()->pos())))
						return;
					QString text;
					in >> text;
					emit new_chat_text(player_id,chat_type,text);
				}
				break;
				//Player number
				case 0x00000009:
				{
					if((in.device()->size()-in.device()->pos())<(int)sizeof(quint16)*2)
					{
						errorOutput(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subIdent: %2").arg(mainIdent).arg(subIdent));
						return;
					}
					quint16 current,max;
					in >> current;
					in >> max;
					emit number_of_player(current,max);
				}
				break;
				default:
				errorOutput(tr("Procotol wrong or corrupted"),QString("unknow subIdent reply code: %1, subIdent: %2").arg(mainIdent).arg(subIdent));
			}
		}
		break;
		default:
			errorOutput(tr("Procotol wrong or corrupted"),QString("unknow ident reply code: %1").arg(mainIdent));
			return;
		break;
	}
}

Pokecraft_client::query_type Pokecraft_client::remove_to_query_list(quint8 id)
{
	int index=0;
	while(index<query_list.size())
	{
		if(query_list.at(index).id==id)
		{
			query_type temp_type=query_list.at(index).type;
			query_list.removeAt(index);
			return temp_type;
		}
		index++;
	}
	return query_type_other;
}

void Pokecraft_client::sendProtocol()
{
	if(socket.state()!=QAbstractSocket::ConnectedState)
	{
		errorOutput(tr("Not connected"));
		return;
	}
	if(have_send_protocol)
	{
		errorOutput(tr("Have already send the protocol"));
		return;
	}
	QByteArray outputData;
	QDataStream out(&outputData, QIODevice::WriteOnly);
	out.setVersion(QDataStream::Qt_4_4);
	quint8 query_number=queryNumber();
	out << (quint8)0x01;
	out << (quint8)query_number;
	out << QString("pkmn-0.0.0.1");
	sendData(outputData);
	add_to_query_list(query_number,query_type_protocol);
}

void Pokecraft_client::tryLogin(QString login,QString pass)
{
	if(socket.state()!=QAbstractSocket::ConnectedState)
	{
		errorOutput(tr("Not connected"));
		return;
	}
	if(!have_send_protocol)
	{
		errorOutput(tr("Have not send the protocol"));
		return;
	}
	if(is_logged)
	{
		errorOutput(tr("Is already logged"));
		return;
	}
	QByteArray outputData;
	QDataStream out(&outputData, QIODevice::WriteOnly);
	out.setVersion(QDataStream::Qt_4_4);
	quint8 query_number=queryNumber();
	out << (quint8)0x03;
	out << (quint8)query_number;
	out << login;
	QCryptographicHash hash(QCryptographicHash::Sha1);
	hash.addData(pass.toUtf8());
	outputData+=hash.result();
	sendData(outputData);
	add_to_query_list(query_number,query_type_login);
}

QList<Player_informations> Pokecraft_client::get_player_informations_list()
{
	return player_informations_list;
}

void Pokecraft_client::not_needed_player_informations(quint32 id)
{
	if(player_id==id)
	{
		errorOutput(tr("Can't remove the current player id"));
		return;
	}
}

Player_informations Pokecraft_client::get_player_informations()
{
	return player_informations;
}

void Pokecraft_client::send_player_move(quint8 moved_unit,quint8 direction)
{
	if(direction<1 || direction>8)
	{
		DebugClass::debugConsole(QString("direction given wrong: %1").arg(direction));
		return;
	}
	QByteArray outputData;
	QDataStream out(&outputData, QIODevice::WriteOnly);
	out.setVersion(QDataStream::Qt_4_4);
	out << (quint8)0x40;
	out << moved_unit;
	out << direction;
	sendData(outputData);
}

void Pokecraft_client::sendChatText(quint8 chatType,QString text)
{
	if((chatType<2 || chatType>5) && chatType!=6)
	{
		DebugClass::debugConsole("chatType wrong: "+QString::number(chatType));
		return;
	}
	QByteArray outputData;
	QDataStream out(&outputData, QIODevice::WriteOnly);
	out.setVersion(QDataStream::Qt_4_4);
	out << (quint8)0x42;
	out << (quint32)0x00000003;
	out << chatType;
	out << text;
	sendData(outputData);
	if(!text.startsWith('/'))
		emit new_chat_text(player_id,chatType,text);
}

void Pokecraft_client::sendPM(QString text,QString pseudo)
{
	emit new_chat_text(player_id,0x06,text);
	if(pseudo==player_informations.pseudo)
		return;
	QByteArray outputData;
	QDataStream out(&outputData, QIODevice::WriteOnly);
	out.setVersion(QDataStream::Qt_4_4);
	out << (quint8)0x42;
	out << (quint32)0x00000003;
	out << (quint8)0x06;
	out << text;
	out << pseudo;
	sendData(outputData);
}

void Pokecraft_client::add_player_watching(quint32 id)
{
	QList<quint32> ids;
	ids << id;
	add_player_watching(ids);
}

void Pokecraft_client::remove_player_watching(quint32 id)
{
	QList<quint32> ids;
	ids << id;
	remove_player_watching(ids);
}

void Pokecraft_client::add_player_watching(QList<quint32> ids)
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
		out << (quint8)0x42;
		out << (quint32)0x0000000A;
		out << (quint8)0x01;
		out << (quint16)new_ids.size();
		int index=0;
		while(index<new_ids.size())
		{
			out << new_ids.at(index);
			index++;
		}
		sendData(outputData);
	}
}

void Pokecraft_client::remove_player_watching(QList<quint32> ids)
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
		out << (quint8)0x42;
		out << (quint32)0x0000000B;
		out << (quint16)new_ids.size();
		int index=0;
		while(index<new_ids.size())
		{
			out << new_ids.at(index);
			index++;
		}
		sendData(outputData);
	}
}

int Pokecraft_client::indexOfPlayerInformations(quint32 id)
{
	int index=0;
	while(index<player_informations_list.size())
	{
		if(player_informations_list.at(index).id==id)
			return index;
		index++;
	}
	DebugClass::debugConsole(QString("id player info not found: %1").arg(id));
	return -1;
}

void Pokecraft_client::resetAll()
{
	player_id_watching.clear();
	player_informations_list.clear();
	is_logged=false;
	have_send_protocol=false;
	query_list.clear();
}

QString Pokecraft_client::getPseudo()
{
	return player_informations.pseudo;
}

void Pokecraft_client::tryDisconnect()
{
	socket.disconnectFromHost();
}

void Pokecraft_client::add_file_watching(QStringList filesName,QList<QByteArray> hashMD5,bool permanent)
{
	if(filesName.size()==0)
	{
		DebugClass::debugConsole(QString("file list size == 0!"));
		return;
	}
	if(filesName.size()!=hashMD5.size())
	{
		DebugClass::debugConsole(QString("file list size != hash list size"));
		return;
	}
	quint8 query_number=queryNumber();
	QByteArray outputData;
	QDataStream out(&outputData, QIODevice::WriteOnly);
	out.setVersion(QDataStream::Qt_4_4);
	out << (quint8)0x02;
	out << (quint32)0x0000000A;
	out << query_number;
	if(permanent)
		out << (quint8)0x01;
	else
		out << (quint8)0x02;
	out << (quint16)filesName.size();
	int index=0;
	while(index<filesName.size())
	{
		if(hashMD5.at(index).size()!=16)
		{
			DebugClass::debugConsole(QString("hash md5 size != 16"));
			return;
		}
		outputData+=hashMD5.at(index);
		out.device()->seek(out.device()->pos()+16);
		if(filesName.at(index).contains("./") || filesName.at(index).contains("\\"))
		{
			DebugClass::debugConsole(QString("file name contains illegale char: %1").arg(filesName.at(index)));
			return;
		}
		if(filesName.at(index).contains(QRegExp("^[a-zA-Z]:/")) || filesName.at(index).startsWith("/"))
		{
			DebugClass::debugConsole(QString("start with wrong string: %1").arg(filesName.at(index)));
			return;
		}
		if(!filesName.at(index).contains(QRegExp("^[0-9/a-zA-Z\\.\\-]+\\.[a-z]{3,4}$")))
		{
			DebugClass::debugConsole(QString("file have not match the regex: %1").arg(filesName.at(index)));
			return;
		}
		out << filesName.at(index);
		index++;
	}
	sendData(outputData);
	add_to_query_list(query_number,query_type_file);
	query_files temp;
	temp.id=query_number;
	temp.filesName=filesName;
	query_files_list << temp;
}

void Pokecraft_client::remove_file_watching(QStringList filesName)
{
	QByteArray outputData;
	QDataStream out(&outputData, QIODevice::WriteOnly);
	out.setVersion(QDataStream::Qt_4_4);
	out << (quint8)0x42;
	out << (quint32)0x0000000A;
	out << (quint16)filesName.size();
	int index=0;
	while(index<filesName.size())
	{
		out << filesName.at(index);
		index++;
	}
	sendData(outputData);
}

QString Pokecraft_client::getHost()
{
	return host;
}

quint16 Pokecraft_client::getPort()
{
	return port;
}
