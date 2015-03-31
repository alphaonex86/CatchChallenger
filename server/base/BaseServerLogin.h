#ifndef CATCHCHALLENGER_BASESERVERLOGIN_H
#define CATCHCHALLENGER_BASESERVERLOGIN_H

#include <QList>
#include <QRegularExpression>
#include "../../general/base/GeneralStructures.h"
#include "DatabaseBase.h"

namespace CatchChallenger {
class BaseServerLogin
{
public:
    explicit BaseServerLogin();
    virtual ~BaseServerLogin();

    void load(CatchChallenger::DatabaseBase * const databaseBase,const QString &datapack_basePath);

    static QSet<QString> compressedExtension;
    static QSet<QString> extensionAllowed;
    static QByteArray rawFiles,compressedFiles;
    static int rawFilesCount,compressedFilesCount;
    struct DatapackCacheFile
    {
        quint32 mtime;
        quint32 partialHash;
    };
    static QHash<QString,quint32> datapack_file_list_cache;
    static QHash<QString,DatapackCacheFile> datapack_file_hash_cache;
    static QRegularExpression fileNameStartStringRegex;
    static QByteArray datapackBaseHash;
public:
    QList<ActionAllow> dictionary_allow_database_to_internal;
    QList<quint8> dictionary_allow_internal_to_database;
    QList<int> dictionary_reputation_database_to_internal;//negative == not found
    QList<quint8> dictionary_skin_database_to_internal;
    QList<quint32> dictionary_skin_internal_to_database;
    QList<quint8> dictionary_starter_database_to_internal;
    QList<quint32> dictionary_starter_internal_to_database;
private:
    virtual void SQL_common_load_finish() = 0;

    void preload_the_skin();
    void loadTheDatapackFileList();

    void preload_dictionary_allow();
    static void preload_dictionary_allow_static(void *object);
    void preload_dictionary_allow_return();
    void preload_dictionary_reputation();
    static void preload_dictionary_reputation_static(void *object);
    void preload_dictionary_reputation_return();
    void preload_dictionary_skin();
    static void preload_dictionary_skin_static(void *object);
    void preload_dictionary_skin_return();
    void preload_dictionary_starter();
    static void preload_dictionary_starter_static(void *object);
    void preload_dictionary_starter_return();
private:
    QHash<QString,quint8> skinList;
    //not global because all server don't need load the dictionary
    CatchChallenger::DatabaseBase *databaseBaseLogin;
    QString datapack_basePathLogin;
protected:
    void unload();
};
}

#endif
