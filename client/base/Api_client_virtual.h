#ifndef POKECRAFT_API_CLIENT_VIRTUAL_H
#define POKECRAFT_API_CLIENT_VIRTUAL_H

#include <QObject>
#include <QString>
#include <QCoreApplication>
#include <QTcpSocket>
#include <QString>
#include <QByteArray>
#include <QList>
#include <QPair>
#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QDateTime>
#include <QCryptographicHash>

#include "../../general/base/QFakeSocket.h"
#include "../../general/base/DebugClass.h"
#include "../../general/base/GeneralStructures.h"
#include "../../general/base/GeneralVariable.h"
#include "ClientStructures.h"
#include "Api_protocol.h"

namespace Pokecraft {
class Api_client_virtual : public Api_protocol
{
	Q_OBJECT
public:
    explicit Api_client_virtual(QAbstractSocket *socket);
	~Api_client_virtual();
    void sendDatapackContent();
    void tryDisconnect();
};
}

#endif // Protocol_and_virtual_data_H
