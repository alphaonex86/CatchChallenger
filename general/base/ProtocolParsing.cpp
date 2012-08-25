#include "ProtocolParsing.h"
#include "DebugClass.h"
#include "GeneralVariable.h"

using namespace Pokecraft;

QSet<quint8>				ProtocolParsing::mainCodeWithoutSubCodeTypeClientToServer;//if need sub code or not
//if is a query
QSet<quint8>				ProtocolParsing::mainCode_IsQueryClientToServer;
quint8					ProtocolParsing::replyCodeClientToServer;
//predefined size
QHash<quint8,quint16>			ProtocolParsing::sizeOnlyMainCodePacketClientToServer;
QHash<quint8,QHash<quint16,quint16> >	ProtocolParsing::sizeMultipleCodePacketClientToServer;
QHash<quint8,quint16>			ProtocolParsing::replySizeOnlyMainCodePacketClientToServer;
QHash<quint8,QHash<quint16,quint16> >	ProtocolParsing::replySizeMultipleCodePacketClientToServer;

QSet<quint8>				ProtocolParsing::mainCodeWithoutSubCodeTypeServerToClient;//if need sub code or not
//if is a query
QSet<quint8>				ProtocolParsing::mainCode_IsQueryServerToClient;
quint8					ProtocolParsing::replyCodeServerToClient;
//predefined size
QHash<quint8,quint16>			ProtocolParsing::sizeOnlyMainCodePacketServerToClient;
QHash<quint8,QHash<quint16,quint16> >	ProtocolParsing::sizeMultipleCodePacketServerToClient;
QHash<quint8,quint16>			ProtocolParsing::replySizeOnlyMainCodePacketServerToClient;
QHash<quint8,QHash<quint16,quint16> >	ProtocolParsing::replySizeMultipleCodePacketServerToClient;

//temp data
qint64 ProtocolParsingOutput::byteWriten;
quint8 ProtocolParsingInput::temp_size_8Bits;
quint16 ProtocolParsingInput::temp_size_16Bits;
quint32 ProtocolParsingInput::temp_size_32Bits;

ProtocolParsing::ProtocolParsing(QAbstractSocket * device)
{
	this->socket=device;
}

void ProtocolParsing::initialiseTheVariable()
{
	//def query without the sub code
	ProtocolParsing::mainCodeWithoutSubCodeTypeServerToClient << 0xC0 << 0xC1 << 0xC3 << 0xC4 << 0xC5 << 0xC6 << 0xC7 << 0xC8;
	ProtocolParsing::mainCodeWithoutSubCodeTypeClientToServer << 0x40 << 0x41;

	//define the size of direct query
	{
		//default like is was more than 255 players
		ProtocolParsing::sizeOnlyMainCodePacketServerToClient[0xC3]=2;
	}
	ProtocolParsing::sizeOnlyMainCodePacketServerToClient[0xC4]=0;
	ProtocolParsing::sizeOnlyMainCodePacketClientToServer[0x40]=2;

	//define the size of the reply
	replySizeMultipleCodePacketClientToServer[0x79][0x0001]=0;

	//main code for query with reply
	ProtocolParsing::mainCode_IsQueryClientToServer << 0x02;

	//reply code
	ProtocolParsing::replyCodeServerToClient=0xC1;
	ProtocolParsing::replyCodeClientToServer=0x41;
}

void ProtocolParsing::setMaxPlayers(quint16 maxPlayers)
{
	if(maxPlayers<=255)
	{
		ProtocolParsing::sizeOnlyMainCodePacketServerToClient[0xC3]=1;
	}
	else
	{
		//this case do into initialiseTheVariable()
	}
}

QByteArray ProtocolParsing::toUTF8(const QString &text)
{
	if(text.isEmpty() || text.size()>255)
		return QByteArray();
	QByteArray returnedData,data;
	data=text.toUtf8();
	if(data.size()==0 || data.size()>255)
		return QByteArray();
	returnedData[0]=data.size();
	returnedData+=data;
	return returnedData;
}

