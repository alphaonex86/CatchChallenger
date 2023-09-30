#ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
#include "BaseServerMasterLoadDictionary.hpp"
#include "BaseServerMasterSendDatapack.hpp"
#include "../../general/base/CommonDatapack.hpp"
#include "../../general/base/cpp11addition.hpp"
#ifdef CATCHCHALLENGER_DB_FILE
#include "BaseServer.hpp"
#endif

#include <regex>
#include <iostream>

using namespace CatchChallenger;

BaseServerMasterLoadDictionary::BaseServerMasterLoadDictionary()
    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
    : databaseBaseBase(NULL)
    #elif CATCHCHALLENGER_DB_BLACKHOLE
    #elif CATCHCHALLENGER_DB_FILE
    #else
    #error Define what do here
    #endif
{
}

BaseServerMasterLoadDictionary::~BaseServerMasterLoadDictionary()
{
}

#if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
void BaseServerMasterLoadDictionary::load(DatabaseBase * const databaseBase)
{
    this->databaseBaseBase=databaseBase;
    preload_dictionary_reputation();
}
#elif CATCHCHALLENGER_DB_BLACKHOLE
#elif CATCHCHALLENGER_DB_FILE
#else
#error Define what do here
#endif

void BaseServerMasterLoadDictionary::preload_dictionary_reputation()
{
    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
    std::string queryText;
    switch(databaseBaseBase->databaseType())
    {
        default:
        abort();
        break;
        #if defined(CATCHCHALLENGER_DB_MYSQL) || (not defined(EPOLLCATCHCHALLENGERSERVER))
        case DatabaseBase::DatabaseType::Mysql:
            queryText="SELECT `id`,`reputation` FROM `dictionary_reputation` ORDER BY `id`";
        break;
        #endif
        #ifndef EPOLLCATCHCHALLENGERSERVER
        case DatabaseBase::DatabaseType::SQLite:
            queryText="SELECT id,reputation FROM dictionary_reputation ORDER BY id";
        break;
        #endif
        #if not defined(EPOLLCATCHCHALLENGERSERVER) || defined(CATCHCHALLENGER_DB_POSTGRESQL)
        case DatabaseBase::DatabaseType::PostgreSQL:
            queryText="SELECT id,reputation FROM dictionary_reputation ORDER BY id";
        break;
        #endif
    }
    if(databaseBaseBase->asyncRead(queryText,this,&BaseServerMasterLoadDictionary::preload_dictionary_reputation_static)==NULL)
    {
        std::cerr << "Sql error for: " << queryText << ", error: " << databaseBaseBase->errorMessage() << std::endl;
        abort();//stop because can't resolv the name
    }
    #elif CATCHCHALLENGER_DB_BLACKHOLE
    preload_dictionary_reputation_return();
    #elif CATCHCHALLENGER_DB_FILE
    preload_dictionary_reputation_return();
    #else
    #error Define what do here
    #endif
}

void BaseServerMasterLoadDictionary::preload_dictionary_reputation_static(void *object)
{
    static_cast<BaseServerMasterLoadDictionary *>(object)->preload_dictionary_reputation_return();
}

