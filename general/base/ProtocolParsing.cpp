#include "ProtocolParsing.h"

//connexion parameters
PacketSizeMode ProtocolParsing::packetSizeMode;

QSet<quint8> ProtocolParsingInput::mainCodeWithoutSubCodeType;//if need sub code or not
//if is a query
QSet<quint8> ProtocolParsingInput::mainCode_IsQuery;
quint8 ProtocolParsingInput::replyCode;
//predefined size
QHash<quint8,quint16> ProtocolParsingInput::sizeOnlyMainCodePacket;
QHash<quint8,QHash<quint16,quint16> > ProtocolParsingInput::sizeMultipleCodePacket;
QHash<quint8,quint16> ProtocolParsingInput::replySizeOnlyMainCodePacket;
QHash<quint8,QHash<quint16,quint16> > ProtocolParsingInput::replySizeMultipleCodePacket;

QSet<quint8> ProtocolParsingOutput::mainCodeWithoutSubCodeType;//if need sub code or not
//if is a query
QSet<quint8> ProtocolParsingOutput::mainCode_IsQuery;
quint8 ProtocolParsingOutput::replyCode;
//predefined size
QHash<quint8,quint16> ProtocolParsingOutput::sizeOnlyMainCodePacket;
QHash<quint8,QHash<quint16,quint16> > ProtocolParsingOutput::sizeMultipleCodePacket;
QHash<quint8,quint16> ProtocolParsingOutput::replySizeOnlyMainCodePacket;
QHash<quint8,QHash<quint16,quint16> > ProtocolParsingOutput::replySizeMultipleCodePacket;

//temp data
qint64 ProtocolParsingOutput::byteWriten;
quint8 ProtocolParsingInput::temp_size_8Bits;
quint16 ProtocolParsingInput::temp_size_16Bits;
quint32 ProtocolParsingInput::temp_size_32Bits;

ProtocolParsing::ProtocolParsing(QIODevice * device)
{
	this->device=device;
}

void ProtocolParsing::initialiseTheVariable(PacketModeTransmission packetModeTransmission)
{
	if(packetModeTransmission==PacketModeTransmission_Client)
	{
		ProtocolParsingOutput::mainCodeWithoutSubCodeType << 0x40 << 0x51;
		ProtocolParsingInput::mainCodeWithoutSubCodeType << 0xC3 << 0xC4 << 0xD1;
		ProtocolParsingOutput::sizeOnlyMainCodePacket[0x40]=2;
		ProtocolParsingInput::sizeOnlyMainCodePacket[0xC3]=4;
		ProtocolParsingInput::sizeOnlyMainCodePacket[0xC4]=0;

		ProtocolParsingOutput::mainCode_IsQuery << 0x02;

		ProtocolParsingOutput::replyCode=0x41;
		ProtocolParsingInput::replyCode=0xC1;
	}
	else
	{
		ProtocolParsingOutput::mainCodeWithoutSubCodeType << 0xC3 << 0xC4 << 0xD1;
		ProtocolParsingInput::mainCodeWithoutSubCodeType << 0x40 << 0x51;
		ProtocolParsingOutput::sizeOnlyMainCodePacket[0xC4]=0;
		ProtocolParsingOutput::sizeOnlyMainCodePacket[0xC3]=4;
		ProtocolParsingInput::sizeOnlyMainCodePacket[0x40]=2;

		ProtocolParsingInput::mainCode_IsQuery << 0x02;

		ProtocolParsingOutput::replyCode=0xC1;
		ProtocolParsingInput::replyCode=0x41;
	}
}

ProtocolParsingInput::ProtocolParsingInput(QIODevice * device) :
	ProtocolParsing(device)
{
	dataClear();
}

