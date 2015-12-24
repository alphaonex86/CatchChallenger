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
#include <QDataStream>

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
    void tryConnect(QString host,uint16_t port);
    void tryDisconnect();
    QString getHost();
    uint16_t getPort();

    //datapack related
    void sendDatapackContentBase(const QByteArray &hashBase=QByteArray());
    void sendDatapackContentMainSub(const QByteArray &hashMain=QByteArray(),const QByteArray &hashSub=QByteArray());
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
    const std::vector<std::string> listDatapackBase(std::string suffix="");
    const std::vector<std::string> listDatapackMain(std::string suffix="");
    const std::vector<std::string> listDatapackSub(std::string suffix="");
    void cleanDatapackBase(std::string suffix="");
    void cleanDatapackMain(std::string suffix="");
    void cleanDatapackSub(std::string suffix="");
    void setProxy(const QNetworkProxy &proxy);
protected:
    bool parseReplyData(const uint8_t &mainCodeType,const uint8_t &queryNumber,const QByteArray &data);
    bool parseReplyData(const uint8_t &mainCodeType,const uint8_t &queryNumber,const char * const data,const unsigned int &size);

    //general data
    void defineMaxPlayers(const uint16_t &maxPlayers);
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
    uint16_t port;
    uint64_t RXSize,TXSize;
    int index_mirror_base;
    int index_mirror_main;
    int index_mirror_sub;
    QNetworkProxy proxy;
    static std::regex excludePathBase;
    static std::regex excludePathMain;

    //file list
    struct query_files
    {
        uint8_t id;
        QStringList filesName;
    };
    QList<query_files> query_files_list_base;
    QList<query_files> query_files_list_main;
    QList<query_files> query_files_list_sub;
    bool wait_datapack_content_base;
    bool wait_datapack_content_main;
    bool wait_datapack_content_sub;
    std::vector<std::string> datapackFilesListBase;
    std::vector<std::string> datapackFilesListMain;
    std::vector<std::string> datapackFilesListSub;
    std::vector<uint32_t> partialHashListBase;
    std::vector<uint32_t> partialHashListMain;
    std::vector<uint32_t> partialHashListSub;
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
    void datapackChecksumDoneBase(const std::vector<std::string> &datapackFilesList, const std::vector<char> &hash, const std::vector<uint32_t> &partialHash);
    void datapackChecksumDoneMain(const std::vector<std::string> &datapackFilesList,const std::vector<char> &hash,const std::vector<uint32_t> &partialHashList);
    void datapackChecksumDoneSub(const std::vector<std::string> &datapackFilesList,const std::vector<char> &hash,const std::vector<uint32_t> &partialHashList);
    void downloadProgressDatapackBase(int64_t bytesReceived, int64_t bytesTotal);
    void downloadProgressDatapackMain(int64_t bytesReceived, int64_t bytesTotal);
    void downloadProgressDatapackSub(int64_t bytesReceived, int64_t bytesTotal);
signals:
    void newDatapackFileBase(const uint32_t &size) const;
    void newDatapackFileMain(const uint32_t &size) const;
    void newDatapackFileSub(const uint32_t &size) const;
    void doDifferedChecksumBase(const std::string &datapackPath);
    void doDifferedChecksumMain(const std::string &datapackPath);
    void doDifferedChecksumSub(const std::string &datapackPath);
    void progressingDatapackFileBase(const uint32_t &size);
    void progressingDatapackFileMain(const uint32_t &size);
    void progressingDatapackFileSub(const uint32_t &size);
};
}

#endif // Protocol_and_data_H