void BaseServerMasterLoadDictionary::preload_dictionary_reputation_return()
{
    std::unordered_map<std::string,uint8_t> reputationResolution;
    {
        uint8_t index=0;
        while(index<CommonDatapack::commonDatapack.get_reputation().size())
        {
            reputationResolution[CommonDatapack::commonDatapack.get_reputation().at(index).name]=index;
            index++;
        }
    }
    std::unordered_set<std::string> foundReputation;
    unsigned int lastId=0;
    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE) || defined(CATCHCHALLENGER_DB_FILE)
    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
    while(databaseBaseBase->next())
    #else
    size_t s=0;
    if(BaseServer::dictionary_serialBuffer!=nullptr)
        *BaseServer::dictionary_serialBuffer >> s;
    for (size_t i = 0; i < s; i++)
    #endif
    {
        #ifdef CATCHCHALLENGER_DB_FILE
        uint32_t tempId;
        *BaseServer::dictionary_serialBuffer >> tempId;
        std::string reputation;
        *BaseServer::dictionary_serialBuffer >> reputation;
        #else
        bool ok=true;
        const uint32_t &tempId=stringtouint32(databaseBaseBase->value(0),&ok);
        if(!ok)
            std::cerr << "BaseServerMasterLoadDictionary::preload_dictionary_reputation_return() " << __FILE__ << ":" << __LINE__ << std::endl;
        const std::string &reputation=std::string(databaseBaseBase->value(1));
        #endif
        if(tempId>lastId)
            lastId=tempId;
        if(dictionary_reputation_database_to_internal.size()<=tempId)
        {
            unsigned int index=static_cast<unsigned int>(dictionary_reputation_database_to_internal.size());
            while(index<=tempId)
            {
                dictionary_reputation_database_to_internal.push_back(-1);
                index++;
            }
        }
        if(reputationResolution.find(reputation)!=reputationResolution.end())
        {
            const uint8_t &internalId=reputationResolution.at(reputation);
            dictionary_reputation_database_to_internal[tempId]=internalId;
            foundReputation.insert(reputation);
            CommonDatapack::commonDatapack.get_reputation_rw()[internalId].reverse_database_id=tempId;
        }
    }
    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
    databaseBaseBase->clear();
    #endif
    #elif CATCHCHALLENGER_DB_BLACKHOLE
    #else
    #error Define what do here
    #endif
    unsigned int index=0;
    while(index<CommonDatapack::commonDatapack.get_reputation().size())
    {
        const std::string &reputation=CommonDatapack::commonDatapack.get_reputation().at(index).name;
        if(foundReputation.find(reputation)==foundReputation.end())
        {
            #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
            lastId++;
            #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
            std::string queryText;
            switch(databaseBaseBase->databaseType())
            {
                default:
                abort();
                break;
                #if defined(CATCHCHALLENGER_DB_MYSQL) || (not defined(EPOLLCATCHCHALLENGERSERVER))
                case DatabaseBase::DatabaseType::Mysql:
                    queryText="INSERT INTO `dictionary_reputation`(`id`,`reputation`) VALUES("+std::to_string(lastId)+",'"+reputation+"');";
                break;
                #endif
                #ifndef EPOLLCATCHCHALLENGERSERVER
                case DatabaseBase::DatabaseType::SQLite:
                    queryText="INSERT INTO dictionary_reputation(id,reputation) VALUES("+std::to_string(lastId)+",'"+reputation+"');";
                break;
                #endif
                #if not defined(EPOLLCATCHCHALLENGERSERVER) || defined(CATCHCHALLENGER_DB_POSTGRESQL)
                case DatabaseBase::DatabaseType::PostgreSQL:
                    queryText="INSERT INTO dictionary_reputation(id,reputation) VALUES("+std::to_string(lastId)+",'"+reputation+"');";
                break;
                #endif
            }
            if(!databaseBaseBase->asyncWrite(queryText))
            {
                std::cerr << "Sql error for: " << queryText << ", error: " << databaseBaseBase->errorMessage() << std::endl;
                abort();//stop because can't resolv the name
            }
            #elif CATCHCHALLENGER_DB_BLACKHOLE
            #elif CATCHCHALLENGER_DB_FILE
            BaseServer::dictionary_haveChange=true;
            #else
            #error Define what do here
            #endif
            while(dictionary_reputation_database_to_internal.size()<=lastId)
                dictionary_reputation_database_to_internal.push_back(-1);
            dictionary_reputation_database_to_internal[lastId]=index;
            CommonDatapack::commonDatapack.get_reputation_rw()[index].reverse_database_id=lastId;
            #else
            std::cerr << "Dictionary reputation mismatch (abort)" << std::endl;
            abort();
            #endif
        }
        index++;
    }
    preload_dictionary_skin();
}

