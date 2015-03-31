#include "BaseServerLogin.h"
#include "../../general/base/CommonDatapack.h"
#include "../../general/base/FacilityLibGeneral.h"
#include "../../general/base/GeneralVariable.h"
#include "../VariableServer.h"

#include <QDebug>
#include <QCryptographicHash>
#include <QRegularExpression>
#include <iostream>

using namespace CatchChallenger;

QSet<QString> BaseServerLogin::compressedExtension;
QSet<QString> BaseServerLogin::extensionAllowed;
QByteArray BaseServerLogin::rawFiles;
QByteArray BaseServerLogin::compressedFiles;
int BaseServerLogin::rawFilesCount;
int BaseServerLogin::compressedFilesCount;
QHash<QString,quint32> BaseServerLogin::datapack_file_list_cache;
QHash<QString,BaseServerLogin::DatapackCacheFile> BaseServerLogin::datapack_file_hash_cache;
QRegularExpression BaseServerLogin::fileNameStartStringRegex=QRegularExpression(QLatin1String("^[a-zA-Z]:/"));
QByteArray BaseServerLogin::datapackBaseHash;

BaseServerLogin::BaseServerLogin() :
    databaseBaseLogin(NULL)
{
}

BaseServerLogin::~BaseServerLogin()
{
}

void BaseServerLogin::load(DatabaseBase * const databaseBase, const QString &datapack_basePath)
{
    this->databaseBaseLogin=databaseBase;
    this->datapack_basePathLogin=datapack_basePath;
    preload_the_skin();
}

void BaseServerLogin::preload_the_skin()
{
    const QStringList &skinFolderList=FacilityLibGeneral::skinIdList(datapack_basePathLogin+DATAPACK_BASE_PATH_SKIN);
    int index=0;
    const int &listsize=skinFolderList.size();
    while(index<listsize)
    {
        skinList[skinFolderList.at(index)]=index;
        index++;
    }
    loadTheDatapackFileList();
}

void BaseServerLogin::loadTheDatapackFileList()
{
    QStringList extensionAllowedTemp=(QString(CATCHCHALLENGER_EXTENSION_ALLOWED)+QString(";")+QString(CATCHCHALLENGER_EXTENSION_COMPRESSED)).split(";");
    extensionAllowed=extensionAllowedTemp.toSet();
    QStringList compressedExtensionAllowedTemp=QString(CATCHCHALLENGER_EXTENSION_COMPRESSED).split(";");
    compressedExtension=compressedExtensionAllowedTemp.toSet();
    QRegularExpression datapack_rightFileName = QRegularExpression(DATAPACK_FILE_REGEX);

    QString text_datapack(datapack_basePathLogin);
    QString text_exclude("map/main/");

    QCryptographicHash baseHash(QCryptographicHash::Sha224);
    QStringList datapack_file_temp=FacilityLibGeneral::listFolder(text_datapack);
    datapack_file_temp.sort();

    int index=0;
    while(index<datapack_file_temp.size()) {
        QFile file(text_datapack+datapack_file_temp.at(index));
        if(datapack_file_temp.at(index).contains(datapack_rightFileName))
        {
            if(file.size()<=8*1024*1024)
            {
                if(!datapack_file_temp.at(index).startsWith(text_exclude))
                {
                    if(file.open(QIODevice::ReadOnly))
                    {
                        const QByteArray &data=file.readAll();
                        {
                            QCryptographicHash hashFile(QCryptographicHash::Sha224);
                            hashFile.addData(data);
                            BaseServerLogin::DatapackCacheFile cacheFile;
                            cacheFile.mtime=QFileInfo(file).lastModified().toTime_t();
                            cacheFile.partialHash=*reinterpret_cast<const int *>(hashFile.result().constData());
                            datapack_file_hash_cache[datapack_file_temp.at(index)]=cacheFile;
                        }
                        baseHash.addData(data);
                        file.close();
                    }
                    else
                    {
                        std::cerr << "Stop now! Unable to open the file " << file.fileName().toStdString() << " to do the datapack checksum for the mirror" << std::endl;
                        abort();
                    }
                }
            }
            else
                std::cerr << "File to big: " << datapack_file_temp.at(index).toStdString() << " size: " << file.size() << std::endl;
        }
        else
            std::cerr << "File excluded because don't match the regex: " << file.fileName().toStdString() << std::endl;
        index++;
    }

    datapackBaseHash=baseHash.result();
    std::cout << datapack_file_temp.size() << " files for datapack loaded" << std::endl;

    preload_dictionary_allow();
}