ProtocolParsingInput::ProtocolParsingInput(QAbstractSocket * socket,PacketModeTransmission packetModeTransmission) :
	ProtocolParsing(socket)
{
	connect(socket,SIGNAL(readyRead()),this,SLOT(parseIncommingData()));
	isClient=(packetModeTransmission==PacketModeTransmission_Client);
	dataClear();
}

bool ProtocolParsingInput::checkStringIntegrity(const QByteArray & data)
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

ProtocolParsingOutput::ProtocolParsingOutput(QAbstractSocket * socket,PacketModeTransmission packetModeTransmission) :
	ProtocolParsing(socket)
{
	isClient=(packetModeTransmission==PacketModeTransmission_Client);
}

void ProtocolParsingInput::parseIncommingData()
{
	#ifdef PROTOCOLPARSINGDEBUG
	DebugClass::debugConsole(QString::number(isClient)+QString(" parseIncommingData(): socket->bytesAvailable(): %1").arg(socket->bytesAvailable()));
	#endif

	//put this as general
	QDataStream in(socket);
	in.setVersion(QDataStream::Qt_4_4);

	while(socket->bytesAvailable()>0)
	{
		if(!haveData)
		{
			if(socket->bytesAvailable()<(int)sizeof(quint8))//ignore because first int is cuted!
				return;
			in >> mainCodeType;
			#ifdef PROTOCOLPARSINGDEBUG
			DebugClass::debugConsole(QString::number(isClient)+QString(" parseIncommingData(): !haveData, mainCodeType: %1").arg(mainCodeType));
			#endif
			haveData=true;
			haveData_dataSize=false;
			have_subCodeType=false;
			have_query_number=false;
			is_reply=false;
			if(isClient)
				need_subCodeType=!mainCodeWithoutSubCodeTypeServerToClient.contains(mainCodeType);
			else
				need_subCodeType=!mainCodeWithoutSubCodeTypeClientToServer.contains(mainCodeType);
			need_query_number=false;
			data_size.clear();
		}

		if(!have_subCodeType)
		{
			#ifdef PROTOCOLPARSINGDEBUG
			DebugClass::debugConsole(QString::number(isClient)+QString(" parseIncommingData(): !have_subCodeType"));
			#endif
			if(!need_subCodeType)
			{
				#ifdef PROTOCOLPARSINGDEBUG
				DebugClass::debugConsole(QString::number(isClient)+QString(" parseIncommingData(): !need_subCodeType"));
				#endif
				//if is a reply
				if((isClient && mainCodeType==replyCodeServerToClient) || (!isClient && mainCodeType==replyCodeClientToServer))
				{
					#ifdef PROTOCOLPARSINGDEBUG
					DebugClass::debugConsole(QString::number(isClient)+QString(" parseIncommingData(): need_query_number=true"));
					#endif
					is_reply=true;
					need_query_number=true;
					//the size with be resolved later
				}
				else
				{

					if(isClient)
					{
						//if is query with reply
						if(mainCode_IsQueryServerToClient.contains(mainCodeType))
						{
							#ifdef PROTOCOLPARSINGDEBUG
							DebugClass::debugConsole(QString::number(isClient)+QString(" parseIncommingData(): need_query_number=true, query with reply"));
							#endif
							need_query_number=true;
						}

						//check if have defined size
						if(sizeOnlyMainCodePacketServerToClient.contains(mainCodeType))
						{
							//have fixed size
							dataSize=sizeOnlyMainCodePacketServerToClient[mainCodeType];
							haveData_dataSize=true;
						}
					}
					else
					{
						//if is query with reply
						if(mainCode_IsQueryClientToServer.contains(mainCodeType))
							need_query_number=true;

						//check if have defined size
						if(sizeOnlyMainCodePacketClientToServer.contains(mainCodeType))
						{
							//have fixed size
							dataSize=sizeOnlyMainCodePacketClientToServer[mainCodeType];
							haveData_dataSize=true;
						}
					}
				}
			}
			else
			{
				#ifdef PROTOCOLPARSINGDEBUG
				DebugClass::debugConsole(QString::number(isClient)+QString(" parseIncommingData(): need_subCodeType"));
				#endif
				if(socket->bytesAvailable()<(int)sizeof(quint16))//ignore because first int is cuted!
					return;
				in >> subCodeType;

				if(isClient)
				{
					#ifdef PROTOCOLPARSINGDEBUG
					DebugClass::debugConsole(QString::number(isClient)+QString(" parseIncommingData(): isClient"));
					#endif
					//if is query with reply
					if(mainCode_IsQueryServerToClient.contains(mainCodeType))
					{
						#ifdef PROTOCOLPARSINGDEBUG
						DebugClass::debugConsole(QString::number(isClient)+QString(" parseIncommingData(): need_query_number=true, query with reply (mainCode_IsQueryServerToClient)"));
						#endif
						need_query_number=true;
					}

					//check if have defined size
					if(sizeMultipleCodePacketServerToClient.contains(mainCodeType))
					{
						if(sizeMultipleCodePacketServerToClient[mainCodeType].contains(subCodeType))
						{
							//have fixed size
							dataSize=sizeMultipleCodePacketServerToClient[mainCodeType][subCodeType];
							haveData_dataSize=true;
						}
					}
				}
				else
				{
					#ifdef PROTOCOLPARSINGDEBUG
					DebugClass::debugConsole(QString::number(isClient)+QString(" parseIncommingData(): !isClient"));
					#endif
					//if is query with reply
					if(mainCode_IsQueryClientToServer.contains(mainCodeType))
					{
						#ifdef PROTOCOLPARSINGDEBUG
						DebugClass::debugConsole(QString::number(isClient)+QString(" parseIncommingData(): need_query_number=true, query with reply (mainCode_IsQueryClientToServer)"));
						#endif
						need_query_number=true;
					}

					//check if have defined size
					if(sizeMultipleCodePacketClientToServer.contains(mainCodeType))
					{
						if(sizeMultipleCodePacketClientToServer[mainCodeType].contains(subCodeType))
						{
							//have fixed size
							dataSize=sizeMultipleCodePacketClientToServer[mainCodeType][subCodeType];
							haveData_dataSize=true;
						}
					}
				}
			}

			//set this parsing step is done
			have_subCodeType=true;
		}
		#ifdef PROTOCOLPARSINGDEBUG
		else
			DebugClass::debugConsole(QString::number(isClient)+QString(" parseIncommingData(): have_subCodeType"));
		#endif
		if(!have_query_number && need_query_number)
		{
			#ifdef PROTOCOLPARSINGDEBUG
			DebugClass::debugConsole(QString::number(isClient)+QString(" parseIncommingData(): need_query_number"));
			#endif
			if(socket->bytesAvailable()<(int)sizeof(quint8))
				return;
			in >> queryNumber;

			// it's reply
			if(is_reply)
			{
				#ifdef PROTOCOLPARSINGDEBUG
				DebugClass::debugConsole(QString::number(isClient)+QString(" parseIncommingData(): it's reply, resolv size"));
				#endif
				if(replySize.contains(queryNumber))
				{
					dataSize=replySize[queryNumber];
					haveData_dataSize=true;
				}
			}
			else // it's query with reply
			{
				//size resolved before, into subCodeType step
			}

			//set this parsing step is done
			have_query_number=true;
		}
		#ifdef PROTOCOLPARSINGDEBUG
		else
			DebugClass::debugConsole(QString::number(isClient)+QString(" parseIncommingData(): not need_query_number"));
		#endif
		if(!haveData_dataSize)
		{
			#ifdef PROTOCOLPARSINGDEBUG
			DebugClass::debugConsole(QString::number(isClient)+QString(" parseIncommingData(): !haveData_dataSize"));
			#endif
			while(!haveData_dataSize)
			{
				switch(data_size.size())
				{
					case 0:
					{
						if(socket->bytesAvailable()<(int)sizeof(quint8))
							return;
						data_size+=socket->read(sizeof(quint8));
						QDataStream in_size(data_size);
						in_size.setVersion(QDataStream::Qt_4_4);
						in_size >> temp_size_8Bits;
						if(temp_size_8Bits!=0x00)
						{
							dataSize=temp_size_8Bits;
							haveData_dataSize=true;
							#ifdef PROTOCOLPARSINGDEBUG
							DebugClass::debugConsole(QString::number(isClient)+QString(" parseIncommingData(): have 8Bits data size"));
							#endif
						}
						#ifdef PROTOCOLPARSINGDEBUG
						else
							DebugClass::debugConsole(QString::number(isClient)+QString(" parseIncommingData(): have not 8Bits data size: %1, temp_size_8Bits: %2").arg(QString(data_size.toHex())).arg(temp_size_8Bits));
						#endif
					}
					break;
					case sizeof(quint8):
					{
						if(socket->bytesAvailable()<(int)sizeof(quint16))
							return;
						data_size+=socket->read(sizeof(quint8));
						{
							QDataStream in_size(data_size);
							in_size.setVersion(QDataStream::Qt_4_4);
							in_size >> temp_size_16Bits;
						}
						if(temp_size_16Bits!=0x0000)
						{
							data_size+=socket->read(sizeof(quint8));
							QDataStream in_size(data_size);
							in_size.setVersion(QDataStream::Qt_4_4);
							//in_size.device()->seek(sizeof(quint8)); or in_size >> temp_size_8Bits;, not both
							in_size >> temp_size_8Bits;
							in_size >> temp_size_16Bits;
							dataSize=temp_size_16Bits;
							haveData_dataSize=true;
							#ifdef PROTOCOLPARSINGDEBUG
							DebugClass::debugConsole(QString::number(isClient)+QString(" parseIncommingData(): have 16Bits data size: %1, temp_size_16Bits: %2").arg(QString(data_size.toHex())).arg(dataSize));
							#endif
						}
						#ifdef PROTOCOLPARSINGDEBUG
						else
							DebugClass::debugConsole(QString::number(isClient)+QString(" parseIncommingData(): have not 16Bits data size"));
						#endif
					}
					break;
					case sizeof(quint16):
					{
						if(socket->bytesAvailable()<(int)sizeof(quint32))
							return;
						data_size+=socket->read(sizeof(quint32));
						QDataStream in_size(data_size);
						in_size.setVersion(QDataStream::Qt_4_4);
						in_size >> temp_size_16Bits;
						in_size >> temp_size_32Bits;
						if(temp_size_32Bits!=0x00000000)
						{
							dataSize=temp_size_32Bits;
							haveData_dataSize=true;
						}
						else
						{
							emit error("size is null");
							return;
						}
					}
					break;
					default:
					emit error(QString("size not understand, internal bug: %1").arg(data_size.size()));
					return;
				}
			}
		}
		#ifdef PROTOCOLPARSINGDEBUG
		else
			DebugClass::debugConsole(QString::number(isClient)+QString(" parseIncommingData(): haveData_dataSize"));
		#endif
		#ifdef POKECRAFT_EXTRA_CHECK
		if(!haveData_dataSize)
		{
			emit error("have not the size here!");
			return;
		}
		#endif
		#ifdef PROTOCOLPARSINGDEBUG
		DebugClass::debugConsole(QString::number(isClient)+QString(" parseIncommingData(): header informations is ready, dataSize: %1").arg(dataSize));
		#endif
		if(dataSize>16*1024*1024)
		{
			emit error("packet size too big");
			return;
		}
		if(dataSize>0)
		{
			do
			{
				//if have too many data, or just the size
				if((dataSize-data.size())<=socket->bytesAvailable())
					data.append(socket->read(dataSize-data.size()));
				else //if need more data
					data.append(socket->readAll());
			} while(
				//need more data
				(dataSize-data.size())>0 &&
				//and have more data
				socket->bytesAvailable()>0
				);

			if((dataSize-data.size())>0)
			{
				#ifdef PROTOCOLPARSINGDEBUG
				if(!need_subCodeType)
					DebugClass::debugConsole(QString::number(isClient)+QString(" parseIncommingData(): not suffisent data: %1, wait more tcp packet").arg(mainCodeType));
				else
					DebugClass::debugConsole(QString::number(isClient)+QString(" parseIncommingData(): not suffisent data: %1,%2, wait more tcp packet").arg(mainCodeType).arg(subCodeType));
				#endif

				return;
			}
		}
		#ifdef POKECRAFT_EXTRA_CHECK
		if(dataSize!=(quint32)data.size())
		{
			emit error("wrong data size here");
			return;
		}
		#endif
		#ifdef PROTOCOLPARSINGINPUTDEBUG
		if(isClient)
			DebugClass::debugConsole(QString::number(isClient)+QString(" parseIncommingData(): parse message as client"));
		else
			DebugClass::debugConsole(QString::number(isClient)+QString(" parseIncommingData(): parse message as server"));
		#endif
		#ifdef PROTOCOLPARSINGINPUTDEBUG
		DebugClass::debugConsole(QString::number(isClient)+QString(" parseIncommingData(): data: %1").arg(QString(data.toHex())));
		#endif
		if(!need_query_number)
		{
			if(!need_subCodeType)
			{
				#ifdef PROTOCOLPARSINGINPUTDEBUG
				DebugClass::debugConsole(QString::number(isClient)+QString(" parseIncommingData(): !need_query_number && !need_subCodeType, mainCodeType: %1").arg(mainCodeType));
				#endif
				parseMessage(mainCodeType,data);
			}
			else
			{
				#ifdef PROTOCOLPARSINGINPUTDEBUG
				DebugClass::debugConsole(QString::number(isClient)+QString(" parseIncommingData(): !need_query_number && need_subCodeType, mainCodeType: %1, subCodeType: %2").arg(mainCodeType).arg(subCodeType));
				#endif
				parseMessage(mainCodeType,subCodeType,data);
			}
		}
		else
		{
			if(!is_reply)
			{
				#ifdef PROTOCOLPARSINGINPUTDEBUG
				DebugClass::debugConsole(QString::number(isClient)+QString(" parseIncommingData(): need_query_number && !is_reply"));
				#endif
				if(!need_subCodeType)
					parseQuery(mainCodeType,queryNumber,data);
				else
					parseQuery(mainCodeType,subCodeType,queryNumber,data);
			}
			else
			{
				if(!reply_subCodeType.contains(queryNumber))
				{
					if(!reply_mainCodeType.contains(queryNumber))
					{
						emit error("reply to a query not send");
						return;
					}
					#ifdef PROTOCOLPARSINGINPUTDEBUG
					DebugClass::debugConsole(QString::number(isClient)+QString(" parseIncommingData(): need_query_number && is_reply && !reply_subCodeType.contains(queryNumber)"));
					#endif
					mainCodeType=reply_mainCodeType[queryNumber];
					reply_mainCodeType.remove(queryNumber);
					parseReplyData(mainCodeType,queryNumber,data);
				}
				else
				{
					#ifdef PROTOCOLPARSINGINPUTDEBUG
					DebugClass::debugConsole(QString::number(isClient)+QString(" parseIncommingData(): need_query_number && is_reply && reply_subCodeType.contains(queryNumber)"));
					#endif
					mainCodeType=reply_mainCodeType[queryNumber];
					subCodeType=reply_subCodeType[queryNumber];
					reply_mainCodeType.remove(queryNumber);
					reply_subCodeType.remove(queryNumber);
					parseReplyData(mainCodeType,subCodeType,queryNumber,data);
				}
			}

		}
		dataClear();
	}
}

