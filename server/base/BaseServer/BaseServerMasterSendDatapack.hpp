#ifndef CATCHCHALLENGER_BASESERVERLOGINSENDDATAPACK_H
#define CATCHCHALLENGER_BASESERVERLOGINSENDDATAPACK_H

#include <vector>
#include <regex>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <cstdint>
#include "../../general/base/GeneralType.hpp"

namespace CatchChallenger {
class BaseServerMasterSendDatapack
{
public:
    explicit BaseServerMasterSendDatapack();
    virtual ~BaseServerMasterSendDatapack();

    #ifndef CATCHCHALLENGER_NOXML
    void load(const std::string &datapack_basePath);
    #endif
    void unload();

    static catchchallenger_datapack_map<std::string,uint8_t> skinList;

    class DatapackCacheFile
    {
    public:
        //uint32_t mtime;
        uint32_t partialHash;
        #ifdef CATCHCHALLENGER_CACHE_HPS
        template <class B>
        void serialize(B& buf) const {
            buf << partialHash;
        }
        template <class B>
        void parse(B& buf) {
            buf >> partialHash;
        }
        #endif
    };
    #ifndef CATCHCHALLENGER_SERVER_DATAPACK_ONLYBYMIRROR
    #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
    static catchchallenger_datapack_set<std::string> compressedExtension;
    static std::vector<char> compressedFilesBuffer;
    static uint8_t compressedFilesBufferCount;
    #endif
    static catchchallenger_datapack_set<std::string> extensionAllowed;
    static std::vector<char> rawFilesBuffer;
    static uint8_t rawFilesBufferCount;
    static catchchallenger_datapack_map<std::string,uint32_t> datapack_file_list_cache;
    static catchchallenger_datapack_map<std::string,DatapackCacheFile> datapack_file_hash_cache_base;
    #endif
    #ifndef CATCHCHALLENGER_NOXML
    static std::regex fileNameStartStringRegex;
    #endif
private:
    #ifndef CATCHCHALLENGER_NOXML
    void preload_the_skin();
    void loadTheDatapackFileList();
    #endif
private:
    #ifndef CATCHCHALLENGER_NOXML
    //not global because all server don't need load the dictionary
    std::string datapack_basePathLogin;
    #endif
};
}

#endif
