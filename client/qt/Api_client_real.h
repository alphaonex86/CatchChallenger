#ifndef CATCHCHALLENGER_API_CLIENT_REAL_H
#define CATCHCHALLENGER_API_CLIENT_REAL_H

#include "../../general/base/GeneralVariable.h"
#include "../libcatchchallenger/DatapackChecksum.h"

#include <QObject>
#include <string>
#include <QCoreApplication>
#include <QAbstractSocket>
#include <vector>
#include <QPair>
#include <QDir>
#include <unordered_map>
#include <QFileInfo>
#include <QDateTime>
#include <QCryptographicHash>
#include <QNetworkAccessManager>
#include <QNetworkProxy>
#include <QRegularExpression>
#include <QDataStream>

#include "../../general/base/GeneralStructures.h"
#include "ClientStructures.h"
#include "Api_protocol_Qt.h"
#include "qt-tar-compressed/QZstdDecodeThread.h"

namespace CatchChallenger {
class Api_client_real : public Api_protocol_Qt
{
    Q_OBJECT
public:
    explicit Api_client_real(ConnectedSocket *socket,bool tolerantMode=false);
    ~Api_client_real();
    void resetAll();
    void closeDownload();

    //connection related
    void tryConnect(std::string host,uint16_t port);
    void tryDisconnect();
    std::string getHost();
    uint16_t getPort();

    //datapack related
    void sendDatapackContentBase(const std::string &hashBase=std::string());
    void sendDatapackContentMainSub(const std::string &hashMain=std::string(),const std::string &hashSub=std::string());
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
    bool mirrorTryNextBase(const std::string &error);
    bool mirrorTryNextMain(const std::string &error);
    bool mirrorTryNextSub(const std::string &error);
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
    bool parseReplyData(const uint8_t &mainCodeType,const uint8_t &queryNumber,const std::string &data);
    bool parseReplyData(const uint8_t &mainCodeType,const uint8_t &queryNumber,const char * const data,const unsigned int &size);

    //general data
    void defineMaxPlayers(const uint16_t &maxPlayers);
private:
    static QRegularExpression regex_DATAPACK_FILE_REGEX;
    /// \todo group into one thread by change for queue
    QZstdDecodeThread zstdDecodeThreadBase;
    QZstdDecodeThread zstdDecodeThreadMain;
    QZstdDecodeThread zstdDecodeThreadSub;
    bool datapackTarBase;
    bool datapackTarMain;
    bool datapackTarSub;
    CatchChallenger::DatapackChecksum datapackChecksum;
    std::string host;
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
        std::vector<std::string> filesName;
    };
    std::vector<query_files> query_files_list_base;
    std::vector<query_files> query_files_list_main;
    std::vector<query_files> query_files_list_sub;
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
        std::string fileName;
    };
    std::unordered_map<QNetworkReply *,UrlInWaiting> urlInWaitingListBase,urlInWaitingListMain,urlInWaitingListSub;
private slots:
    void writeNewFileBase(const std::string &fileName, const std::string &data);
    void writeNewFileMain(const std::string &fileName, const std::string &data);
    void writeNewFileSub(const std::string &fileName, const std::string &data);
    void checkIfContinueOrFinished();
    bool getHttpFileBase(const std::string &url, const std::string &fileName);
    bool getHttpFileMain(const std::string &url, const std::string &fileName);
    bool getHttpFileSub(const std::string &url, const std::string &fileName);
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
    //protocol/connection info
    void disconnect();
};
}

#endif // Protocol_and_data_H