void BaseServerLogin::preload_dictionary_allow()
{
    QString queryText;
    switch(databaseBaseLogin->databaseType())
    {
        default:
        case DatabaseBase::Type::Mysql:
            queryText=QStringLiteral("SELECT `id`,`allow` FROM `dictionary_allow` ORDER BY `allow`");
        break;
        case DatabaseBase::Type::SQLite:
            queryText=QStringLiteral("SELECT id,allow FROM dictionary_allow ORDER BY allow");
        break;
        case DatabaseBase::Type::PostgreSQL:
            queryText=QStringLiteral("SELECT id,allow FROM dictionary_allow ORDER BY allow");
        break;
    }
    if(databaseBaseLogin->asyncRead(queryText.toLatin1(),this,&BaseServerLogin::preload_dictionary_allow_static)==NULL)
    {
        qDebug() << QStringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(databaseBaseLogin->errorMessage());
        abort();//stop because can't resolv the name
    }
}

void BaseServerLogin::preload_dictionary_allow_static(void *object)
{
    static_cast<BaseServerLogin *>(object)->preload_dictionary_allow_return();
}

void BaseServerLogin::preload_dictionary_allow_return()
{
    dictionary_allow_internal_to_database << 0x00 << 0x00;
    bool haveAllowClan=false;
    QString allowClan(QStringLiteral("clan"));
    int lastId=0;
    while(databaseBaseLogin->next())
    {
        bool ok;
        lastId=QString(databaseBaseLogin->value(0)).toUInt(&ok);
        const QString &allow=QString(databaseBaseLogin->value(1));
        if(dictionary_allow_database_to_internal.size()<=lastId)
        {
            int index=dictionary_allow_database_to_internal.size();
            while(index<=lastId)
            {
                dictionary_allow_database_to_internal << ActionAllow_Nothing;
                index++;
            }
        }
        if(allow==allowClan)
        {
            haveAllowClan=true;
            dictionary_allow_database_to_internal[lastId]=ActionAllow_Clan;
            dictionary_allow_internal_to_database[ActionAllow_Clan]=lastId;
        }
        else
            dictionary_allow_database_to_internal << ActionAllow_Nothing;
    }
    databaseBaseLogin->clear();
    if(!haveAllowClan)
    {
        lastId++;
        QString queryText;
        switch(databaseBaseLogin->databaseType())
        {
            default:
            case DatabaseBase::Type::Mysql:
                queryText=QStringLiteral("INSERT INTO `dictionary_allow`(`id`,`allow`) VALUES(%1,'clan');").arg(lastId);
            break;
            case DatabaseBase::Type::SQLite:
                queryText=QStringLiteral("INSERT INTO dictionary_allow(id,allow) VALUES(%1,'clan');").arg(lastId);
            break;
            case DatabaseBase::Type::PostgreSQL:
                queryText=QStringLiteral("INSERT INTO dictionary_allow(id,allow) VALUES(%1,'clan');").arg(lastId);
            break;
        }
        if(!databaseBaseLogin->asyncWrite(queryText.toLatin1()))
        {
            qDebug() << QStringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(databaseBaseLogin->errorMessage());
            abort();//stop because can't resolv the name
        }
        while(dictionary_allow_database_to_internal.size()<lastId)
            dictionary_allow_database_to_internal << ActionAllow_Nothing;
        dictionary_allow_database_to_internal << ActionAllow_Clan;
        dictionary_allow_internal_to_database[ActionAllow_Clan]=lastId;
    }
    preload_dictionary_reputation();
}

