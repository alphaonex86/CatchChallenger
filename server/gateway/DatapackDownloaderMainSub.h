#ifndef CATCHCHALLENGER_DatapackDownloaderMainSub_H
#define CATCHCHALLENGER_DatapackDownloaderMainSub_H

#include "../../general/base/GeneralVariable.h"
#include "../../client/base/DatapackChecksum.h"

#include <QObject>
#include <string>
#include <QCoreApplication>
#include <QAbstractSocket>
#include <string>
#include <QByteArray>
#include <vector>
#include <QDir>
#include <unordered_map>
#include <QFileInfo>
#include <QDateTime>
#include <QCryptographicHash>
#include <regex>

#include <curl/curl.h>

#include "../../general/base/GeneralStructures.h"
#include "../../client/base/qt-tar-xz/QXzDecodeThread.h"

namespace CatchChallenger {
class DatapackDownloaderMainSub
{
public:
    explicit DatapackDownloaderMainSub(const std::string &mDatapackBase, const std::string &mainDatapackCode, const std::string &subDatapackCode);
    virtual ~DatapackDownloaderMainSub();
    static std::unordered_map<std::string,std::unordered_map<std::string,DatapackDownloaderMainSub *> > datapackDownloaderMainSub;
    std::vector<void *> clientInSuspend;
    void datapackDownloadError();
    void resetAll();
    void datapackFileList(const char * const data,const unsigned int &size);
    void writeNewFileToRoute(const std::string &fileName, const QByteArray &data);

    //datapack related
    void sendDatapackContentMainSub();
    void sendDatapackContentMain();
    void sendDatapackContentSub();
    void test_mirror_main();
    void test_mirror_sub();
    void httpErrorEventMain();
    void httpErrorEventSub();
    void decodedIsFinishMain();
    void decodedIsFinishSub();
    bool mirrorTryNextMain();
    bool mirrorTryNextSub();
    void httpFinishedForDatapackListMain(const QByteArray data=QByteArray());
    void httpFinishedForDatapackListSub(const QByteArray data=QByteArray());
    const std::vector<std::string> listDatapackMain(std::string suffix);
    const std::vector<std::string> listDatapackSub(std::string suffix);
    void cleanDatapackMain(std::string suffix);
    void cleanDatapackSub(std::string suffix);

    const std::string mDatapackBase;
    const std::string mDatapackMain;
    std::string mDatapackSub;
    QByteArray hashMain;
    QByteArray hashSub;
    QByteArray sendedHashMain;
    QByteArray sendedHashSub;
    static std::unordered_set<std::string> extensionAllowed;
    static std::string commandUpdateDatapackMain;
    static std::string commandUpdateDatapackSub;
private:
    static std::regex regex_DATAPACK_FILE_REGEX;
    /// \todo group into one thread by change for queue
    QXzDecodeThread xzDecodeThreadMain;
    QXzDecodeThread xzDecodeThreadSub;
    bool datapackTarXzMain;
    bool datapackTarXzSub;
    CatchChallenger::DatapackChecksum datapackChecksum;
    int index_mirror_main;
    int index_mirror_sub;
    static std::regex excludePathMain;
    enum DatapackStatus
    {
        Main=0x02,
        Sub=0x03
    };
    DatapackStatus datapackStatus;

    bool wait_datapack_content_main;
    bool wait_datapack_content_sub;
    std::vector<std::string> datapackFilesListMain;
    std::vector<std::string> datapackFilesListSub;
    std::vector<uint32_t> partialHashListMain;
    std::vector<uint32_t> partialHashListSub;
    static std::string text_slash;
    static std::string text_dotcoma;
    bool httpError;
    bool httpModeMain;
    bool httpModeSub;
    const std::string mainDatapackCode;
    const std::string subDatapackCode;
    CURL *curl;
private:
    bool getHttpFileMain(const std::string &url, const std::string &fileName);
    bool getHttpFileSub(const std::string &url, const std::string &fileName);
private slots:
    void writeNewFileMain(const std::string &fileName, const QByteArray &data);
    void writeNewFileSub(const std::string &fileName, const QByteArray &data);
    void checkIfContinueOrFinished();
    void haveTheDatapackMainSub();
    void datapackDownloadFinishedMain();
    void datapackDownloadFinishedSub();
    void datapackChecksumDoneMain(const std::vector<std::string> &datapackFilesList,const QByteArray &hash, const std::vector<uint32_t> &partialHash);
    void datapackChecksumDoneSub(const std::vector<std::string> &datapackFilesList,const QByteArray &hash, const std::vector<uint32_t> &partialHash);
};
}

#endif // Protocol_and_data_H
