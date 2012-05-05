#include "Pokecraft_client.h"

#ifdef Q_CC_GNU
//this next header is needed to change file time/date under gcc
#include <utime.h>
#include <sys/stat.h>
#endif

Pokecraft_client::Pokecraft_client()
{
	host="localhost";
	port=42489;
	datapack_base_name=QString("%1/%2-%3/").arg(QApplication::applicationDirPath()).arg(host).arg(port);
	connect(&socket,SIGNAL(stateChanged(QAbstractSocket::SocketState)),	this,SIGNAL(stateChanged(QAbstractSocket::SocketState)));
	connect(&socket,SIGNAL(error(QAbstractSocket::SocketError)),		this,SIGNAL(error(QAbstractSocket::SocketError)));
	connect(&socket,SIGNAL(readyRead()),					this,SLOT(readyRead()));
	connect(&socket,SIGNAL(disconnected()),					this,SLOT(disconnected()));
	disconnected();
	dataClear();
}

Pokecraft_client::~Pokecraft_client()
{
	socket.abort();
	socket.disconnectFromHost();
	if(socket.state()!=QAbstractSocket::UnconnectedState)
		socket.waitForDisconnected();
}

void Pokecraft_client::dataClear()
{
	haveData=false;
	dataSize=0;
	data.clear();
}

void Pokecraft_client::tryConnect(QString host,quint16 port)
{
	DebugClass::debugConsole(QString("Try connect on: %1:%2").arg(host).arg(port));
	this->host=host;
	this->port=port;
	socket.connectToHost(host,port);
	datapack_base_name=QString("%1/%2-%3/").arg(QApplication::applicationDirPath()).arg(host).arg(port);
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
			if(socket->bytesAvailable()<(int)sizeof(int))//ignore because first int is cuted!
			{
				/*errorOutput(tr("Bytes available is not sufficient to do a int: %1").arg(QString(socket->readAll().toHex())));
				dataClear();
				socket->flush();*/
				return;
			}
			QDataStream in(socket);
			in.setVersion(QDataStream::Qt_4_4);
			in >> dataSize;
			dataSize-=sizeof(int);
			if(dataSize>10240*1024) // 10240KB
			{
				errorOutput(tr("Reply size is >10240KB, seam corrupted: %1").arg(dataSize));
				dataClear();
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
			//DebugClass::debugConsole(QString("data: %1").arg(QString(data.toHex())));
			parseInput(data);
			dataClear();
		}
	}
}

