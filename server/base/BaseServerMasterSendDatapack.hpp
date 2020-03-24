#ifndef CATCHCHALLENGER_BASESERVERLOGINSENDDATAPACK_H
#define CATCHCHALLENGER_BASESERVERLOGINSENDDATAPACK_H

#include <vector>
#include <regex>
#include "../../general/base/GeneralStructures.hpp"
#include "../../general/base/GeneralVariable.hpp"
#include "../VariableServer.hpp"
#include "DatabaseBase.hpp"

namespace CatchChallenger {
class BaseServerMasterSendDatapack
{
public:
    explicit BaseServerMasterSendDatapack();
    virtual ~BaseServerMasterSendDatapack();

    void load(const std::string &datapack_basePath);
    void unload();

    static std::unordered_map<std::string,uint8_t> skinList;

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
    static std::unordered_set<std::string> compressedExtension;
    static std::vector<char> compressedFilesBuffer;
    static uint8_t compressedFilesBufferCount;
    #endif
    static std::unordered_set<std::string> extensionAllowed;
    static std::vector<char> rawFilesBuffer;
    static uint8_t rawFilesBufferCount;
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
