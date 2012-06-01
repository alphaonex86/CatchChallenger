#ifndef PROTOCOLPARSING_H
#define PROTOCOLPARSING_H

#include <QSet>
#include <QHash>

class ProtocolParsing
{
public:
	static QSet<quint8> getMainCodeWithoutSubCodeTypeClientToServer();
	static QSet<quint8> getMainCodeWithoutSubCodeTypeServerToClient();
	static QHash<quint8,quint16> getSizeOnlyMainCodePacketClientToServer();
	static QHash<quint8,quint16> getSizeOnlyMainCodePacketServerToClient();
	static QHash<quint8,QHash<quint16,quint16> > getSizeMultipleCodePacketClientToServer();
	static QHash<quint8,QHash<quint16,quint16> > getSizeMultipleCodePacketServerToClient();
};

#endif // PROTOCOLPARSING_H