void Pokecraft_client::disconnected()
{
	wait_datapack_content=false;
	haveData=false;
	lastQueryNumber=1;
	last_subIdent=0;
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
	//DebugClass::debugConsole(QString("parseInput(): inputData: %1").arg(QString(inputData.toHex())));
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
			if((in.device()->size()-in.device()->pos())<(int)sizeof(quint16))
                        {
                                errorOutput(QString(__FILE__)+":"+QString::number(__LINE__)+": "+QString("have not send the login, mainIdent: %1").arg(mainIdent));
                                return;
			}
                        in >> subIdent;
			if(subIdent!=0x0009)
			{
				errorOutput(QString(__FILE__)+QString::number(__LINE__)+QString("have not send the login, mainIdent: %1, subIdent: %2").arg(mainIdent).arg(subIdent));
				return;
			}
			else
				in.device()->seek(0);
		}
	}
	switch(mainIdent)
	{
		case 0xC0:
		{
			if((in.device()->size()-in.device()->pos())<(int)sizeof(quint16))
			{
				errorOutput(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1").arg(mainIdent)+", "+QString(__FILE__)+":"+QString::number(__LINE__)+": "+QString("\nlast_mainIdent: %1, last_subIdent: %2, last_query: %3").arg(last_mainIdent).arg(last_subIdent).arg(QString(last_query.toHex())));
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
					errorOutput(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1").arg(mainIdent)+", "+QString(__FILE__)+":"+QString::number(__LINE__)+": "+QString("\nlast_mainIdent: %1, last_subIdent: %2, last_query: %3").arg(last_mainIdent).arg(last_subIdent).arg(QString(last_query.toHex())));
					return;
				}
				in >> type;
				in >> player_id;
				switch(type)
				{
					case 0x01:
					{
						if(!checkStringIntegrity(inputData.right(inputData.size()-in.device()->pos())))
						{
							errorOutput(tr("Procotol wrong or corrupted"),QString("wrong text with main ident: %1").arg(mainIdent)+", "+QString(__FILE__)+":"+QString::number(__LINE__)+": "+QString("\nlast_mainIdent: %1, last_subIdent: %2, last_query: %3").arg(last_mainIdent).arg(last_subIdent).arg(QString(last_query.toHex())));
							return;
						}
						QString mapName;
						in >> mapName;
						quint16 x,y,speed;
						quint8 direction;
						if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint16)*2+sizeof(quint8)+sizeof(quint16)))
						{
							errorOutput(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1").arg(mainIdent)+", "+QString(__FILE__)+":"+QString::number(__LINE__)+": "+QString("\nlast_mainIdent: %1, last_subIdent: %2, last_query: %3").arg(last_mainIdent).arg(last_subIdent).arg(QString(last_query.toHex())));
							return;
						}
						in >> x;
						in >> y;
						in >> direction;
						in >> speed;
						if(direction<1 || direction>8)
						{
							errorOutput(tr("Procotol wrong or corrupted"),QString("direction have wrong value: %1, at main ident: %2").arg(direction).arg(mainIdent)+", "+QString(__FILE__)+":"+QString::number(__LINE__)+": "+QString("\nlast_mainIdent: %1, last_subIdent: %2, last_query: %3").arg(last_mainIdent).arg(last_subIdent).arg(QString(last_query.toHex())));
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
							errorOutput(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1").arg(mainIdent)+", "+QString(__FILE__)+":"+QString::number(__LINE__)+": "+QString("\nlast_mainIdent: %1, last_subIdent: %2, last_query: %3").arg(last_mainIdent).arg(last_subIdent).arg(QString(last_query.toHex())));
							return;
						}
						quint8 list_size;
						in >> list_size;
						if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)*2*list_size))
						{
							errorOutput(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1").arg(mainIdent)+", "+QString(__FILE__)+":"+QString::number(__LINE__)+": "+QString("\nlast_mainIdent: %1, last_subIdent: %2, last_query: %3").arg(last_mainIdent).arg(last_subIdent).arg(QString(last_query.toHex())));
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
					errorOutput(tr("Procotol wrong or corrupted"),QString("wrong type code with main ident: %1").arg(mainIdent)+", "+QString(__FILE__)+":"+QString::number(__LINE__)+": "+QString("\nlast_mainIdent: %1, last_subIdent: %2, last_query: %3").arg(last_mainIdent).arg(last_subIdent).arg(QString(last_query.toHex())));
					return;
				}
				index++;
			}
			last_subIdent=0;
		}
		break;
		case 0xC1:
		{
			if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
			{
				errorOutput(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1").arg(mainIdent)+", "+QString(__FILE__)+":"+QString::number(__LINE__)+": "+QString("\nlast_mainIdent: %1, last_subIdent: %2, last_query: %3").arg(last_mainIdent).arg(last_subIdent).arg(QString(last_query.toHex())));
				return;
			}
			in >> queryNumberReturned;
			//local the query number to get the type
			switch(remove_to_query_list(queryNumberReturned))
			{
				case query_type_protocol:
				{
					if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
					{
						errorOutput(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, and queryNumberReturned: %2, type: query_type_protocol").arg(mainIdent).arg(queryNumberReturned)+", "+QString(__FILE__)+":"+QString::number(__LINE__)+": "+QString("\nlast_mainIdent: %1, last_subIdent: %2, last_query: %3").arg(last_mainIdent).arg(last_subIdent).arg(QString(last_query.toHex())));
						return;
					}
					in >> returnCode;
					if(returnCode==0x01)
					{
						have_send_protocol=true;
						emit protocol_is_good();
						DebugClass::debugConsole("the protocol is good");
					}
					else if(returnCode==0x02)
					{
						errorOutput(tr("protocol wrong"));
						return;
					}
					else if(returnCode==0x03)
					{
						if(!checkStringIntegrity(inputData.right(inputData.size()-in.device()->pos())))
						{
							errorOutput(tr("Procotol wrong or corrupted"),QString("wrong text with main ident: %1").arg(mainIdent)+", "+QString(__FILE__)+":"+QString::number(__LINE__)+": "+QString("\nlast_mainIdent: %1, last_subIdent: %2, last_query: %3").arg(last_mainIdent).arg(last_subIdent).arg(QString(last_query.toHex())));
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
						errorOutput(tr("Procotol wrong or corrupted"),QString("bad return code: %1").arg(returnCode)+", "+QString(__FILE__)+":"+QString::number(__LINE__)+": "+QString("\nlast_mainIdent: %1, last_subIdent: %2, last_query: %3").arg(last_mainIdent).arg(last_subIdent).arg(QString(last_query.toHex())));
						return;
					}
				}
				break;
				case query_type_login:
				{
					if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
					{
						errorOutput(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, and queryNumberReturned: %2").arg(mainIdent).arg(queryNumberReturned)+", "+QString(__FILE__)+":"+QString::number(__LINE__)+": "+QString("\nlast_mainIdent: %1, last_subIdent: %2, last_query: %3").arg(last_mainIdent).arg(last_subIdent).arg(QString(last_query.toHex())));
						return;
					}
					quint8 returnCode;
					in >> returnCode;
					if(returnCode==0x01)
					{
						if(!checkStringIntegrity(inputData.right(inputData.size()-in.device()->pos())))
						{
							errorOutput(tr("Procotol wrong or corrupted"),QString("wrong text with main ident: %1").arg(mainIdent)+", "+QString(__FILE__)+":"+QString::number(__LINE__)+": "+QString("\nlast_mainIdent: %1, last_subIdent: %2, last_query: %3").arg(last_mainIdent).arg(last_subIdent).arg(QString(last_query.toHex())));
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
							errorOutput(tr("wrong size to get the player id")+", "+QString(__FILE__)+":"+QString::number(__LINE__)+": "+QString("\nlast_mainIdent: %1, last_subIdent: %2, last_query: %3").arg(last_mainIdent).arg(last_subIdent).arg(QString(last_query.toHex())));
							return;
						}
						in >> player_id;
						is_logged=true;
						DebugClass::debugConsole("is logged with id: "+QString::number(player_id));
						emit logged();
					}
					else
					{
						errorOutput(tr("Procotol wrong or corrupted"),QString("bad return code: %1").arg(returnCode)+", "+QString(__FILE__)+":"+QString::number(__LINE__)+": "+QString("\nlast_mainIdent: %1, last_subIdent: %2, last_query: %3").arg(last_mainIdent).arg(last_subIdent).arg(QString(last_query.toHex())));
						return;
					}
				}
				break;
				case query_type_datapack:
				{
					if((in.device()->size()-in.device()->pos())!=((int)sizeof(quint8))*datapackFilesList.size())
					{
						errorOutput(tr("wrong size to return file list")+", "+QString(__FILE__)+":"+QString::number(__LINE__)+": "+QString("\nlast_mainIdent: %1, last_subIdent: %2, last_query: %3").arg(last_mainIdent).arg(last_subIdent).arg(QString(last_query.toHex())));
						return;
					}
					quint8 reply_code;
					int index=0;
					while(index<datapackFilesList.size())
					{
						in >> reply_code;
						if(reply_code==0x02)
						{
							DebugClass::debugConsole(QString("remove the file: %1").arg(datapackFilesList.at(index)));
							if(!QFile::remove(datapack_base_name+datapackFilesList.at(index)))
								DebugClass::debugConsole(QString("unable to remove: %1").arg(datapackFilesList.at(index)));
						}
						index++;
					}
					datapackFilesList.clear();
					cleanDatapack("");
					emit haveTheDatapack();
				}
				break;
				default:
				break;
			}
			last_subIdent=queryNumberReturned;
		}
		break;
		case 0xC2:
		{
			if((in.device()->size()-in.device()->pos())<(int)sizeof(quint16))
			{
				errorOutput(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1").arg(mainIdent)+", "+QString(__FILE__)+":"+QString::number(__LINE__)+": "+QString("\nlast_mainIdent: %1, last_subIdent: %2, last_query: %3").arg(last_mainIdent).arg(last_subIdent).arg(QString(last_query.toHex())));
				return;
			}
			in >> subIdent;
			switch(subIdent)
			{
				//update of the player info
				case 0x0002:
				{
					if((in.device()->size()-in.device()->pos())<(int)sizeof(quint16))
					{
						errorOutput(tr("Procotol wrong or corrupted"),QString("wrong size for list size with main ident: %1, subIdent: %2").arg(mainIdent).arg(subIdent)+", "+QString(__FILE__)+":"+QString::number(__LINE__)+": "+QString("\nlast_mainIdent: %1, last_subIdent: %2, last_query: %3").arg(last_mainIdent).arg(last_subIdent).arg(QString(last_query.toHex())));
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
							errorOutput(tr("Procotol wrong or corrupted"),QString("wrong size for id with main ident: %1, subIdent: %2").arg(mainIdent).arg(subIdent)+", "+QString(__FILE__)+":"+QString::number(__LINE__)+": "+QString("\nlast_mainIdent: %1, last_subIdent: %2, last_query: %3").arg(last_mainIdent).arg(last_subIdent).arg(QString(last_query.toHex())));
							return;
						}
						in >> temp.id;
						if(!checkStringIntegrity(inputData.right(inputData.size()-in.device()->pos())))
						{
							errorOutput(tr("Procotol wrong or corrupted"),QString("wrong string for pseudo with main ident: %1, subIdent: %2").arg(mainIdent).arg(subIdent)+", "+QString(__FILE__)+":"+QString::number(__LINE__)+": "+QString("\nlast_mainIdent: %1, last_subIdent: %2, last_query: %3").arg(last_mainIdent).arg(last_subIdent).arg(QString(last_query.toHex())));
							return;
						}
						in >> temp.pseudo;
						if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
						{
							errorOutput(tr("Procotol wrong or corrupted"),QString("wrong size for pseudo with main ident: %1, subIdent: %2").arg(mainIdent).arg(subIdent)+", "+QString(__FILE__)+":"+QString::number(__LINE__)+": "+QString("\nlast_mainIdent: %1, last_subIdent: %2, last_query: %3").arg(last_mainIdent).arg(last_subIdent).arg(QString(last_query.toHex())));
							return;
						}
						if(!checkStringIntegrity(inputData.right(inputData.size()-in.device()->pos())))
						{
							errorOutput(tr("Procotol wrong or corrupted"),QString("wrong string for description with main ident: %1, subIdent: %2").arg(mainIdent).arg(subIdent)+", "+QString(__FILE__)+":"+QString::number(__LINE__)+": "+QString("\nlast_mainIdent: %1, last_subIdent: %2, last_query: %3").arg(last_mainIdent).arg(last_subIdent).arg(QString(last_query.toHex())));
							return;
						}
						in >> temp.description;
						if((in.device()->size()-in.device()->pos())<=(int)(sizeof(quint16)+sizeof(quint8)))
						{
							errorOutput(tr("Procotol wrong or corrupted"),QString("wrong size for clan and status with main ident: %1, subIdent: %2").arg(mainIdent).arg(subIdent)+", "+QString(__FILE__)+":"+QString::number(__LINE__)+": "+QString("\nlast_mainIdent: %1, last_subIdent: %2, last_query: %3").arg(last_mainIdent).arg(last_subIdent).arg(QString(last_query.toHex())));
							return;
						}
						in >> temp.clan;
						quint8 status;
						in >> status;
						temp.type = (Player_type)status;
						if(!checkStringIntegrity(inputData.right(inputData.size()-in.device()->pos())))
						{
							errorOutput(tr("Procotol wrong or corrupted"),QString("missing skin variable: %1, subIdent: %2").arg(mainIdent).arg(subIdent)+", "+QString(__FILE__)+":"+QString::number(__LINE__)+": "+QString("\nlast_mainIdent: %1, last_subIdent: %2, last_query: %3").arg(last_mainIdent).arg(last_subIdent).arg(QString(last_query.toHex())));
							return;
						}
						in >> temp.skin;

						if(temp.id==player_id)
						{

							if((in.device()->size()-in.device()->pos())<=(int)(sizeof(quint32)))
							{
								errorOutput(tr("Procotol wrong or corrupted"),QString("wrong size for clan and status with main ident: %1, subIdent: %2").arg(mainIdent).arg(subIdent)+", "+QString(__FILE__)+":"+QString::number(__LINE__)+": "+QString("\nlast_mainIdent: %1, last_subIdent: %2, last_query: %3").arg(last_mainIdent).arg(last_subIdent).arg(QString(last_query.toHex())));
								return;
							}
							in >> player_informations.cash;
							player_informations.public_informations=temp;
							emit new_player_info(player_informations);
							emit have_current_player_info();
						}
						player_informations_list << temp;

						index++;
					}
					if(in.device()->size()!=in.device()->pos())
					{
						errorOutput(tr("Procotol wrong or corrupted"),QString("remaining data with main ident: %1, subIdent: %2").arg(mainIdent).arg(subIdent)+", "+QString(__FILE__)+":"+QString::number(__LINE__)+": "+QString("\nlast_mainIdent: %1, last_subIdent: %2, last_query: %3").arg(last_mainIdent).arg(last_subIdent).arg(QString(last_query.toHex())));
						return;
					}
					this->player_informations_list << player_informations_list;
                                        emit new_player_info(player_informations_list);
					emit new_player_info();
				}
				break;
				//file as input
				case 0x0003:
				{
					if(!checkStringIntegrity(inputData.right(inputData.size()-in.device()->pos())))
					{
						errorOutput(tr("Procotol wrong or corrupted"),QString("wrong string for file name with main ident: %1, subIdent: %2").arg(mainIdent).arg(subIdent)+", "+QString(__FILE__)+":"+QString::number(__LINE__)+": "+QString("\nlast_mainIdent: %1, last_subIdent: %2, last_query: %3").arg(last_mainIdent).arg(last_subIdent).arg(QString(last_query.toHex())));
						return;
					}
					QString fileName;
					in >> fileName;

					if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
					{
						errorOutput(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subIdent: %2").arg(mainIdent).arg(subIdent)+", "+QString(__FILE__)+":"+QString::number(__LINE__)+": "+QString("\nlast_mainIdent: %1, last_subIdent: %2, last_query: %3").arg(last_mainIdent).arg(last_subIdent).arg(QString(last_query.toHex())));
						return;
					}
					quint32 file_size;
					in >> file_size;
					if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
					{
						errorOutput(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subIdent: %2").arg(mainIdent).arg(subIdent)+", "+QString(__FILE__)+":"+QString::number(__LINE__)+": "+QString("\nlast_mainIdent: %1, last_subIdent: %2, last_query: %3").arg(last_mainIdent).arg(last_subIdent).arg(QString(last_query.toHex())));
						return;
					}
					quint32 mtime;
					in >> mtime;
					QDateTime date;
					date.setTime_t(mtime);
					DebugClass::debugConsole(QString("write the file: %1, with date: %2").arg(fileName).arg(date.toString("dd.MM.yyyy hh:mm:ss.zzz")));
					QByteArray data=inputData.right(inputData.size()-in.device()->pos());
					QFile file(datapack_base_name+fileName);
					QDir dir(QFileInfo(file).absolutePath());
					if(!dir.exists())
						if(!dir.mkpath(QFileInfo(file).absolutePath()))
						{
							errorOutput(tr("I/O error"),QString("unable to create the folder: %1").arg(QFileInfo(file).absolutePath()));
							return;
						}
					if(file.open(QIODevice::WriteOnly|QIODevice::Truncate))
					{
						if(file.write(data)!=data.size())
						{
							errorOutput(tr("I/O error"),QString("unable to write into the file: %1").arg(file.errorString()));
							return;
						}
						file.close();
						#ifdef Q_CC_GNU
							//this function avalaible on unix and mingw
							utimbuf butime;
							time_t time=mtime;
							butime.actime=time;
							butime.modtime=time;
							//creation time not exists into unix world
							if(utime(file.fileName().toLatin1().data(),&butime)!=0)
							{
								errorOutput(tr("I/O error"),QString("unable to open/create the file: %1").arg(file.errorString()));
								return;
							}
						#else
							#error Unable to set the set mtime
						#endif
					}
					else
					{
						errorOutput(tr("I/O error"),QString("unable to open/create the file: %1").arg(file.errorString()));
						return;
					}
					//emit filesContent(fileName,mtime,data);
				}
				break;
				//chat as input
				case 0x0005:
				{
					if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)+sizeof(quint8)))
					{
						errorOutput(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subIdent: %2").arg(mainIdent).arg(subIdent)+", "+QString(__FILE__)+":"+QString::number(__LINE__)+": "+QString("\nlast_mainIdent: %1, last_subIdent: %2, last_query: %3").arg(last_mainIdent).arg(last_subIdent).arg(QString(last_query.toHex())));
						return;
					}
					quint32 player_id;
					quint8 chat_type;
					in >> player_id;
					in >> chat_type;
					if(!checkStringIntegrity(inputData.right(inputData.size()-in.device()->pos())))
					{
						errorOutput(tr("Procotol wrong or corrupted"),QString("wrong text with main ident: %1, subIdent: %2").arg(mainIdent).arg(subIdent)+", "+QString(__FILE__)+":"+QString::number(__LINE__)+": "+QString("\nlast_mainIdent: %1, last_subIdent: %2, last_query: %3").arg(last_mainIdent).arg(last_subIdent).arg(QString(last_query.toHex())));
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
						errorOutput(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subIdent: %2").arg(mainIdent).arg(subIdent)+", "+QString(__FILE__)+":"+QString::number(__LINE__)+": "+QString("\nlast_mainIdent: %1, last_subIdent: %2, last_query: %3").arg(last_mainIdent).arg(last_subIdent).arg(QString(last_query.toHex())));
						return;
					}
					quint8 code;
					in >> code;
					if(!checkStringIntegrity(inputData.right(inputData.size()-in.device()->pos())))
					{
						errorOutput(tr("Procotol wrong or corrupted"),QString("wrong string for reason with main ident: %1, subIdent: %2").arg(mainIdent).arg(subIdent)+", "+QString(__FILE__)+":"+QString::number(__LINE__)+": "+QString("\nlast_mainIdent: %1, last_subIdent: %2, last_query: %3").arg(last_mainIdent).arg(last_subIdent).arg(QString(last_query.toHex())));
						return;
					}
					QString text;
					in >> text;
					switch(code)
					{
						case 0x01:
							errorOutput(tr("Disconnected by the server"),QString("You have been kicked by the server with the reason: %1").arg(text));
						return;
						case 0x02:
							errorOutput(tr("Disconnected by the server"),QString("You have been ban by the server with the reason: %1").arg(text));
						return;
						case 0x03:
							errorOutput(tr("Disconnected by the server"),QString("You have been disconnected by the server with the reason: %1").arg(text));
						return;
						default:
							errorOutput(tr("Disconnected by the server"),QString("You have been disconnected by strange way by the server with the reason: %1").arg(text));
						return;
					}
				}
				break;
				//Player number
				case 0x0009:
				{
					if((in.device()->size()-in.device()->pos())<(int)sizeof(quint16)*2)
					{
						errorOutput(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subIdent: %2").arg(mainIdent).arg(subIdent)+", "+QString(__FILE__)+":"+QString::number(__LINE__)+": "+QString("\nlast_mainIdent: %1, last_subIdent: %2, last_query: %3").arg(last_mainIdent).arg(last_subIdent).arg(QString(last_query.toHex())));
						return;
					}
					quint16 current,max;
					in >> current;
					in >> max;
					emit number_of_player(current,max);
				}
				break;
				default:
				errorOutput(tr("Procotol wrong or corrupted"),QString("unknow subIdent reply code: %1, subIdent: %2").arg(mainIdent).arg(subIdent)+", "+QString(__FILE__)+":"+QString::number(__LINE__)+": "+QString("\nlast_mainIdent: %1, last_subIdent: %2, last_query: %3").arg(last_mainIdent).arg(last_subIdent).arg(QString(last_query.toHex())));
			}
			last_subIdent=subIdent;
		}
		break;
		default:
			last_subIdent=0;
			errorOutput(tr("Procotol wrong or corrupted"),QString("unknow ident reply code: %1").arg(mainIdent)+", "+QString(__FILE__)+":"+QString::number(__LINE__)+": "+QString("\nlast_mainIdent: %1, last_subIdent: %2, last_query: %3").arg(last_mainIdent).arg(last_subIdent).arg(QString(last_query.toHex())));
			return;
		break;
	}
	last_mainIdent=mainIdent;
	last_query=inputData;
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
	out << (quint8)0x02;
	out << (quint16)0x0001;
	out << (quint8)query_number;
	out << QString(PROTOCOL_HEADER);
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
	out << (quint8)0x02;
	out << (quint16)0x0002;
	out << (quint8)query_number;
	out << login;
	QCryptographicHash hash(QCryptographicHash::Sha1);
	hash.addData(pass.toUtf8());
	outputData+=hash.result();
	sendData(outputData);
	add_to_query_list(query_number,query_type_login);
}

QList<Player_public_informations> Pokecraft_client::get_player_informations_list()
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

Player_private_and_public_informations Pokecraft_client::get_player_informations()
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
	out << (quint16)0x0003;
	out << chatType;
	out << text;
	sendData(outputData);
	if(!text.startsWith('/'))
		emit new_chat_text(player_id,chatType,text);
}

void Pokecraft_client::sendPM(QString text,QString pseudo)
{
	emit new_chat_text(player_id,0x06,text);
	if(pseudo==player_informations.public_informations.pseudo)
		return;
	QByteArray outputData;
	QDataStream out(&outputData, QIODevice::WriteOnly);
	out.setVersion(QDataStream::Qt_4_4);
	out << (quint8)0x42;
	out << (quint16)0x0003;
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
		out << (quint16)0x000A;
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
		out << (quint16)0x000B;
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
	mainIdent=0;
	queryNumberReturned=0;
	subIdent=0;
	returnCode=0;
	player_id_watching.clear();
	player_informations_list.clear();
	is_logged=false;
	have_send_protocol=false;
	query_list.clear();
	wait_datapack_content=false;
	datapackFilesList.clear();
	last_mainIdent=0;
	last_subIdent=0;
	last_query.clear();
	query_files_list.clear();
	is_logged=false;
	have_send_protocol=false;
}

QString Pokecraft_client::getPseudo()
{
	return player_informations.public_informations.pseudo;
}

void Pokecraft_client::tryDisconnect()
{
	socket.disconnectFromHost();
}

QString Pokecraft_client::getHost()
{
	return host;
}

quint16 Pokecraft_client::getPort()
{
	return port;
}

void Pokecraft_client::sendDatapackContent()
{
	if(wait_datapack_content)
	{
		DebugClass::debugConsole(QString("already in wait of datapack content"));
		return;
	}
	wait_datapack_content=true;
	datapack_base_name=QString("%1/%2-%3/").arg(QApplication::applicationDirPath()).arg(host).arg(port);
	quint8 datapack_content_query_number=queryNumber();
	datapackFilesList=listDatapack("");
	QByteArray outputData;
	QDataStream out(&outputData, QIODevice::WriteOnly);
	out.setVersion(QDataStream::Qt_4_4);
	out << (quint8)0x02;
	out << (quint16)0x000C;
	out << datapack_content_query_number;
	out << (quint32)datapackFilesList.size();
	int index=0;
	while(index<datapackFilesList.size())
	{
		out << datapackFilesList.at(index);
		struct stat info;
		stat(QString(datapack_base_name+datapackFilesList.at(index)).toLatin1().data(),&info);
		out << (quint32)info.st_mtime;
		index++;
	}
	sendData(outputData);
	add_to_query_list(datapack_content_query_number,query_type_datapack);
}

const QStringList Pokecraft_client::listDatapack(QString suffix)
{
	QStringList returnFile;
	QDir finalDatapackFolder(datapack_base_name+suffix);
	QString fileName;
	QFileInfoList entryList=finalDatapackFolder.entryInfoList(QDir::AllEntries|QDir::NoDotAndDotDot|QDir::Hidden|QDir::System,QDir::DirsFirst);//possible wait time here
	int sizeEntryList=entryList.size();
	for (int index=0;index<sizeEntryList;++index)
	{
		QFileInfo fileInfo=entryList.at(index);
		if(fileInfo.isDir())
			returnFile << listDatapack(suffix+fileInfo.fileName()+"/");//put unix separator because it's transformed into that's under windows too
		else
			returnFile << suffix+fileInfo.fileName();
	}
	return returnFile;
}

void Pokecraft_client::cleanDatapack(QString suffix)
{
	QDir finalDatapackFolder(datapack_base_name+suffix);
	QFileInfoList entryList=finalDatapackFolder.entryInfoList(QDir::AllEntries|QDir::NoDotAndDotDot|QDir::Hidden|QDir::System,QDir::DirsFirst);//possible wait time here
	int sizeEntryList=entryList.size();
	for (int index=0;index<sizeEntryList;++index)
	{
		QFileInfo fileInfo=entryList.at(index);
		if(fileInfo.isDir())
			cleanDatapack(suffix+fileInfo.fileName()+"/");//put unix separator because it's transformed into that's under windows too
		else
			return;
	}
	entryList=finalDatapackFolder.entryInfoList(QDir::AllEntries|QDir::NoDotAndDotDot|QDir::Hidden|QDir::System,QDir::DirsFirst);//possible wait time here
	if(entryList.size()==0)
		finalDatapackFolder.rmpath(datapack_base_name+suffix);
}

quint32 Pokecraft_client::getId()
{
	return player_id;
}

QString Pokecraft_client::get_datapack_base_name()
{
	return datapack_base_name;
}