void BaseServerLogin::preload_dictionary_reputation()
{
    QString queryText;
    switch(databaseBaseLogin->databaseType())
    {
        default:
        case DatabaseBase::Type::Mysql:
            queryText=QStringLiteral("SELECT `id`,`reputation` FROM `dictionary_reputation` ORDER BY `reputation`");
        break;
        case DatabaseBase::Type::SQLite:
            queryText=QStringLiteral("SELECT id,reputation FROM dictionary_reputation ORDER BY reputation");
        break;
        case DatabaseBase::Type::PostgreSQL:
            queryText=QStringLiteral("SELECT id,reputation FROM dictionary_reputation ORDER BY reputation");
        break;
    }
    if(databaseBaseLogin->asyncRead(queryText.toLatin1(),this,&BaseServerLogin::preload_dictionary_reputation_static)==NULL)
    {
        qDebug() << QStringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(databaseBaseLogin->errorMessage());
        abort();//stop because can't resolv the name
    }
}

void BaseServerLogin::preload_dictionary_reputation_static(void *object)
{
    static_cast<BaseServerLogin *>(object)->preload_dictionary_reputation_return();
}

void BaseServerLogin::preload_dictionary_reputation_return()
{
    QHash<QString,quint8> reputationResolution;
    {
        quint8 index=0;
        while(index<CommonDatapack::commonDatapack.reputation.size())
        {
            reputationResolution[CommonDatapack::commonDatapack.reputation.at(index).name]=index;
            index++;
        }
    }
    QSet<QString> foundReputation;
    int lastId=0;
    while(databaseBaseLogin->next())
    {
        bool ok;
        lastId=QString(databaseBaseLogin->value(0)).toUInt(&ok);
        const QString &reputation=QString(databaseBaseLogin->value(1));
        if(dictionary_reputation_database_to_internal.size()<=lastId)
        {
            int index=dictionary_reputation_database_to_internal.size();
            while(index<=lastId)
            {
                dictionary_reputation_database_to_internal << -1;
                index++;
            }
        }
        if(reputationResolution.contains(reputation))
        {
            dictionary_reputation_database_to_internal[lastId]=reputationResolution.value(reputation);
            foundReputation << reputation;
            CommonDatapack::commonDatapack.reputation[reputationResolution.value(reputation)].reverse_database_id=lastId;
        }
    }
    databaseBaseLogin->clear();
    int index=0;
    while(index<CommonDatapack::commonDatapack.reputation.size())
    {
        const QString &reputation=CommonDatapack::commonDatapack.reputation.at(index).name;
        if(!foundReputation.contains(reputation))
        {
            lastId++;
            QString queryText;
            switch(databaseBaseLogin->databaseType())
            {
                default:
                case DatabaseBase::Type::Mysql:
                    queryText=QStringLiteral("INSERT INTO `dictionary_reputation`(`id`,`reputation`) VALUES(%1,'%2');").arg(lastId).arg(reputation);
                break;
                case DatabaseBase::Type::SQLite:
                    queryText=QStringLiteral("INSERT INTO dictionary_reputation(id,reputation) VALUES(%1,'%2');").arg(lastId).arg(reputation);
                break;
                case DatabaseBase::Type::PostgreSQL:
                    queryText=QStringLiteral("INSERT INTO dictionary_reputation(id,reputation) VALUES(%1,'%2');").arg(lastId).arg(reputation);
                break;
            }
            if(!databaseBaseLogin->asyncWrite(queryText.toLatin1()))
            {
                qDebug() << QStringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(databaseBaseLogin->errorMessage());
                abort();//stop because can't resolv the name
            }
            while(dictionary_reputation_database_to_internal.size()<=lastId)
                dictionary_reputation_database_to_internal << -1;
            dictionary_reputation_database_to_internal[lastId]=index;
            CommonDatapack::commonDatapack.reputation[index].reverse_database_id=lastId;
        }
        index++;
    }
    preload_dictionary_skin();
}

