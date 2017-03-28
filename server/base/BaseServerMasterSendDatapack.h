#ifndef CATCHCHALLENGER_BASESERVERLOGINSENDDATAPACK_H
#define CATCHCHALLENGER_BASESERVERLOGINSENDDATAPACK_H

#include <vector>
#include <regex>
#include "../../general/base/GeneralStructures.h"
#include "../../general/base/GeneralVariable.h"
#include "../VariableServer.h"
#include "DatabaseBase.h"

namespace CatchChallenger {
class BaseServerMasterSendDatapack
{
public:
    explicit BaseServerMasterSendDatapack();
    virtual ~BaseServerMasterSendDatapack();

    void load(const std::string &datapack_basePath);
    void unload();

    static std::unordered_map<std::string,uint8_t> skinList;

    struct DatapackCacheFile
    {
        //uint32_t mtime;
        uint32_t partialHash;
    };
    #ifndef CATCHCHALLENGER_SERVER_DATAPACK_ONLYBYMIRROR
    #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
    static std::unordered_set<std::string> compressedExtension;
    static std::vector<char> compressedFilesBuffer;
    static int compressedFilesBufferCount;
    #endif
    static std::unordered_set<std::string> extensionAllowed;
    static std::vector<char> rawFilesBuffer;
    static int rawFilesBufferCount;
    static std::unordered_map<std::string,uint32_t> datapack_file_list_cache;
    static std::unordered_map<std::string,DatapackCacheFile> datapack_file_hash_cache_base;
    #endif
    static std::regex fileNameStartStringRegex;
private:
    void preload_the_skin();
    void loadTheDatapackFileList();
private:
    //not global because all server don't need load the dictionary
    std::string datapack_basePathLogin;
};
}

#endif