void ProtocolParsingInput::dataClear()
{
	data.clear();
	dataSize=0;
	haveData=false;
}

void ProtocolParsingInput::newOutputQuery(const quint8 &mainCodeType,const quint8 &queryNumber)
{
	if(reply_mainCodeType.contains(queryNumber))
	{
		emit error("Query with this query number already found");
		return;
	}
	if(isClient)
	{
		if(replySizeOnlyMainCodePacketServerToClient.contains(mainCodeType))
			replySize[queryNumber]=replySizeOnlyMainCodePacketServerToClient[mainCodeType];
	}
	else
	{
		if(replySizeOnlyMainCodePacketClientToServer.contains(mainCodeType))
			replySize[queryNumber]=replySizeOnlyMainCodePacketClientToServer[mainCodeType];
	}
	#ifdef PROTOCOLPARSINGINPUTDEBUG
	DebugClass::debugConsole(QString::number(isClient)+QString(" ProtocolParsingInput::newOutputQuery(): queryNumber: %1, mainCodeType: %2").arg(queryNumber).arg(mainCodeType));
	#endif
	reply_mainCodeType[queryNumber]=mainCodeType;
}

void ProtocolParsingInput::newOutputQuery(const quint8 &mainCodeType,const quint16 &subCodeType,const quint8 &queryNumber)
{
	if(reply_mainCodeType.contains(queryNumber))
	{
		emit error("Query with this query number already found");
		return;
	}
	if(isClient)
	{
		if(replySizeMultipleCodePacketServerToClient.contains(mainCodeType))
		{
			if(replySizeMultipleCodePacketServerToClient[mainCodeType].contains(subCodeType))
				replySize[queryNumber]=replySizeMultipleCodePacketServerToClient[mainCodeType][subCodeType];
		}
	}
	else
	{
		if(replySizeMultipleCodePacketClientToServer.contains(mainCodeType))
		{
			if(replySizeMultipleCodePacketClientToServer[mainCodeType].contains(subCodeType))
				replySize[queryNumber]=replySizeMultipleCodePacketClientToServer[mainCodeType][subCodeType];
		}
	}
	#ifdef PROTOCOLPARSINGINPUTDEBUG
	DebugClass::debugConsole(QString::number(isClient)+QString(" ProtocolParsingInput::newOutputQuery(): queryNumber: %1, mainCodeType: %2, subCodeType: %3").arg(queryNumber).arg(mainCodeType).arg(subCodeType));
	#endif
	reply_mainCodeType[queryNumber]=mainCodeType;
	reply_subCodeType[queryNumber]=subCodeType;
}

