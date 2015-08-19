#ifndef CATCHCHALLENGER_BASESERVERLOGINSENDDATAPACK_H
#define CATCHCHALLENGER_BASESERVERLOGINSENDDATAPACK_H

#include <QList>
#include <QRegularExpression>
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

    void load(const std::basic_string<char> &datapack_basePath);
    void unload();

    static std::unordered_map<std::basic_string<char>,quint8> skinList;

    static std::unordered_set<std::basic_string<char> > compressedExtension;
    static std::unordered_set<std::basic_string<char> > extensionAllowed;
    static QByteArray rawFilesBuffer,compressedFilesBuffer;
    static int rawFilesBufferCount,compressedFilesBufferCount;
    struct DatapackCacheFile
    {
        quint32 mtime;
        quint32 partialHash;
    };
    static std::unordered_map<std::basic_string<char>,quint32> datapack_file_list_cache;
    static std::unordered_map<std::basic_string<char>,DatapackCacheFile> datapack_file_hash_cache;
    static QRegularExpression fileNameStartStringRegex;
private:
    void preload_the_skin();
    void loadTheDatapackFileList();
private:
    //not global because all server don't need load the dictionary
    std::basic_string<char> datapack_basePathLogin;
};
}

#endif