void BaseServerLogin::preload_dictionary_skin()
{
    QString queryText;
    switch(databaseBaseLogin->databaseType())
    {
        default:
        case DatabaseBase::Type::Mysql:
            queryText=QStringLiteral("SELECT `id`,`skin` FROM `dictionary_skin` ORDER BY `skin`");
        break;
        case DatabaseBase::Type::SQLite:
            queryText=QStringLiteral("SELECT id,skin FROM dictionary_skin ORDER BY skin");
        break;
        case DatabaseBase::Type::PostgreSQL:
            queryText=QStringLiteral("SELECT id,skin FROM dictionary_skin ORDER BY skin");
        break;
    }
    if(databaseBaseLogin->asyncRead(queryText.toLatin1(),this,&BaseServerLogin::preload_dictionary_skin_static)==NULL)
    {
        qDebug() << QStringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(databaseBaseLogin->errorMessage());
        abort();//stop because can't resolv the name
    }
}

void BaseServerLogin::preload_dictionary_skin_static(void *object)
{
    static_cast<BaseServerLogin *>(object)->preload_dictionary_skin_return();
}

void BaseServerLogin::preload_dictionary_skin_return()
{
    {
        int index=0;
        while(index<skinList.size())
        {
            dictionary_skin_internal_to_database << 0;
            index++;
        }
    }
    QSet<QString> foundSkin;
    int lastId=0;
    while(databaseBaseLogin->next())
    {
        bool ok;
        lastId=QString(databaseBaseLogin->value(0)).toUInt(&ok);
        const QString &skin=QString(databaseBaseLogin->value(1));
        if(dictionary_skin_database_to_internal.size()<=lastId)
        {
            int index=dictionary_skin_database_to_internal.size();
            while(index<=lastId)
            {
                dictionary_skin_database_to_internal << 0;
                index++;
            }
        }
        if(skinList.contains(skin))
        {
            const quint8 &internalValue=skinList.value(skin);
            dictionary_skin_database_to_internal[lastId]=internalValue;
            dictionary_skin_internal_to_database[internalValue]=lastId;
            foundSkin << skin;
        }
    }
    databaseBaseLogin->clear();
    QHashIterator<QString,quint8> i(skinList);
    while (i.hasNext()) {
        i.next();
        const QString &skin=i.key();
        if(!foundSkin.contains(skin))
        {
            lastId++;
            QString queryText;
            switch(databaseBaseLogin->databaseType())
            {
                default:
                case DatabaseBase::Type::Mysql:
                    queryText=QStringLiteral("INSERT INTO `dictionary_skin`(`id`,`skin`) VALUES(%1,'%2');").arg(lastId).arg(skin);
                break;
                case DatabaseBase::Type::SQLite:
                    queryText=QStringLiteral("INSERT INTO dictionary_skin(id,skin) VALUES(%1,'%2');").arg(lastId).arg(skin);
                break;
                case DatabaseBase::Type::PostgreSQL:
                    queryText=QStringLiteral("INSERT INTO dictionary_skin(id,skin) VALUES(%1,'%2');").arg(lastId).arg(skin);
                break;
            }
            if(!databaseBaseLogin->asyncWrite(queryText.toLatin1()))
            {
                qDebug() << QStringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(databaseBaseLogin->errorMessage());
                abort();//stop because can't resolv the name
            }
            while(dictionary_skin_database_to_internal.size()<lastId)
                dictionary_skin_database_to_internal << 0;
            dictionary_skin_database_to_internal << i.value();
            dictionary_skin_internal_to_database[i.value()]=lastId;
        }
    }
    preload_dictionary_starter();
}

void BaseServerLogin::preload_dictionary_starter()
{
    QString queryText;
    switch(databaseBaseLogin->databaseType())
    {
        default:
        case DatabaseBase::Type::Mysql:
            queryText=QStringLiteral("SELECT `id`,`starter` FROM `dictionary_starter` ORDER BY `starter`");
        break;
        case DatabaseBase::Type::SQLite:
            queryText=QStringLiteral("SELECT id,starter FROM dictionary_starter ORDER BY starter");
        break;
        case DatabaseBase::Type::PostgreSQL:
            queryText=QStringLiteral("SELECT id,starter FROM dictionary_starter ORDER BY starter");
        break;
    }
    if(databaseBaseLogin->asyncRead(queryText.toLatin1(),this,&BaseServerLogin::preload_dictionary_starter_static)==NULL)
    {
        qDebug() << QStringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(databaseBaseLogin->errorMessage());
        abort();//stop because can't resolv the name
    }
}