void BaseServerMasterLoadDictionary::preload_dictionary_skin()
{
    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
    std::string queryText;
    switch(databaseBaseBase->databaseType())
    {
        default:
        abort();
        break;
        #if defined(CATCHCHALLENGER_DB_MYSQL) || (not defined(EPOLLCATCHCHALLENGERSERVER))
        case DatabaseBase::DatabaseType::Mysql:
            queryText="SELECT `id`,`skin` FROM `dictionary_skin` ORDER BY `id`";
        break;
        #endif
        #ifndef EPOLLCATCHCHALLENGERSERVER
        case DatabaseBase::DatabaseType::SQLite:
            queryText="SELECT id,skin FROM dictionary_skin ORDER BY id";
        break;
        #endif
        #if not defined(EPOLLCATCHCHALLENGERSERVER) || defined(CATCHCHALLENGER_DB_POSTGRESQL)
        case DatabaseBase::DatabaseType::PostgreSQL:
            queryText="SELECT id,skin FROM dictionary_skin ORDER BY id";
        break;
        #endif
    }
    if(databaseBaseBase->asyncRead(queryText,this,&BaseServerMasterLoadDictionary::preload_dictionary_skin_static)==NULL)
    {
        std::cerr << "Sql error for: " << queryText << ", error: " << databaseBaseBase->errorMessage() << std::endl;
        abort();//stop because can't resolv the name
    }
    #elif CATCHCHALLENGER_DB_BLACKHOLE
    preload_dictionary_skin_return();
    #elif CATCHCHALLENGER_DB_FILE
    preload_dictionary_skin_return();
    #else
    #error Define what do here
    #endif
}

void BaseServerMasterLoadDictionary::preload_dictionary_skin_static(void *object)
{
    static_cast<BaseServerMasterLoadDictionary *>(object)->preload_dictionary_skin_return();
}

void BaseServerMasterLoadDictionary::preload_dictionary_skin_return()
{
    {
        unsigned int index=0;
        while(index<BaseServerMasterSendDatapack::skinList.size())
        {
            dictionary_skin_internal_to_database.push_back(0);
            index++;
        }
    }
    std::unordered_set<std::string> foundSkin;
    unsigned int lastId=0;
    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE) || defined(CATCHCHALLENGER_DB_FILE)
    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
    while(databaseBaseBase->next())
    #else
    size_t s=0;
    if(BaseServer::dictionary_serialBuffer!=nullptr)
        *BaseServer::dictionary_serialBuffer >> s;
    for (size_t i = 0; i < s; i++)
    #endif
    {
        #ifdef CATCHCHALLENGER_DB_FILE
        uint32_t tempId;
        *BaseServer::dictionary_serialBuffer >> tempId;
        std::string skin;
        *BaseServer::dictionary_serialBuffer >> skin;
        #else
        bool ok=true;
        const uint32_t &tempId=stringtouint32(databaseBaseBase->value(0),&ok);
        if(!ok)
            std::cerr << "BaseServerMasterLoadDictionary::preload_dictionary_reputation_return() " << __FILE__ << ":" << __LINE__ << std::endl;
        const std::string &skin=databaseBaseBase->value(1);
        #endif
        if(tempId>lastId)
            lastId=tempId;
        if(dictionary_skin_database_to_internal.size()<=tempId)
        {
            unsigned int index=static_cast<uint32_t>(dictionary_skin_database_to_internal.size());
            while(index<=tempId)
            {
                dictionary_skin_database_to_internal.push_back(0);
                index++;
            }
        }
        if(BaseServerMasterSendDatapack::skinList.find(skin)!=BaseServerMasterSendDatapack::skinList.end())
        {
            const uint8_t &internalValue=BaseServerMasterSendDatapack::skinList.at(skin);
            dictionary_skin_database_to_internal[tempId]=internalValue;
            dictionary_skin_internal_to_database[internalValue]=tempId;
            foundSkin.insert(skin);
        }
    }
    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
    databaseBaseBase->clear();
    #endif
    #elif CATCHCHALLENGER_DB_BLACKHOLE
    #else
    #error Define what do here
    #endif
    auto i=BaseServerMasterSendDatapack::skinList.begin();
    while(i!=BaseServerMasterSendDatapack::skinList.end())
    {
        const std::string &skin=i->first;
        if(foundSkin.find(skin)==foundSkin.end())
        {
            #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
            lastId++;
            #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
            std::string queryText;
            switch(databaseBaseBase->databaseType())
            {
                default:
                abort();
                break;
                #if defined(CATCHCHALLENGER_DB_MYSQL) || (not defined(EPOLLCATCHCHALLENGERSERVER))
                case DatabaseBase::DatabaseType::Mysql:
                    queryText="INSERT INTO `dictionary_skin`(`id`,`skin`) VALUES("+std::to_string(lastId)+",'"+skin+"');";
                break;
                #endif
                #ifndef EPOLLCATCHCHALLENGERSERVER
                case DatabaseBase::DatabaseType::SQLite:
                    queryText="INSERT INTO dictionary_skin(id,skin) VALUES("+std::to_string(lastId)+",'"+skin+"');";
                break;
                #endif
                #if not defined(EPOLLCATCHCHALLENGERSERVER) || defined(CATCHCHALLENGER_DB_POSTGRESQL)
                case DatabaseBase::DatabaseType::PostgreSQL:
                    queryText="INSERT INTO dictionary_skin(id,skin) VALUES("+std::to_string(lastId)+",'"+skin+"');";
                break;
                #endif
            }
            if(!databaseBaseBase->asyncWrite(queryText))
            {
                std::cerr << "Sql error for: " << queryText << ", error: " << databaseBaseBase->errorMessage() << std::endl;
                abort();//stop because can't resolv the name
            }
            #elif CATCHCHALLENGER_DB_BLACKHOLE
            #elif CATCHCHALLENGER_DB_FILE
            BaseServer::dictionary_haveChange=true;
            #else
            #error Define what do here
            #endif
            while(dictionary_skin_database_to_internal.size()<lastId)
                dictionary_skin_database_to_internal.push_back(0);
            dictionary_skin_database_to_internal.push_back(i->second);
            dictionary_skin_internal_to_database[i->second]=lastId;
            #else
            std::cerr << "Dictionary skin mismatch (abort)" << std::endl;
            abort();
            #endif
        }
        ++i;
    }
    preload_dictionary_starter();
}

