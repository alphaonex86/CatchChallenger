#ifndef CATCHCHALLENGER_DatapackDownloaderBase_H
#define CATCHCHALLENGER_DatapackDownloaderBase_H

#include "../../general/base/GeneralVariable.h"
#include "../../client/base/DatapackChecksum.h"
#include "../../client/base/qt-tar-xz/QXzDecode.h"

#include <string>
#include <vector>
#include <unordered_map>
#include <regex>

#include <curl/curl.h>

#include "../../general/base/GeneralStructures.h"

namespace CatchChallenger {
class LinkToGameServer;
class DatapackDownloaderBase
{
public:
    explicit DatapackDownloaderBase(const std::string &mDatapackBase);
    virtual ~DatapackDownloaderBase();
    static DatapackDownloaderBase *datapackDownloaderBase;
    std::vector<LinkToGameServer *> clientInSuspend;
    void datapackDownloadError();
    void resetAll();
    void datapackFileList(const char * const data,const unsigned int &size);
    void writeNewFileBase(const std::string &fileName, const std::vector<char> &data);
    void sendDatapackProgressionBase(void * client);

    //datapack related
    void sendDatapackContentBase();
    void test_mirror_base();
    void decodedIsFinishBase(QXzDecode &xzDecodeBase);
    bool mirrorTryNextBase();
    void httpFinishedForDatapackListBase(const std::vector<char> data=std::vector<char>());
    const std::vector<std::string> listDatapackBase(std::string suffix);
    void cleanDatapackBase(std::string suffix);

    std::vector<char> hashBase;
    std::vector<char> sendedHashBase;
    static std::unordered_set<std::string> extensionAllowed;
    static std::string commandUpdateDatapackBase;
    static std::vector<std::string> httpDatapackMirrorBaseList;
    static CURLM *curlm;
    static unsigned int curlmCount;
    static std::vector<CURL *> curlSuspendList;
    static std::unordered_map<CURL *,void *> curlPrivateData;
private:
    static std::regex regex_DATAPACK_FILE_REGEX;
    bool datapackTarXzBase;
    CatchChallenger::DatapackChecksum datapackChecksum;
    unsigned int index_mirror_base;
    unsigned int numberOfFileWritten;
    static std::regex excludePathBase;
    //file list
    struct query_files
    {
        uint8_t id;
        std::vector<std::string> filesName;
    };
    std::vector<query_files> query_files_list_base;
    bool wait_datapack_content_base;
    std::vector<std::string> datapackFilesListBase;
    std::vector<uint32_t> partialHashListBase;
    bool httpError;
    bool httpModeBase;
    const std::string mDatapackBase;
    struct UrlInWaiting
    {
        std::string fileName;
    };
private:
    bool getHttpFileBase(const std::string &url, const std::string &fileName);
private:
    void datapackDownloadFinishedBase();
    void datapackChecksumDoneBase(const std::vector<std::string> &datapackFilesList,const std::vector<char> &hash, const std::vector<uint32_t> &partialHash);
    void haveTheDatapack();
};
}

#endif // Protocol_and_data_H