void BaseServerLogin::preload_dictionary_starter_static(void *object)
{
    static_cast<BaseServerLogin *>(object)->preload_dictionary_starter_return();
}

void BaseServerLogin::preload_dictionary_starter_return()
{
    QHash<QString,quint8> profileNameToId;
    {
        int index=0;
        while(index<CommonDatapack::commonDatapack.profileList.size())
        {
            const Profile &profile=CommonDatapack::commonDatapack.profileList.at(index);
            profileNameToId[profile.id]=index;
            index++;
        }
    }
    {
        int index=0;
        while(index<CommonDatapack::commonDatapack.profileList.size())
        {
            dictionary_starter_internal_to_database << 0;
            index++;
        }
    }
    QSet<QString> foundstarter;
    int lastId=0;
    while(databaseBaseLogin->next())
    {
        bool ok;
        lastId=QString(databaseBaseLogin->value(0)).toUInt(&ok);
        const QString &starter=QString(databaseBaseLogin->value(1));
        if(dictionary_starter_database_to_internal.size()<=lastId)
        {
            int index=dictionary_starter_database_to_internal.size();
            while(index<=lastId)
            {
                dictionary_starter_database_to_internal << 0;
                index++;
            }
        }
        if(profileNameToId.contains(starter))
        {
            const quint8 &internalValue=profileNameToId.value(starter);
            dictionary_starter_database_to_internal[lastId]=internalValue;
            dictionary_starter_internal_to_database[internalValue]=lastId;
            foundstarter << CommonDatapack::commonDatapack.profileList.at(internalValue).id;
        }
    }
    databaseBaseLogin->clear();
    int index=0;
    while(index<CommonDatapack::commonDatapack.profileList.size())
    {
        const Profile &profile=CommonDatapack::commonDatapack.profileList.at(index);
        if(!foundstarter.contains(profile.id))
        {
            lastId++;
            QString queryText;
            switch(databaseBaseLogin->databaseType())
            {
                default:
                case DatabaseBase::Type::Mysql:
                    queryText=QStringLiteral("INSERT INTO `dictionary_starter`(`id`,`starter`) VALUES(%1,'%2');").arg(lastId).arg(profile.id);
                break;
                case DatabaseBase::Type::SQLite:
                    queryText=QStringLiteral("INSERT INTO dictionary_starter(id,starter) VALUES(%1,'%2');").arg(lastId).arg(profile.id);
                break;
                case DatabaseBase::Type::PostgreSQL:
                    queryText=QStringLiteral("INSERT INTO dictionary_starter(id,starter) VALUES(%1,'%2');").arg(lastId).arg(profile.id);
                break;
            }
            if(!databaseBaseLogin->asyncWrite(queryText.toLatin1()))
            {
                qDebug() << QStringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(databaseBaseLogin->errorMessage());
                abort();//stop because can't resolv the name
            }
            while(dictionary_starter_database_to_internal.size()<lastId)
                dictionary_starter_database_to_internal << 0;
            dictionary_starter_database_to_internal << index;
            dictionary_starter_internal_to_database[index]=lastId;
        }
        index++;
    }
    SQL_common_load_finish();
}

void BaseServerLogin::unload()
{
    dictionary_starter_database_to_internal.clear();
    dictionary_starter_internal_to_database.clear();
    dictionary_skin_database_to_internal.clear();
    dictionary_skin_internal_to_database.clear();
    dictionary_reputation_database_to_internal.clear();
    dictionary_allow_database_to_internal.clear();
    dictionary_allow_internal_to_database.clear();
    databaseBaseLogin=NULL;
}