void BaseServerMasterLoadDictionary::preload_dictionary_starter()
{
    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
    std::string queryText;
    switch(databaseBaseBase->databaseType())
    {
        default:
        abort();
        break;
        #if defined(CATCHCHALLENGER_DB_MYSQL) || (not defined(EPOLLCATCHCHALLENGERSERVER))
        case DatabaseBase::DatabaseType::Mysql:
            queryText="SELECT `id`,`starter` FROM `dictionary_starter` ORDER BY `id`";
        break;
        #endif
        #ifndef EPOLLCATCHCHALLENGERSERVER
        case DatabaseBase::DatabaseType::SQLite:
            queryText="SELECT id,starter FROM dictionary_starter ORDER BY id";
        break;
        #endif
        #if not defined(EPOLLCATCHCHALLENGERSERVER) || defined(CATCHCHALLENGER_DB_POSTGRESQL)
        case DatabaseBase::DatabaseType::PostgreSQL:
            queryText="SELECT id,starter FROM dictionary_starter ORDER BY id";
        break;
        #endif
    }
    if(databaseBaseBase->asyncRead(queryText,this,&BaseServerMasterLoadDictionary::preload_dictionary_starter_static)==NULL)
    {
        std::cerr << "Sql error for: " << queryText << ", error: " << databaseBaseBase->errorMessage() << std::endl;
        abort();//stop because can't resolv the name
    }
    #elif CATCHCHALLENGER_DB_BLACKHOLE
    preload_dictionary_starter_return();
    #elif CATCHCHALLENGER_DB_FILE
    preload_dictionary_starter_return();
    #else
    #error Define what do here
    #endif
}

void BaseServerMasterLoadDictionary::preload_dictionary_starter_static(void *object)
{
    static_cast<BaseServerMasterLoadDictionary *>(object)->preload_dictionary_starter_return();
}