ProtocolParsingOutput::ProtocolParsingOutput(QIODevice * device) :
	ProtocolParsing(device)
{
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
			need_subCodeType=!mainCodeWithoutSubCodeType.contains(mainCodeType);
			need_query_number=false;
			data_size.clear();
		}

		if(!have_subCodeType)
		{
			if(!need_subCodeType)
			{
				//if is a reply
				if(mainCodeType==replyCode)
				{
					is_reply=true;
					need_query_number=true;
					//the size with be resolved later
				}
				else
				{
					//if is query with reply
					if(mainCode_IsQuery.contains(mainCodeType))
						need_query_number=true;

					//check if have defined size
					if(sizeOnlyMainCodePacket.contains(mainCodeType))
					{
						//have fixed size
						dataSize=sizeOnlyMainCodePacket[mainCodeType];
						haveData_dataSize=true;
					}
				}
			}
			else
			{
				if(device->bytesAvailable()<(int)sizeof(quint16))//ignore because first int is cuted!
					return;
				in >> subCodeType;

				//if is query with reply
				if(mainCode_IsQuery.contains(mainCodeType))
					need_query_number=true;

				//check if have defined size
				if(sizeMultipleCodePacket.contains(mainCodeType))
				{
					if(sizeMultipleCodePacket[mainCodeType].contains(subCodeType))
					{
						//have fixed size
						dataSize=sizeMultipleCodePacket[mainCodeType][subCodeType];
						haveData_dataSize=true;
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
	if(replySizeOnlyMainCodePacket.contains(mainCodeType))
		replySize[queryNumber]=replySizeOnlyMainCodePacket[mainCodeType];
	reply_mainCodeType[queryNumber]=mainCodeType;
}

void ProtocolParsingInput::newOutputQuery(const quint8 &mainCodeType,const quint16 &subCodeType,const quint8 &queryNumber)
{
	if(reply_mainCodeType.contains(queryNumber))
	{
		emit error("Query with this query number already found");
		return;
	}
	if(replySizeMultipleCodePacket.contains(mainCodeType))
	{
		if(replySizeMultipleCodePacket[mainCodeType].contains(subCodeType))
			replySize[queryNumber]=replySizeMultipleCodePacket[mainCodeType][subCodeType];
	}
	reply_mainCodeType[queryNumber]=mainCodeType;
	reply_subCodeType[queryNumber]=subCodeType;
}

bool ProtocolParsingOutput::packOutcommingData(const quint8 &mainCodeType,const quint16 &subCodeType,const quint8 &queryNumber,const QByteArray &data)
{
	QByteArray block;
	QDataStream out(&block, QIODevice::WriteOnly);
	out << subCodeType;
	out << queryNumber;

	return packOutcommingData(mainCodeType,subCodeType,packetSize,block+data);
}

bool ProtocolParsingOutput::packOutcommingData(const quint8 &mainCodeType,const quint16 &subCodeType,const QByteArray &data)
{
	QByteArray block;
	QDataStream out(&block, QIODevice::WriteOnly);
	out << subCodeType;

	return packOutcommingData(mainCodeType,subCodeType,packetSize,block+data);
}

bool ProtocolParsingOutput::postReplyData(const quint8 &queryNumber,const QByteArray &data)
{
	if(replyNeedSize.contains(queryNumber))
	{
		QByteArray block;
		QDataStream out(&block, QIODevice::WriteOnly);
		out << replyCode;
		out << queryNumber;

		if(!replyNeedSize[queryNumber])
			out << encodeSize(data.size());

		replyNeedSize.remove(queryNumber);
		internalPackOutcommingData(data);
		return;
	}
	emit error("query number not found");
}

void ProtocolParsingOutput::newInputQuery(const quint8 &mainCodeType,const quint8 &queryNumber)
{
	if(replySize.contains(queryNumber))
	{
		emit error("Query with this query number already found");
		return;
	}
	if(replySizeOnlyMainCodePacket.contains(mainCodeType))
		replySize[queryNumber]=replySizeOnlyMainCodePacket[mainCodeType];
}

void ProtocolParsingOutput::newInputQuery(const quint8 &mainCodeType,const quint16 &subCodeType,const quint8 &queryNumber)
{
	if(replySize.contains(queryNumber))
	{
		emit error("Query with this query number already found");
		return;
	}
	if(replySizeMultipleCodePacket.contains(mainCodeType))
	{
		if(replySizeMultipleCodePacket[mainCodeType].contains(subCodeType))
			replySize[queryNumber]=replySizeMultipleCodePacket[mainCodeType][subCodeType];
	}
}

bool ProtocolParsingOutput::packOutcommingData(const quint8 &mainCodeType,const quint16 &subCodeType,const QByteArray &data)
{
	QByteArray block;
	QDataStream out(&block, QIODevice::WriteOnly);
	out << mainCodeType;
	out << subCodeType;

	if(!sizeMultipleCodePacket.contains(mainCodeType))
		block+=encodeSize(data.size());
	else if(!sizeMultipleCodePacket[mainCodeType].contains(subCodeType))
		block+=encodeSize(data.size());

	return internalPackOutcommingData(block+data);
}

bool ProtocolParsingOutput::packOutcommingData(const quint8 &mainCodeType,const QByteArray &data)
{
	QByteArray block;
	QDataStream out(&block, QIODevice::WriteOnly);
	out << mainCodeType;

	if(!sizeOnlyMainCodePacket.contains(mainCodeType))
		block+=encodeSize(data.size());

	return internalPackOutcommingData(block+data);
}

bool ProtocolParsingOutput::packOutcommingData(const quint8 &mainCodeType,const quint8 &queryNumber,const QByteArray &data)
{
	emit newOutputQuery(mainCodeType,queryNumber);

	QByteArray block;
	QDataStream out(&block, QIODevice::WriteOnly);
	out << mainCodeType;
	out << queryNumber;

	if(!sizeOnlyMainCodePacket.contains(mainCodeType))
		block+=encodeSize(data.size());

	return internalPackOutcommingData(block+data);
}

bool ProtocolParsingOutput::packOutcommingData(const quint8 &mainCodeType,const quint16 &subCodeType,const quint8 &queryNumber,const QByteArray &data)
{
	emit newOutputQuery(mainCodeType,subCodeType,queryNumber);

	QByteArray block;
	QDataStream out(&block, QIODevice::WriteOnly);
	out << mainCodeType;
	out << subCodeType;
	out << queryNumber;

	if(!sizeMultipleCodePacket.contains(mainCodeType))
		block+=encodeSize(data.size());
	else if(!sizeMultipleCodePacket[mainCodeType].contains(subCodeType))
		block+=encodeSize(data.size());

	return internalPackOutcommingData(block+data);
}

bool ProtocolParsingOutput::internalPackOutcommingData(const quint8 &mainCodeType,const QByteArray &data)
{
	if(device!=NULL)
	{
		#ifdef DEBUG_MESSAGE_CLIENT_RAW_NETWORK
		emit message(QString("Sended packet size: %1").arg(data.size()+sizeof(qint32)));
		#endif // DEBUG_MESSAGE_CLIENT_RAW_NETWORK
		//emit message(QString("data: %1").arg(QString(data.toHex())));
		byteWriten = device->write(data);
		if(data.size()!=byteWriten)
		{
			emit error(QString("All the bytes have not be written: %1").arg(device->errorString()));
			return false;
		}
	}
	else
		emit fake_send_data(data);
	return true;
}

QByteArray ProtocolParsingOutput::encodeSize(quint32 size)
{
	QByteArray block;
	QDataStream out(&block, QIODevice::WriteOnly);
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
