#ifndef CATCHCHALLENGER_DatapackDownloaderMainSub_H
#define CATCHCHALLENGER_DatapackDownloaderMainSub_H

#include "../../general/base/GeneralVariable.hpp"
#include "../../client/libcatchchallenger/DatapackChecksum.hpp"

#include <string>
#include <string>
#include <vector>
#include <unordered_map>
#include <regex>

#include <curl/curl.h>

#include "../../general/base/GeneralStructures.hpp"

class DatapackDownloaderMainSub
{
public:
    explicit DatapackDownloaderMainSub();
    virtual ~DatapackDownloaderMainSub();

    virtual std::unordered_map<std::string,uint32_t/*partialHash*/> datapack_file_list(const std::string &path,const bool withHash) = 0;
    virtual void newError(const std::string &error,const std::string &detailedError) = 0;
    virtual void message(const std::string &message) = 0;
    virtual std::string datapackPathMain() const = 0;
    virtual std::string datapackPathSub() const = 0;
    virtual std::string mainDatapackCode() const = 0;
    virtual std::string subDatapackCode() const = 0;
    virtual bool mkpath(const std::string &dir) = 0;

    void datapackDownloadError();
    void resetAll();
    void datapackFileList(const char * const data,const unsigned int &size);

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
    void writeNewFileMain(const std::string &fileName, const std::string &data);
    void writeNewFileSub(const std::string &fileName, const std::string &data);

    std::vector<char> hashMain;
    std::vector<char> hashSub;
    static std::unordered_set<std::string> extensionAllowed;
    static std::vector<std::string> httpDatapackMirrorServerList;
private:
    static std::regex regex_DATAPACK_FILE_REGEX;
    bool datapackTarMain;
    bool datapackTarSub;
    CatchChallenger::DatapackChecksum datapackChecksum;
    unsigned int index_mirror_main;
    unsigned int index_mirror_sub;
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
private:
    bool getHttpFileMain(const std::string &url, const std::string &fileName);
    bool getHttpFileSub(const std::string &url, const std::string &fileName);

    static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp);
    struct MemoryStruct {
      char *memory;
      size_t size;
      std::string fileName;
    };
private:
    void checkIfContinueOrFinished();
    void haveTheDatapackMainSub();
    void datapackDownloadFinishedSub();
    void datapackChecksumDoneMain(const std::vector<std::string> &datapackFilesList,const std::vector<char> &hash, const std::vector<uint32_t> &partialHash);
    void datapackChecksumDoneSub(const std::vector<std::string> &datapackFilesList,const std::vector<char> &hash, const std::vector<uint32_t> &partialHash);
};

#endif // Protocol_and_data_H
