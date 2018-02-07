#ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
#include "BaseServerMasterLoadDictionary.h"
#include "BaseServerMasterSendDatapack.h"
#include "../../general/base/CommonDatapackServerSpec.h"
#include "../../general/base/FacilityLibGeneral.h"
#include "../../general/base/GeneralVariable.h"
#include "../VariableServer.h"

#include <regex>
#include <iostream>

using namespace CatchChallenger;

BaseServerMasterLoadDictionary::BaseServerMasterLoadDictionary() :
    databaseBaseBase(NULL)
{
}

BaseServerMasterLoadDictionary::~BaseServerMasterLoadDictionary()
{
}

void BaseServerMasterLoadDictionary::load(DatabaseBase * const databaseBase)
{
    this->databaseBaseBase=databaseBase;
    preload_dictionary_reputation();
}

void BaseServerMasterLoadDictionary::preload_dictionary_reputation()
{
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
        while(index<CommonDatapack::commonDatapack.reputation.size())
        {
            reputationResolution[CommonDatapack::commonDatapack.reputation.at(index).name]=index;
            index++;
        }
    }
    std::unordered_set<std::string> foundReputation;
    unsigned int lastId=0;
    while(databaseBaseBase->next())
    {
        bool ok;
        const uint32_t &tempId=stringtouint32(databaseBaseBase->value(0),&ok);
        if(tempId>lastId)
            lastId=tempId;
        const std::string &reputation=std::string(databaseBaseBase->value(1));
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
            CommonDatapack::commonDatapack.reputation[internalId].reverse_database_id=tempId;
        }
    }
    databaseBaseBase->clear();
    unsigned int index=0;
    while(index<CommonDatapack::commonDatapack.reputation.size())
    {
        const std::string &reputation=CommonDatapack::commonDatapack.reputation.at(index).name;
        if(foundReputation.find(reputation)==foundReputation.end())
        {
            #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
            lastId++;
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
            while(dictionary_reputation_database_to_internal.size()<=lastId)
                dictionary_reputation_database_to_internal.push_back(-1);
            dictionary_reputation_database_to_internal[lastId]=index;
            CommonDatapack::commonDatapack.reputation[index].reverse_database_id=lastId;
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
    while(databaseBaseBase->next())
    {
        bool ok;
        const uint32_t &tempId=stringtouint32(databaseBaseBase->value(0),&ok);
        if(tempId>lastId)
            lastId=tempId;
        const std::string &skin=databaseBaseBase->value(1);
        if(dictionary_skin_database_to_internal.size()<=tempId)
        {
            unsigned int index=dictionary_skin_database_to_internal.size();
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
    databaseBaseBase->clear();
    auto i=BaseServerMasterSendDatapack::skinList.begin();
    while(i!=BaseServerMasterSendDatapack::skinList.end())
    {
        const std::string &skin=i->first;
        if(foundSkin.find(skin)==foundSkin.end())
        {
            #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
            lastId++;
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
}

void BaseServerMasterLoadDictionary::preload_dictionary_starter_static(void *object)
{
    static_cast<BaseServerMasterLoadDictionary *>(object)->preload_dictionary_starter_return();
}

void BaseServerMasterLoadDictionary::preload_dictionary_starter_return()
{
    std::unordered_map<std::string,uint8_t> profileNameToId;
    {
        unsigned int index=0;
        while(index<CommonDatapack::commonDatapack.profileList.size())
        {
            const Profile &profile=CommonDatapack::commonDatapack.profileList.at(index);
            profileNameToId[profile.databaseId]=index;
            index++;
        }
    }
    {
        unsigned int index=0;
        while(index<CommonDatapack::commonDatapack.profileList.size())
        {
            dictionary_starter_internal_to_database.push_back(0);
            index++;
        }
    }
    std::unordered_set<std::string> foundstarter;
    unsigned int lastId=0;
    while(databaseBaseBase->next())
    {
        bool ok;
        const uint32_t &tempId=stringtouint32(databaseBaseBase->value(0),&ok);
        if(tempId>lastId)
            lastId=tempId;
        const std::string &starter=std::string(databaseBaseBase->value(1));
        if(dictionary_starter_database_to_internal.size()<=tempId)
        {
            unsigned int index=dictionary_starter_database_to_internal.size();
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
            foundstarter.insert(CommonDatapack::commonDatapack.profileList.at(internalValue).databaseId);
        }
    }
    databaseBaseBase->clear();
    unsigned int index=0;
    while(index<CommonDatapack::commonDatapack.profileList.size())
    {
        const Profile &profile=CommonDatapack::commonDatapack.profileList.at(index);
        if(foundstarter.find(profile.databaseId)==foundstarter.end())
        {
            #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
            lastId++;
            std::string queryText;
            switch(databaseBaseBase->databaseType())
            {
                default:
                case DatabaseBase::DatabaseType::Mysql:
                    queryText="INSERT INTO `dictionary_starter`(`id`,`starter`) VALUES("+std::to_string(lastId)+",'"+profile.databaseId+"');";
                break;
                case DatabaseBase::DatabaseType::SQLite:
                    queryText="INSERT INTO dictionary_starter(id,starter) VALUES("+std::to_string(lastId)+",'"+profile.databaseId+"');";
                break;
                case DatabaseBase::DatabaseType::PostgreSQL:
                    queryText="INSERT INTO dictionary_starter(id,starter) VALUES("+std::to_string(lastId)+",'"+profile.databaseId+"');";
                break;
            }
            if(!databaseBaseBase->asyncWrite(queryText))
            {
                std::cerr << "Sql error for: " << queryText << ", error: " << databaseBaseBase->errorMessage() << std::endl;
                abort();//stop because can't resolv the name
            }
            while(dictionary_starter_database_to_internal.size()<lastId)
                dictionary_starter_database_to_internal.push_back(0);
            dictionary_starter_database_to_internal.push_back(index);
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
    databaseBaseBase=NULL;
}
#endif
