#ifndef CATCHCHALLENGER_API_CLIENT_REAL_H
#define CATCHCHALLENGER_API_CLIENT_REAL_H

#include <QObject>
#include <QString>
#include <QCoreApplication>
#include <QAbstractSocket>
#include <QString>
#include <QByteArray>
#include <QList>
#include <QPair>
#include <QDir>
#include <QFileInfo>
#include <QDateTime>
#include <QCryptographicHash>

#include "../../general/base/DebugClass.h"
#include "../../general/base/GeneralStructures.h"
#include "../../general/base/GeneralVariable.h"
#include "ClientStructures.h"
#include "../../general/base/Api_protocol.h"

namespace CatchChallenger {
class Api_client_real : public Api_protocol
{
    Q_OBJECT
public:
    explicit Api_client_real(ConnectedSocket *socket,bool tolerantMode=false);
    ~Api_client_real();
    static Api_protocol *client;
    void resetAll();

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

    //general data
    void defineMaxPlayers(const quint16 &maxPlayers);
private:
    QString host;
    quint16 port;
    quint64 RXSize,TXSize;

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
    void writeNewFile(const QString &fileName, const QByteArray &data, const quint64 &mtime);
};
}

#endif // Protocol_and_data_H