bool ProtocolParsingOutput::postReplyData(const quint8 &queryNumber,const QByteArray &data)
{
	QByteArray block;
	QDataStream out(&block, QAbstractSocket::WriteOnly);
	if(isClient)
		out << replyCodeClientToServer;
	else
		out << replyCodeServerToClient;
	out << queryNumber;

	if(!replySize.contains(queryNumber))
		block+=encodeSize(data.size());
	else
		replySize.remove(queryNumber);

	return internalPackOutcommingData(block+data);
}

void ProtocolParsingOutput::newInputQuery(const quint8 &mainCodeType,const quint8 &queryNumber)
{
	if(replySize.contains(queryNumber))
	{
		emit error("Query with this query number already found");
		return;
	}
	if(isClient)
	{
		if(replySizeOnlyMainCodePacketClientToServer.contains(mainCodeType))
			replySize[queryNumber]=replySizeOnlyMainCodePacketClientToServer[mainCodeType];
	}
	else
	{
		if(replySizeOnlyMainCodePacketServerToClient.contains(mainCodeType))
			replySize[queryNumber]=replySizeOnlyMainCodePacketServerToClient[mainCodeType];
	}
}

void ProtocolParsingOutput::newInputQuery(const quint8 &mainCodeType,const quint16 &subCodeType,const quint8 &queryNumber)
{
	if(replySize.contains(queryNumber))
	{
		emit error("Query with this query number already found");
		return;
	}
	if(isClient)
	{
		if(replySizeMultipleCodePacketClientToServer.contains(mainCodeType))
		{
			if(replySizeMultipleCodePacketClientToServer[mainCodeType].contains(subCodeType))
				replySize[queryNumber]=replySizeMultipleCodePacketClientToServer[mainCodeType][subCodeType];
		}
	}
	else
	{
		if(replySizeMultipleCodePacketServerToClient.contains(mainCodeType))
		{
			if(replySizeMultipleCodePacketServerToClient[mainCodeType].contains(subCodeType))
				replySize[queryNumber]=replySizeMultipleCodePacketServerToClient[mainCodeType][subCodeType];
		}
	}
}

