#ifndef CATCHCHALLENGER_API_CLIENT_REAL_H
#define CATCHCHALLENGER_API_CLIENT_REAL_H

#include "../../general/base/GeneralVariable.h"
#include "DatapackChecksum.h"

#include <QObject>
#include <QString>
#include <QCoreApplication>
#include <QAbstractSocket>
#include <QString>
#include <QByteArray>
#include <QList>
#include <QPair>
#include <QDir>
#include <QHash>
#include <QFileInfo>
#include <QDateTime>
#include <QCryptographicHash>
#include <QNetworkAccessManager>
#include <QNetworkProxy>

#include "../../general/base/DebugClass.h"
#include "../../general/base/GeneralStructures.h"
#include "ClientStructures.h"
#include "Api_protocol.h"
#include "qt-tar-xz/QXzDecodeThread.h"

namespace CatchChallenger {
class Api_client_real : public Api_protocol
{
    Q_OBJECT
public:
    explicit Api_client_real(ConnectedSocket *socket,bool tolerantMode=false);
    ~Api_client_real();
    void resetAll();

    //connection related
    void tryConnect(QString host,quint16 port);
    void tryDisconnect();
    QString getHost();
    quint16 getPort();

    //datapack related
    void sendDatapackContent();
    void test_mirror();
    void httpErrorEvent();
    void decodedIsFinish();
    bool mirrorTryNext();
    void httpFinishedForDatapackList();
    const QStringList listDatapack(QString suffix);
    void cleanDatapack(QString suffix);
    void setProxy(const QNetworkProxy &proxy);
protected:
    void parseFullReplyData(const quint8 &mainCodeType,const quint8 &subCodeType,const quint8 &queryNumber,const QByteArray &data);
    void parseFullReplyData(const quint8 &mainCodeType,const quint8 &subCodeType,const quint8 &queryNumber,const char * const data,const unsigned int &size);

    //general data
    void defineMaxPlayers(const quint16 &maxPlayers);
private:
    static QRegularExpression regex_DATAPACK_FILE_REGEX;
    QXzDecodeThread xzDecodeThread;
    bool datapackTarXz;
    CatchChallenger::DatapackChecksum datapackChecksum;
    QString host;
    quint16 port;
    quint64 RXSize,TXSize;
    int index_mirror;
    bool test_with_proxy;
    QNetworkProxy proxy;

    //file list
    struct query_files
    {
        quint8 id;
        QStringList filesName;
    };
    QList<query_files> query_files_list;
    bool wait_datapack_content;
    QStringList datapackFilesList;
    QList<quint32> partialHashList;
    static QString text_slash;
    static QString text_dotcoma;
    bool httpMode,httpError;
    int qnamQueueCount,qnamQueueCount2,qnamQueueCount3,qnamQueueCount4;
    QNetworkAccessManager qnam;
    QNetworkAccessManager qnam2;
    QNetworkAccessManager qnam3;
    QNetworkAccessManager qnam4;
    struct UrlInWaiting
    {
        QString fileName;
    };
    QHash<QNetworkReply *,UrlInWaiting> urlInWaitingList;
private slots:
    void disconnected();
    void writeNewFile(const QString &fileName, const QByteArray &data);
    void getHttpFile(const QString &url, const QString &fileName);
    void httpFinished();
    void datapackChecksumDone(const QStringList &datapackFilesList,const QByteArray &hash, const QList<quint32> &partialHash);
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
signals:
    void newDatapackFile(const quint32 &size) const;
    void doDifferedChecksum(const QString &datapackPath);
    void progressingDatapackFile(const quint32 &size);
};
}

#endif // Protocol_and_data_H
