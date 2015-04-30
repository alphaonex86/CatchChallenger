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

    void load(const QString &datapack_basePath);
    void unload();

    static QHash<QString,quint8> skinList;

    static QSet<QString> compressedExtension;
    static QSet<QString> extensionAllowed;
    static QByteArray rawFilesBuffer,compressedFilesBuffer;
    static int rawFilesBufferCount,compressedFilesBufferCount;
    struct DatapackCacheFile
    {
        quint32 mtime;
        quint32 partialHash;
    };
    static QHash<QString,quint32> datapack_file_list_cache;
    static QHash<QString,DatapackCacheFile> datapack_file_hash_cache;
    static QRegularExpression fileNameStartStringRegex;
private:
    void preload_the_skin();
    void loadTheDatapackFileList();
private:
    //not global because all server don't need load the dictionary
    QString datapack_basePathLogin;
};
}

#endif