void BaseServerMasterLoadDictionary::preload_dictionary_starter_return()
{
    std::unordered_map<std::string,uint8_t> profileNameToId;
    {
        uint8_t index=0;
        while(index<CommonDatapack::commonDatapack.get_profileList().size())
        {
            const Profile &profile=CommonDatapack::commonDatapack.get_profileList().at(index);
            profileNameToId[profile.databaseId]=index;
            index++;
        }
    }
    {
        unsigned int index=0;
        while(index<CommonDatapack::commonDatapack.get_profileList().size())
        {
            dictionary_starter_internal_to_database.push_back(0);
            index++;
        }
    }
    std::unordered_set<std::string> foundstarter;
    unsigned int lastId=0;
    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE) || defined(CATCHCHALLENGER_DB_FILE)
    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
    while(databaseBaseBase->next())
    #else
    size_t s=0;
    if(BaseServer::dictionary_serialBuffer!=nullptr)
        *BaseServer::dictionary_serialBuffer >> s;
    for (size_t i = 0; i < s; i++)
    #endif
    {
        #ifdef CATCHCHALLENGER_DB_FILE
        uint32_t tempId;
        *BaseServer::dictionary_serialBuffer >> tempId;
        std::string starter;
        *BaseServer::dictionary_serialBuffer >> starter;
        #else
        bool ok=true;
        const uint32_t &tempId=stringtouint32(databaseBaseBase->value(0),&ok);
        if(!ok)
            std::cerr << "BaseServerMasterLoadDictionary::preload_dictionary_reputation_return() " << __FILE__ << ":" << __LINE__ << std::endl;
        const std::string &starter=std::string(databaseBaseBase->value(1));
        #endif
        if(tempId>lastId)
            lastId=tempId;
        if(dictionary_starter_database_to_internal.size()<=tempId)
        {
            uint8_t index=static_cast<uint8_t>(dictionary_starter_database_to_internal.size());
            while(index<=tempId)
            {
                dictionary_starter_database_to_internal.push_back(0);
                index++;
            }
        }
        if(profileNameToId.find(starter)!=profileNameToId.end())
        {
            const uint8_t &internalValue=profileNameToId.at(starter);
            dictionary_starter_database_to_internal[tempId]=internalValue;
            dictionary_starter_internal_to_database[internalValue]=tempId;
            foundstarter.insert(CommonDatapack::commonDatapack.get_profileList().at(internalValue).databaseId);
        }
    }
    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
    databaseBaseBase->clear();
    #endif
    #elif CATCHCHALLENGER_DB_BLACKHOLE
    #else
    #error Define what do here
    #endif
    unsigned int index=0;
    while(index<CommonDatapack::commonDatapack.get_profileList().size())
    {
        const Profile &profile=CommonDatapack::commonDatapack.get_profileList().at(index);
        if(foundstarter.find(profile.databaseId)==foundstarter.end())
        {
            #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
            lastId++;
            #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
            std::string queryText;
            switch(databaseBaseBase->databaseType())
            {
                #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_CLASS_QT)
                case DatabaseBase::DatabaseType::Mysql:
                    queryText="INSERT INTO `dictionary_starter`(`id`,`starter`) VALUES("+std::to_string(lastId)+",'"+profile.databaseId+"');";
                break;
                #endif
                #if defined(CATCHCHALLENGER_DB_SQLITE) || defined(CATCHCHALLENGER_CLASS_QT)
                case DatabaseBase::DatabaseType::SQLite:
                    queryText="INSERT INTO dictionary_starter(id,starter) VALUES("+std::to_string(lastId)+",'"+profile.databaseId+"');";
                break;
                #endif
                #if defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_CLASS_QT)
                case DatabaseBase::DatabaseType::PostgreSQL:
                    queryText="INSERT INTO dictionary_starter(id,starter) VALUES("+std::to_string(lastId)+",'"+profile.databaseId+"');";
                break;
                #endif
            default:
                abort();
                break;
            }
            if(!databaseBaseBase->asyncWrite(queryText))
            {
                std::cerr << "Sql error for: " << queryText << ", error: " << databaseBaseBase->errorMessage() << std::endl;
                abort();//stop because can't resolv the name
            }
            #elif CATCHCHALLENGER_DB_BLACKHOLE
            #elif CATCHCHALLENGER_DB_FILE
            BaseServer::dictionary_haveChange=true;
            #else
            #error Define what do here
            #endif
            while(dictionary_starter_database_to_internal.size()<lastId)
                dictionary_starter_database_to_internal.push_back(0);
            dictionary_starter_database_to_internal.push_back(static_cast<uint8_t>(index));
            dictionary_starter_internal_to_database[index]=lastId;
            #else
            std::cerr << "Dictionary starter mismatch (abort)" << std::endl;
            abort();
            #endif
        }
        index++;
    }
    SQL_common_load_finish();
}

void BaseServerMasterLoadDictionary::unload()
{
    dictionary_starter_database_to_internal.clear();
    dictionary_starter_internal_to_database.clear();
    dictionary_skin_database_to_internal.clear();
    dictionary_skin_internal_to_database.clear();
    dictionary_reputation_database_to_internal.clear();

    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
    databaseBaseBase=NULL;
    #elif CATCHCHALLENGER_DB_BLACKHOLE
    #elif CATCHCHALLENGER_DB_FILE
    #else
    #error Define what do here
    #endif
}
#endif