bool ProtocolParsingOutput::packOutcommingData(const quint8 &mainCodeType,const quint16 &subCodeType,const QByteArray &data)
{
	QByteArray block;
	QDataStream out(&block, QAbstractSocket::WriteOnly);
	out << mainCodeType;
	out << subCodeType;

	if(isClient)
	{
		if(!sizeMultipleCodePacketClientToServer.contains(mainCodeType))
			block+=encodeSize(data.size());
		else if(!sizeMultipleCodePacketClientToServer[mainCodeType].contains(subCodeType))
			block+=encodeSize(data.size());
	}
	else
	{
		if(!sizeMultipleCodePacketServerToClient.contains(mainCodeType))
			block+=encodeSize(data.size());
		else if(!sizeMultipleCodePacketServerToClient[mainCodeType].contains(subCodeType))
			block+=encodeSize(data.size());
	}

	return internalPackOutcommingData(block+data);
}

bool ProtocolParsingOutput::packOutcommingData(const quint8 &mainCodeType,const QByteArray &data)
{
	QByteArray block;
	QDataStream out(&block, QAbstractSocket::WriteOnly);
	out << mainCodeType;

	if(isClient)
	{
		if(!sizeOnlyMainCodePacketClientToServer.contains(mainCodeType))
			block+=encodeSize(data.size());
	}
	else
	{
		if(!sizeOnlyMainCodePacketServerToClient.contains(mainCodeType))
			block+=encodeSize(data.size());
	}

	return internalPackOutcommingData(block+data);
}

