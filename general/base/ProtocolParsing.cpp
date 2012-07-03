#include "ProtocolParsing.h"
#include "DebugClass.h"

//connexion parameters
PacketSizeMode ProtocolParsing::packetSizeMode;

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
	this->device=device;
}

void ProtocolParsing::initialiseTheVariable()
{
	ProtocolParsing::mainCodeWithoutSubCodeTypeServerToClient << 0xC3 << 0xC4 << 0xD1;
	ProtocolParsing::mainCodeWithoutSubCodeTypeClientToServer << 0x40 << 0x51;
	ProtocolParsing::sizeOnlyMainCodePacketServerToClient[0xC4]=0;
	ProtocolParsing::sizeOnlyMainCodePacketServerToClient[0xC3]=4;
	ProtocolParsing::sizeOnlyMainCodePacketClientToServer[0x40]=2;

	ProtocolParsing::mainCode_IsQueryClientToServer << 0x02;

	ProtocolParsing::replyCodeServerToClient=0xC1;
	ProtocolParsing::replyCodeClientToServer=0x41;
}

ProtocolParsingInput::ProtocolParsingInput(QAbstractSocket * device,PacketModeTransmission packetModeTransmission) :
	ProtocolParsing(device)
{
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

ProtocolParsingOutput::ProtocolParsingOutput(QAbstractSocket * device,PacketModeTransmission packetModeTransmission) :
	ProtocolParsing(device)
{
	isClient=(packetModeTransmission==PacketModeTransmission_Client);
}

void ProtocolParsingInput::parseIncommingData()
{
	//put this as general
	QDataStream in(device);
	in.setVersion(QDataStream::Qt_4_4);

	while(device->bytesAvailable()>0)
	{
		if(!haveData)
		{
			if(device->bytesAvailable()<(int)sizeof(quint8))//ignore because first int is cuted!
				return;
			in >> mainCodeType;
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
			if(!need_subCodeType)
			{
				//if is a reply
				if((isClient && mainCodeType==replyCodeServerToClient) || (!isClient && mainCodeType==replyCodeClientToServer))
				{
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
							need_query_number=true;

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
				if(device->bytesAvailable()<(int)sizeof(quint16))//ignore because first int is cuted!
					return;
				in >> subCodeType;

				if(isClient)
				{
					//if is query with reply
					if(mainCode_IsQueryServerToClient.contains(mainCodeType))
						need_query_number=true;

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
					//if is query with reply
					if(mainCode_IsQueryClientToServer.contains(mainCodeType))
						need_query_number=true;

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
		if(!have_query_number && need_query_number)
		{
			if(device->bytesAvailable()<(int)sizeof(quint8))
				return;
			in >> queryNumber;

			// it's reply
			if(is_reply)
			{
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
		if(!haveData_dataSize)
		{
			if(!haveData_dataSize)
			{
				//decode the size here
				if(packetSizeMode==PacketSizeMode_Small)
				{
					QDataStream in_size(data_size);
					in_size.setVersion(QDataStream::Qt_4_4);

					while(!haveData_dataSize)
					{
						switch(data_size.size())
						{
							case 0:
							{
								if(device->bytesAvailable()<(int)sizeof(quint8))
									return;
								data_size+=device->read(sizeof(quint8));
								in_size >> temp_size_8Bits;
								if(temp_size_8Bits!=0x00)
								{
									dataSize=temp_size_8Bits;
									haveData_dataSize=true;
								}
							}
							break;
							case sizeof(quint8):
							{
								if(device->bytesAvailable()<(int)sizeof(quint16))
									return;
								data_size+=device->read(sizeof(quint16));
								in_size >> temp_size_16Bits;
								if(temp_size_16Bits!=0x0000)
								{
									dataSize=temp_size_16Bits;
									haveData_dataSize=true;
								}
							}
							break;
							case sizeof(quint16):
							{
								if(device->bytesAvailable()<(int)sizeof(quint32))
									return;
								data_size+=device->read(sizeof(quint32));
								in_size >> temp_size_32Bits;
								if(temp_size_32Bits!=0x00000000)
								{
									dataSize=temp_size_32Bits;
									haveData_dataSize=true;
								}
								emit error("size is null");
								return;
							}
							break;
							default:
							emit error("size not understand, internal bug");
							return;
						}
					}
				}
			}
		}
		#ifdef POKECRAFT_EXTRA_CHECK
		if(!haveData_dataSize)
		{
			emit error("have not the size here!");
			return;
		}
		#endif
		if(dataSize>16*1024*1024)
		{
			emit error("packet size too big");
			return;
		}
		if(dataSize>0)
		{
			if((dataSize-data.size())<=device->bytesAvailable())
				data.append(device->read(dataSize-data.size()));
			else
			{
				data.append(device->readAll());
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
		if(!need_query_number)
		{
			if(!need_subCodeType)
				parseMessage(mainCodeType,data);
			else
				parseMessage(mainCodeType,subCodeType,data);
		}
		else
		{
			if(is_reply)
			{
				if(!need_subCodeType)
					parseQuery(mainCodeType,queryNumber,data);
				else
					parseQuery(mainCodeType,subCodeType,queryNumber,data);
			}
			else
			{
				if(!reply_subCodeType.contains(queryNumber))
				{
					mainCodeType=reply_mainCodeType[queryNumber];
					reply_mainCodeType.remove(queryNumber);
					parseReplyData(mainCodeType,queryNumber,data);
				}
				else
				{
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
		out << encodeSize(data.size());
	else
		replySize.remove(queryNumber);

	return internalPackOutcommingData(data);
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
	DebugClass::debugConsole("internalPackOutcommingData(): start");
	#ifdef DEBUG_MESSAGE_CLIENT_RAW_NETWORK
	emit message(QString("Sended packet size: %1").arg(data.size()+sizeof(qint32)));
	#endif // DEBUG_MESSAGE_CLIENT_RAW_NETWORK
	//emit message(QString("data: %1").arg(QString(data.toHex())));
	byteWriten = device->write(data);
	if(data.size()!=byteWriten)
	{
		DebugClass::debugConsole(QString("All the bytes have not be written: %1, byteWriten: %2").arg(device->errorString()).arg(byteWriten));
		emit error(QString("All the bytes have not be written: %1, byteWriten: %2").arg(device->errorString()).arg(byteWriten));
		return false;
	}
	return true;
}

QByteArray ProtocolParsingOutput::encodeSize(quint32 size)
{
	QByteArray block;
	QDataStream out(&block, QAbstractSocket::WriteOnly);
	if(packetSizeMode==PacketSizeMode_Small)
	{
		if(size<=0xFF)
			out << quint8(size);
		else if(size<=0xFFFF)
			out << quint8(0x00) << quint16(size);
		else
			out << quint8(0x0000) << quint32(size);
	}
	else
	{
		if(size<=0xFFFF)
			out << quint16(size);
		else
			out << quint8(0x0000) << quint32(size);
	}
	return block;
}
