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
#include <QRegularExpression>

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
    void sendDatapackContentBase();
    void sendDatapackContentMainSub();
    void sendDatapackContentMain();
    void sendDatapackContentSub();
    void test_mirror_base();
    void test_mirror_main();
    void test_mirror_sub();
    void httpErrorEventBase();
    void httpErrorEventMain();
    void httpErrorEventSub();
    void decodedIsFinishBase();
    void decodedIsFinishMain();
    void decodedIsFinishSub();
    bool mirrorTryNextBase();
    bool mirrorTryNextMain();
    bool mirrorTryNextSub();
    void httpFinishedForDatapackListBase();
    void httpFinishedForDatapackListMain();
    void httpFinishedForDatapackListSub();
    const QStringList listDatapackBase(QString suffix);
    const QStringList listDatapackMain(QString suffix);
    const QStringList listDatapackSub(QString suffix);
    void cleanDatapackBase(QString suffix);
    void cleanDatapackMain(QString suffix);
    void cleanDatapackSub(QString suffix);
    void setProxy(const QNetworkProxy &proxy);
protected:
    void parseFullReplyData(const quint8 &mainCodeType,const quint8 &subCodeType,const quint8 &queryNumber,const QByteArray &data);
    void parseFullReplyData(const quint8 &mainCodeType,const quint8 &subCodeType,const quint8 &queryNumber,const char * const data,const unsigned int &size);

    //general data
    void defineMaxPlayers(const quint16 &maxPlayers);
private:
    static QRegularExpression regex_DATAPACK_FILE_REGEX;
    /// \todo group into one thread by change for queue
    QXzDecodeThread xzDecodeThreadBase;
    QXzDecodeThread xzDecodeThreadMain;
    QXzDecodeThread xzDecodeThreadSub;
    bool datapackTarXzBase;
    bool datapackTarXzMain;
    bool datapackTarXzSub;
    CatchChallenger::DatapackChecksum datapackChecksum;
    QString host;
    quint16 port;
    quint64 RXSize,TXSize;
    int index_mirror_base;
    int index_mirror_main;
    int index_mirror_sub;
    QNetworkProxy proxy;
    static QRegularExpression excludePathBase;
    static QRegularExpression excludePathMain;
    enum DatapackStatus
    {
        Base=0x01,
        Main=0x02,
        Sub=0x03,
        Finished=0x04
    };
    DatapackStatus datapackStatus;

    //file list
    struct query_files
    {
        quint8 id;
        QStringList filesName;
    };
    QList<query_files> query_files_list_base;
    QList<query_files> query_files_list_main;
    QList<query_files> query_files_list_sub;
    bool wait_datapack_content_base;
    bool wait_datapack_content_main;
    bool wait_datapack_content_sub;
    QStringList datapackFilesListBase;
    QStringList datapackFilesListMain;
    QStringList datapackFilesListSub;
    QList<quint32> partialHashListBase;
    QList<quint32> partialHashListMain;
    QList<quint32> partialHashListSub;
    static QString text_slash;
    static QString text_dotcoma;
    bool httpError;
    bool httpModeBase;
    bool httpModeMain;
    bool httpModeSub;
    int qnamQueueCount,qnamQueueCount2,qnamQueueCount3,qnamQueueCount4;
    QNetworkAccessManager qnam;
    QNetworkAccessManager qnam2;
    QNetworkAccessManager qnam3;
    QNetworkAccessManager qnam4;
    struct UrlInWaiting
    {
        QString fileName;
    };
    QHash<QNetworkReply *,UrlInWaiting> urlInWaitingListBase,urlInWaitingListMain,urlInWaitingListSub;
private slots:
    void disconnected();
    void writeNewFileBase(const QString &fileName, const QByteArray &data);
    void writeNewFileMain(const QString &fileName, const QByteArray &data);
    void writeNewFileSub(const QString &fileName, const QByteArray &data);
    void checkIfContinueOrFinished();
    void getHttpFileBase(const QString &url, const QString &fileName);
    void getHttpFileMain(const QString &url, const QString &fileName);
    void getHttpFileSub(const QString &url, const QString &fileName);
    void httpFinishedBase();
    void httpFinishedMain();
    void httpFinishedSub();
    void datapackDownloadFinishedBase();
    void datapackDownloadFinishedMain();
    void datapackDownloadFinishedSub();
    void datapackChecksumDoneBase(const QStringList &datapackFilesList,const QByteArray &hash, const QList<quint32> &partialHash);
    void datapackChecksumDoneMain(const QStringList &datapackFilesList,const QByteArray &hash, const QList<quint32> &partialHash);
    void datapackChecksumDoneSub(const QStringList &datapackFilesList,const QByteArray &hash, const QList<quint32> &partialHash);
    void downloadProgressDatapackBase(qint64 bytesReceived, qint64 bytesTotal);
    void downloadProgressDatapackMain(qint64 bytesReceived, qint64 bytesTotal);
    void downloadProgressDatapackSub(qint64 bytesReceived, qint64 bytesTotal);
signals:
    void newDatapackFileBase(const quint32 &size) const;
    void newDatapackFileMain(const quint32 &size) const;
    void newDatapackFileSub(const quint32 &size) const;
    void doDifferedChecksumBase(const QString &datapackPath);
    void doDifferedChecksumMain(const QString &datapackPath);
    void doDifferedChecksumSub(const QString &datapackPath);
    void progressingDatapackFileBase(const quint32 &size);
    void progressingDatapackFileMain(const quint32 &size);
    void progressingDatapackFileSub(const quint32 &size);
};
}

#endif // Protocol_and_data_H