bool ProtocolParsingOutput::packOutcommingQuery(const quint8 &mainCodeType,const quint8 &queryNumber,const QByteArray &data)
{
	emit newOutputQuery(mainCodeType,queryNumber);

	QByteArray block;
	QDataStream out(&block, QAbstractSocket::WriteOnly);
	out << mainCodeType;
	out << queryNumber;

	if(isClient)
	{
		if(!sizeOnlyMainCodePacketClientToServer.contains(mainCodeType))
			block+=encodeSize(data.size());
	}
	else
	{
		if(!sizeOnlyMainCodePacketServerToClient.contains(mainCodeType))
			block+=encodeSize(data.size());
	}

	return internalPackOutcommingData(block+data);
}

bool ProtocolParsingOutput::packOutcommingQuery(const quint8 &mainCodeType,const quint16 &subCodeType,const quint8 &queryNumber,const QByteArray &data)
{
	emit newOutputQuery(mainCodeType,subCodeType,queryNumber);

	QByteArray block;
	QDataStream out(&block, QAbstractSocket::WriteOnly);
	out << mainCodeType;
	out << subCodeType;
	out << queryNumber;

	if(isClient)
	{
		if(!sizeMultipleCodePacketClientToServer.contains(mainCodeType))
			block+=encodeSize(data.size());
		else if(!sizeMultipleCodePacketClientToServer[mainCodeType].contains(subCodeType))
			block+=encodeSize(data.size());
	}
	else
	{
		if(!sizeMultipleCodePacketServerToClient.contains(mainCodeType))
			block+=encodeSize(data.size());
		else if(!sizeMultipleCodePacketServerToClient[mainCodeType].contains(subCodeType))
			block+=encodeSize(data.size());
	}

	return internalPackOutcommingData(block+data);
}

