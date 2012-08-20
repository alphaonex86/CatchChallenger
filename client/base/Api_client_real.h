#ifndef Protocol_and_data_H
#define Protocol_and_data_H

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

#include "../../general/base/DebugClass.h"
#include "../../general/base/GeneralStructures.h"
#include "../../general/base/GeneralVariable.h"
#include "ClientStructures.h"
#include "Api_protocol.h"

namespace Pokecraft {
class Api_client_real : public Api_protocol
{
	Q_OBJECT
public:
	explicit Api_client_real();
	~Api_client_real();

	//connection related
	void tryConnect(QString host,quint16 port);
	void tryDisconnect();
	QString getHost();
	quint16 getPort();

	//datapack related
	void sendDatapackContent();
	const QStringList listDatapack(QString suffix);
	void cleanDatapack(QString suffix);
	QString get_datapack_base_name();
protected:
	void parseReplyData(const quint8 &mainCodeType,const quint16 &subCodeType,const quint8 &queryNumber,const QByteArray &data);
	void resetAll();
private:
	QString host;
	quint16 port;
	QTcpSocket socket;

	//file list
	struct query_files
	{
		quint8 id;
		QStringList filesName;
	};
	QList<query_files> query_files_list;
	QString datapack_base_name;
	bool wait_datapack_content;
	QStringList datapackFilesList;
signals:
	void stateChanged(QAbstractSocket::SocketState socketState);
	void error(QAbstractSocket::SocketError socketError);

	void haveTheDatapack();
private slots:
	void disconnected();
};
}

#endif // Protocol_and_data_H
