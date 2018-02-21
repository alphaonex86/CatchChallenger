#ifndef CATCHCHALLENGER_DatapackDownloaderMainSub_H
#define CATCHCHALLENGER_DatapackDownloaderMainSub_H

#include "../../general/base/GeneralVariable.h"
#include "../../client/base/DatapackChecksum.h"

#include <string>
#include <string>
#include <vector>
#include <unordered_map>
#include <regex>

#include <curl/curl.h>

#include "../../general/base/GeneralStructures.h"

namespace CatchChallenger {
class LinkToGameServer;
class DatapackDownloaderMainSub
{
public:
    explicit DatapackDownloaderMainSub(const std::string &mDatapackBase, const std::string &mainDatapackCode, const std::string &subDatapackCode);
    virtual ~DatapackDownloaderMainSub();
    static std::unordered_map<std::string,std::unordered_map<std::string,DatapackDownloaderMainSub *> > datapackDownloaderMainSub;
    std::vector<LinkToGameServer *> clientInSuspend;
    void datapackDownloadError();
    void resetAll();
    void datapackFileList(const char * const data,const unsigned int &size);
    void writeNewFileToRoute(const std::string &fileName, const std::vector<char> &data);
    void sendDatapackProgressionMainSub(void * client);

    //datapack related
    void sendDatapackContentMainSub();
    void sendDatapackContentMain();
    void sendDatapackContentSub();
    void test_mirror_main();
    void test_mirror_sub();
    void httpErrorEventMain();
    void httpErrorEventSub();
    void decodedIsFinishMain(const std::vector<char> &rawData);
    void decodedIsFinishSub(const std::vector<char> &rawData);
    bool mirrorTryNextMain();
    bool mirrorTryNextSub();
    void httpFinishedForDatapackListMain(const std::vector<char> data=std::vector<char>());
    void httpFinishedForDatapackListSub(const std::vector<char> data=std::vector<char>());
    const std::vector<std::string> listDatapackMain(std::string suffix);
    const std::vector<std::string> listDatapackSub(std::string suffix);
    void cleanDatapackMain(std::string suffix);
    void cleanDatapackSub(std::string suffix);

    const std::string mDatapackBase;
    const std::string mDatapackMain;
    std::string mDatapackSub;
    std::vector<char> hashMain;
    std::vector<char> hashSub;
    std::vector<char> sendedHashMain;
    std::vector<char> sendedHashSub;
    static std::unordered_set<std::string> extensionAllowed;
    static std::string commandUpdateDatapackMain;
    static std::string commandUpdateDatapackSub;
    static std::vector<std::string> httpDatapackMirrorServerList;
private:
    static std::regex regex_DATAPACK_FILE_REGEX;
    bool datapackTarMain;
    bool datapackTarSub;
    CatchChallenger::DatapackChecksum datapackChecksum;
    unsigned int index_mirror_main;
    unsigned int index_mirror_sub;
    unsigned int numberOfFileWrittenMain,numberOfFileWrittenSub;
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
    bool httpError;
    bool httpModeMain;
    bool httpModeSub;
    const std::string mainDatapackCode;
    const std::string subDatapackCode;
private:
    bool getHttpFileMain(const std::string &url, const std::string &fileName);
    bool getHttpFileSub(const std::string &url, const std::string &fileName);
private:
    void writeNewFileMain(const std::string &fileName, const std::vector<char> &data);
    void writeNewFileSub(const std::string &fileName, const std::vector<char> &data);
    void checkIfContinueOrFinished();
    void haveTheDatapackMainSub();
    void datapackDownloadFinishedSub();
    void datapackChecksumDoneMain(const std::vector<std::string> &datapackFilesList,const std::vector<char> &hash, const std::vector<uint32_t> &partialHash);
    void datapackChecksumDoneSub(const std::vector<std::string> &datapackFilesList,const std::vector<char> &hash, const std::vector<uint32_t> &partialHash);
};
}

#endif // Protocol_and_data_H
