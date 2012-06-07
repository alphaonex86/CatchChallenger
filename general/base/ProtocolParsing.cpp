#include "ProtocolParsing.h"

//connexion parameters
PacketSizeMode ProtocolParsing::packetSizeMode;

QSet<quint8> ProtocolParsingInput::mainCodeWithoutSubCodeType;//if need sub code or not
//if is a query
QSet<quint8> ProtocolParsingInput::mainCode_IsQuery;
//predefined size
QHash<quint8,quint16> ProtocolParsingInput::sizeOnlyMainCodePacket;
QHash<quint8,QHash<quint16,quint16> > ProtocolParsingInput::sizeMultipleCodePacket;

QSet<quint8> ProtocolParsingOutput::mainCodeWithoutSubCodeType;//if need sub code or not
//if is a query
QSet<quint8> ProtocolParsingOutput::mainCode_IsQuery;
//predefined size
QHash<quint8,quint16> ProtocolParsingOutput::sizeOnlyMainCodePacket;
QHash<quint8,QHash<quint16,quint16> > ProtocolParsingOutput::sizeMultipleCodePacket;

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
		ProtocolParsingOutput::mainCodeWithoutSubCodeType << 0x40;
		ProtocolParsingInput::mainCodeWithoutSubCodeType << 0xC3 << 0xC4;
		ProtocolParsingOutput::sizeOnlyMainCodePacket[0x40]=2;
		ProtocolParsingInput::sizeOnlyMainCodePacket[0xC3]=4;
		ProtocolParsingInput::sizeOnlyMainCodePacket[0xC4]=0;
	}
	else
	{
		ProtocolParsingOutput::mainCodeWithoutSubCodeType << 0xC3 << 0xC4;
		ProtocolParsingInput::mainCodeWithoutSubCodeType << 0x40;
		ProtocolParsingOutput::sizeOnlyMainCodePacket[0xC4]=0;
		ProtocolParsingOutput::sizeOnlyMainCodePacket[0xC3]=4;
		ProtocolParsingInput::sizeOnlyMainCodePacket[0x40]=2;

		ProtocolParsingInput::mainCode_IsQuery << 0x02;
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
			need_subCodeType=!mainCodeWithoutSubCodeType.contains(mainCodeType);
			need_query_number=mainCode_IsQuery.contains(mainCodeType);
			data_size.clear();
		}
		if(haveData)
		{
			if(!haveData_dataSize)
			{
				if(need_subCodeType)
				{
					if(sizeOnlyMainCodePacket.contains(mainCodeType))
					{
						//have fixed size
						dataSize=sizeOnlyMainCodePacket[mainCodeType];
						haveData_dataSize=true;
					}
				}
				else
				{
					if(device->bytesAvailable()<(int)sizeof(quint16))//ignore because first int is cuted!
						return;
					in >> subCodeType;
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
			if(!haveData_dataSize)
			{
				emit error("have not the size here!");
				return;
			}
			if(need_query_number)
			{
				if(device->bytesAvailable()<(int)sizeof(quint8))
					return;
				in >> queryNumber;
			}
			if(dataSize>0)
			{
				if((dataSize-data.size())<device->bytesAvailable())
				{
					//DebugClass::debugConsole(QString("dataSize(%1)-data.size(%2)=%3").arg(dataSize).arg(data.size()).arg(dataSize-data.size()));
					data.append(device->read(dataSize-data.size()));
				}
				else
					data.append(device->readAll());
			}
			if(dataSize==(quint32)data.size())
			{
				if(need_query_number)
				{
					if(need_subCodeType)
						parseMessage(mainCodeType,data);
					else
						parseMessage(mainCodeType,subCodeType,data);
				}
				else
				{
					if(need_subCodeType)
						parseQuery(mainCodeType,queryNumber,data);
					else
						parseQuery(mainCodeType,subCodeType,queryNumber,data);

				}
				dataClear();
			}
		}
	}
}

void ProtocolParsingInput::dataClear()
{
	data.clear();
	dataSize=0;
	haveData=false;
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

bool ProtocolParsingOutput::packOutcommingData(const quint8 &mainCodeType,const quint8 &queryNumber,const QByteArray &data)
{
	QByteArray block;
	QDataStream out(&block, QIODevice::WriteOnly);
	out << queryNumber;

	return packOutcommingData(mainCodeType,subCodeType,packetSize,block+data);
}

bool ProtocolParsingOutput::packOutcommingData(const quint8 &mainCodeType,const QByteArray &data)
{
	return packOutcommingData(mainCodeType,subCodeType,packetSize,block+data);
}

bool ProtocolParsingOutput::internalPackOutcommingData(const quint8 &mainCodeType,const QByteArray &data)
{
	//can't group this block to keep the right header size calculation
	QByteArray block;
	QDataStream out(&block, QIODevice::WriteOnly);
	out << qint32(mainCodeType);

	#ifdef POKECRAFT_EXTRA_CHECK
	if(!mainCodeWithoutSubCodeType.contains(mainCodeType))
	{
		if(sizeMultipleCodePacket.contains(mainCodeType))
		{
			if(sizeMultipleCodePacket[mainCodeType].contains(subCodeType))
			{
				if(sizeMultipleCodePacket[mainCodeType][subCodeType]!=data.size())
				{
					emit error("Data size is not valid");
					return false;
				}
			}
			else
			{
				if(!packetSize)
				{
					emit error("Packet size is not fix");
					return false;
				}
			}
		}
		else
		{
			if(!packetSize)
			{
				emit error("Packet size is not fix");
				return false;
			}
		}
	}
	else
	{
		if(sizeOnlyMainCodePacket.contains(mainCodeType))
		{
			if(sizeOnlyMainCodePacket[mainCodeType]!=data.size())
			{
				emit error("Data size is not valid");
				return false;
			}
		}
		else
		{
			if(!packetSize && mainCodeType!=0xC1)
			{
				emit error("Packet size is not fix");
				return false;
			}
		}
	}
	#endif

	block+=data;

	if(device!=NULL)
	{
		#ifdef DEBUG_MESSAGE_CLIENT_RAW_NETWORK
		emit message(QString("Sended packet size: %1").arg(data.size()+sizeof(qint32)));
		#endif // DEBUG_MESSAGE_CLIENT_RAW_NETWORK
		//emit message(QString("data: %1").arg(QString(block.toHex())));
		byteWriten = device->write(block);
		if(block.size()!=byteWriten)
		{
			emit error(QString("All the bytes have not be written: %1").arg(device->errorString()));
			return false;
		}
	}
	else
		emit fake_send_data(block);
	return true;
}
