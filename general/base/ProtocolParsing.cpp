#include "ProtocolParsing.h"

QSet<quint8> ProtocolParsing::getMainCodeWithoutSubCodeTypeClientToServer()
{
	QSet<quint8> mainCodeWithoutSubCodeTypeClientToServer;
	mainCodeWithoutSubCodeTypeClientToServer << 0x40;
	return mainCodeWithoutSubCodeTypeClientToServer;
}

QSet<quint8> ProtocolParsing::getMainCodeWithoutSubCodeTypeServerToClient()
{
	QSet<quint8> mainCodeWithoutSubCodeTypeServerToClient;
	mainCodeWithoutSubCodeTypeServerToClient << 0xC3;
	mainCodeWithoutSubCodeTypeServerToClient << 0xC4;
	return mainCodeWithoutSubCodeTypeServerToClient;
}

QHash<quint8,quint16> ProtocolParsing::getSizeOnlyMainCodePacketClientToServer()
{
	QHash<quint8,quint16> returnedList;
	returnedList[0x40]=2;
	return returnedList;
}

QHash<quint8,quint16> ProtocolParsing::getSizeOnlyMainCodePacketServerToClient()
{
	QHash<quint8,quint16> returnedList;
	returnedList[0xC3]=4;
	returnedList[0xC4]=0;
	return returnedList;
}

QHash<quint8,QHash<quint16,quint16> > ProtocolParsing::getSizeMultipleCodePacketClientToServer()
{
	QHash<quint8,QHash<quint16,quint16> > returnedList;
	return returnedList;
}

QHash<quint8,QHash<quint16,quint16> > ProtocolParsing::getSizeMultipleCodePacketServerToClient()
{
	QHash<quint8,QHash<quint16,quint16> > returnedList;
	return returnedList;
}