bool ProtocolParsingOutput::internalPackOutcommingData(const QByteArray &data)
{
	#ifdef PROTOCOLPARSINGDEBUG
	DebugClass::debugConsole("internalPackOutcommingData(): start");
	#endif
	#ifdef DEBUG_PROTOCOLPARSING_RAW_NETWORK
	emit message(QString("Sended packet size: %1: %2").arg(data.size()).arg(QString(data.toHex())));
	#endif // DEBUG_PROTOCOLPARSING_RAW_NETWORK
	byteWriten = socket->write(data);
	if(data.size()!=byteWriten)
	{
		#ifdef PROTOCOLPARSINGDEBUG
		DebugClass::debugConsole(QString("All the bytes have not be written: %1, byteWriten: %2").arg(socket->errorString()).arg(byteWriten));
		#endif
		emit error(QString("All the bytes have not be written: %1, byteWriten: %2").arg(socket->errorString()).arg(byteWriten));
		return false;
	}
	return true;
}

QByteArray ProtocolParsingOutput::encodeSize(quint32 size)
{
	QByteArray block;
	QDataStream out(&block, QAbstractSocket::WriteOnly);
	if(size<=0xFF)
		out << quint8(size);
	else if(size<=0xFFFF)
		out << quint8(0x00) << quint16(size);
	else
		out << quint16(0x0000) << quint32(size);
	return block;
}
