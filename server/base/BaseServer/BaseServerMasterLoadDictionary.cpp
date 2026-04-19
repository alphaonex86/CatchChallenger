#ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
#include "BaseServerMasterLoadDictionary.hpp"
#include "BaseServerMasterSendDatapack.hpp"
#include "../../general/base/CommonDatapack.hpp"
#include "../../general/base/cpp11addition.hpp"
#ifdef CATCHCHALLENGER_DB_FILE
#include "BaseServer.hpp"
#include <fstream>
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
    uint32_t tempId=0;
    if(BaseServer::dictionary_reputation_in_file!=nullptr)
    while(BaseServer::dictionary_reputation_in_file->peek()!=EOF)
    #endif
    {
        #ifdef CATCHCHALLENGER_DB_FILE
        uint8_t len=0;
        BaseServer::dictionary_reputation_in_file->read(reinterpret_cast<char*>(&len),1);
        if(!BaseServer::dictionary_reputation_in_file->good()) break;
        std::string reputation(len,'\0');
        BaseServer::dictionary_reputation_in_file->read(&reputation[0],len);
        if(!BaseServer::dictionary_reputation_in_file->good()) break;
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
                dictionary_reputation_database_to_internal.push_back(255);
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
        #ifdef CATCHCHALLENGER_DB_FILE
        tempId++;
        #endif
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
            BaseServer::dictionary_reputation_haveChange=true;
            #else
            #error Define what do here
            #endif
            while(dictionary_reputation_database_to_internal.size()<=lastId)
                dictionary_reputation_database_to_internal.push_back(255);
            dictionary_reputation_database_to_internal[lastId]=index;
            CommonDatapack::commonDatapack.get_reputation_rw()[index].reverse_database_id=lastId;
            #else
            std::cerr << "Dictionary reputation mismatch (abort)" << std::endl;
            abort();
            #endif
        }
        index++;
    }
    #ifdef CATCHCHALLENGER_DB_FILE
    if(BaseServer::dictionary_reputation_in_file!=nullptr)
    {
        BaseServer::dictionary_reputation_in_file->close();
        delete BaseServer::dictionary_reputation_in_file;
        BaseServer::dictionary_reputation_in_file=nullptr;
    }
    if(BaseServer::dictionary_reputation_haveChange)
    {
        std::ofstream dict_out("database/common/dictionary_reputation", std::ofstream::binary|std::ofstream::app);
        if(dict_out.good() && dict_out.is_open())
        {
            unsigned int newCount=0;
            for(uint8_t i=0;i<CommonDatapack::commonDatapack.get_reputation().size();i++)
            {
                if(foundReputation.find(CommonDatapack::commonDatapack.get_reputation().at(i).name)==foundReputation.end())
                {
                    const std::string &name=CommonDatapack::commonDatapack.get_reputation().at(i).name;
                    const uint8_t len=static_cast<uint8_t>(name.size());
                    dict_out.write(reinterpret_cast<const char*>(&len),1);
                    dict_out.write(name.data(),len);
                    newCount++;
                }
            }
            if(newCount>0)
                std::cout << newCount << " new reputation dictionary entries appended to database/common/dictionary_reputation" << std::endl;
        }
        else
            std::cerr << "Unable to open database/common/dictionary_reputation for writing" << std::endl;
        BaseServer::dictionary_reputation_haveChange=false;
    }
    #endif
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
    uint32_t tempId=0;
    if(BaseServer::dictionary_skin_in_file!=nullptr)
    while(BaseServer::dictionary_skin_in_file->peek()!=EOF)
    #endif
    {
        #ifdef CATCHCHALLENGER_DB_FILE
        uint8_t len=0;
        BaseServer::dictionary_skin_in_file->read(reinterpret_cast<char*>(&len),1);
        if(!BaseServer::dictionary_skin_in_file->good()) break;
        std::string skin(len,'\0');
        BaseServer::dictionary_skin_in_file->read(&skin[0],len);
        if(!BaseServer::dictionary_skin_in_file->good()) break;
        #else
        bool ok=true;
        const uint32_t &tempId=stringtouint32(databaseBaseBase->value(0),&ok);
        if(!ok)
            std::cerr << "BaseServerMasterLoadDictionary::preload_dictionary_skin_return() " << __FILE__ << ":" << __LINE__ << std::endl;
        const std::string &skin=databaseBaseBase->value(1);
        #endif
        if(tempId>lastId)
            lastId=tempId;
        if(dictionary_skin_database_to_internal.size()<=tempId)
        {
            unsigned int index=static_cast<uint32_t>(dictionary_skin_database_to_internal.size());
            while(index<=tempId)
            {
                dictionary_skin_database_to_internal.push_back(255);
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
        #ifdef CATCHCHALLENGER_DB_FILE
        tempId++;
        #endif
    }
    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
    databaseBaseBase->clear();
    #endif
    #elif CATCHCHALLENGER_DB_BLACKHOLE
    #else
    #error Define what do here
    #endif
    catchchallenger_datapack_map<std::string,uint8_t>::iterator i=BaseServerMasterSendDatapack::skinList.begin();
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
            BaseServer::dictionary_skin_haveChange=true;
            #else
            #error Define what do here
            #endif
            while(dictionary_skin_database_to_internal.size()<lastId)
                dictionary_skin_database_to_internal.push_back(255);
            dictionary_skin_database_to_internal.push_back(i->second);
            dictionary_skin_internal_to_database[i->second]=lastId;
            #else
            std::cerr << "Dictionary skin mismatch (abort)" << std::endl;
            abort();
            #endif
        }
        ++i;
    }
    #ifdef CATCHCHALLENGER_DB_FILE
    if(BaseServer::dictionary_skin_in_file!=nullptr)
    {
        BaseServer::dictionary_skin_in_file->close();
        delete BaseServer::dictionary_skin_in_file;
        BaseServer::dictionary_skin_in_file=nullptr;
    }
    if(BaseServer::dictionary_skin_haveChange)
    {
        std::ofstream dict_out("database/common/dictionary_skin", std::ofstream::binary|std::ofstream::app);
        if(dict_out.good() && dict_out.is_open())
        {
            unsigned int newCount=0;
            for(const std::pair<const std::string,uint8_t> &entry : BaseServerMasterSendDatapack::skinList)
            {
                if(foundSkin.find(entry.first)==foundSkin.end())
                {
                    const std::string &name=entry.first;
                    const uint8_t len=static_cast<uint8_t>(name.size());
                    dict_out.write(reinterpret_cast<const char*>(&len),1);
                    dict_out.write(name.data(),len);
                    newCount++;
                }
            }
            if(newCount>0)
                std::cout << newCount << " new skin dictionary entries appended to database/common/dictionary_skin" << std::endl;
        }
        else
            std::cerr << "Unable to open database/common/dictionary_skin for writing" << std::endl;
        BaseServer::dictionary_skin_haveChange=false;
    }
    #endif
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
    uint32_t tempId=0;
    if(BaseServer::dictionary_starter_in_file!=nullptr)
    while(BaseServer::dictionary_starter_in_file->peek()!=EOF)
    #endif
    {
        #ifdef CATCHCHALLENGER_DB_FILE
        uint8_t len=0;
        BaseServer::dictionary_starter_in_file->read(reinterpret_cast<char*>(&len),1);
        if(!BaseServer::dictionary_starter_in_file->good()) break;
        std::string starter(len,'\0');
        BaseServer::dictionary_starter_in_file->read(&starter[0],len);
        if(!BaseServer::dictionary_starter_in_file->good()) break;
        #else
        bool ok=true;
        const uint32_t &tempId=stringtouint32(databaseBaseBase->value(0),&ok);
        if(!ok)
            std::cerr << "BaseServerMasterLoadDictionary::preload_dictionary_starter_return() " << __FILE__ << ":" << __LINE__ << std::endl;
        const std::string &starter=std::string(databaseBaseBase->value(1));
        #endif
        if(tempId>lastId)
            lastId=tempId;
        if(dictionary_starter_database_to_internal.size()<=tempId)
        {
            uint8_t index=static_cast<uint8_t>(dictionary_starter_database_to_internal.size());
            while(index<=tempId)
            {
                dictionary_starter_database_to_internal.push_back(255);
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
        #ifdef CATCHCHALLENGER_DB_FILE
        tempId++;
        #endif
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
            BaseServer::dictionary_starter_haveChange=true;
            #else
            #error Define what do here
            #endif
            while(dictionary_starter_database_to_internal.size()<lastId)
                dictionary_starter_database_to_internal.push_back(255);
            dictionary_starter_database_to_internal.push_back(static_cast<uint8_t>(index));
            dictionary_starter_internal_to_database[index]=lastId;
            #else
            std::cerr << "Dictionary starter mismatch (abort)" << std::endl;
            abort();
            #endif
        }
        index++;
    }
    #ifdef CATCHCHALLENGER_DB_FILE
    if(BaseServer::dictionary_starter_in_file!=nullptr)
    {
        BaseServer::dictionary_starter_in_file->close();
        delete BaseServer::dictionary_starter_in_file;
        BaseServer::dictionary_starter_in_file=nullptr;
    }
    if(BaseServer::dictionary_starter_haveChange)
    {
        std::ofstream dict_out("database/common/dictionary_starter", std::ofstream::binary|std::ofstream::app);
        if(dict_out.good() && dict_out.is_open())
        {
            unsigned int newCount=0;
            for(unsigned int i=0;i<CommonDatapack::commonDatapack.get_profileList().size();i++)
            {
                const Profile &profile=CommonDatapack::commonDatapack.get_profileList().at(i);
                if(foundstarter.find(profile.databaseId)==foundstarter.end())
                {
                    const std::string &name=profile.databaseId;
                    const uint8_t len=static_cast<uint8_t>(name.size());
                    dict_out.write(reinterpret_cast<const char*>(&len),1);
                    dict_out.write(name.data(),len);
                    newCount++;
                }
            }
            if(newCount>0)
                std::cout << newCount << " new starter dictionary entries appended to database/common/dictionary_starter" << std::endl;
        }
        else
            std::cerr << "Unable to open database/common/dictionary_starter for writing" << std::endl;
        BaseServer::dictionary_starter_haveChange=false;
    }
    #endif
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
