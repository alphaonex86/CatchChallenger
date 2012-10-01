#ifndef POKECRAFT_API_CLIENT_REAL_H
#define POKECRAFT_API_CLIENT_REAL_H

#include <QObject>
#include <QString>
#include <QCoreApplication>
#include <QAbstractSocket>
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
	explicit Api_client_real(QAbstractSocket *socket);
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
protected:
	void parseReplyData(const quint8 &mainCodeType,const quint16 &subCodeType,const quint8 &queryNumber,const QByteArray &data);
	void resetAll();
private:
	QString host;
	quint16 port;

	//file list
	struct query_files
	{
		quint8 id;
		QStringList filesName;
	};
	QList<query_files> query_files_list;
	bool wait_datapack_content;
    QStringList datapackFilesList;
private slots:
	void disconnected();
	void writeNewFile(const QString &fileName,const QByteArray &data,const quint32 &mtime);
};
}

#endif // Protocol_and_data_H
