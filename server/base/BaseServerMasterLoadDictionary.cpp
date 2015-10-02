#include "BaseServerMasterLoadDictionary.h"
#include "BaseServerMasterSendDatapack.h"
#include "../../general/base/CommonDatapackServerSpec.h"
#include "../../general/base/FacilityLibGeneral.h"
#include "../../general/base/GeneralVariable.h"
#include "../VariableServer.h"

#include <QDebug>
#include <QCryptographicHash>
#include <std::regex>
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
    preload_dictionary_allow();
}

void BaseServerMasterLoadDictionary::preload_dictionary_allow()
{
    std::string queryText;
    switch(databaseBaseBase->databaseType())
    {
        default:
        case DatabaseBase::DatabaseType::Mysql:
            queryText="SELECT `id`,`allow` FROM `dictionary_allow` ORDER BY `allow`";
        break;
        case DatabaseBase::DatabaseType::SQLite:
            queryText="SELECT id,allow FROM dictionary_allow ORDER BY allow";
        break;
        case DatabaseBase::DatabaseType::PostgreSQL:
            queryText="SELECT id,allow FROM dictionary_allow ORDER BY allow";
        break;
    }
    if(databaseBaseBase->asyncRead(queryText,this,&BaseServerMasterLoadDictionary::preload_dictionary_allow_static)==NULL)
    {
        std::cerr << "Sql error for: " << queryText << ", error: " << databaseBaseBase->errorMessage() << std::endl;
        abort();//stop because can't resolv the name
    }
}

void BaseServerMasterLoadDictionary::preload_dictionary_allow_static(void *object)
{
    static_cast<BaseServerMasterLoadDictionary *>(object)->preload_dictionary_allow_return();
}

void BaseServerMasterLoadDictionary::preload_dictionary_allow_return()
{
    dictionary_allow_internal_to_database={0x00,0x00};
    bool haveAllowClan=false;
    std::string allowClan("clan");
    unsigned int lastId=0;
    while(databaseBaseBase->next())
    {
        bool ok;
        lastId=stringtouint32(databaseBaseBase->value(0),&ok);
        const std::string &allow=std::string(databaseBaseBase->value(1));
        if(dictionary_allow_database_to_internal.size()<=lastId)
        {
            unsigned int index=dictionary_allow_database_to_internal.size();
            while(index<=lastId)
            {
                dictionary_allow_database_to_internal.push_back(ActionAllow_Nothing);
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
            dictionary_allow_database_to_internal.push_back(ActionAllow_Nothing);
    }
    databaseBaseBase->clear();
    if(!haveAllowClan)
    {
        #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
        lastId++;
        std::string queryText;
        switch(databaseBaseBase->databaseType())
        {
            default:
            case DatabaseBase::DatabaseType::Mysql:
                queryText="INSERT INTO `dictionary_allow`(`id`,`allow`) VALUES("+std::to_string(lastId)+",'clan');";
            break;
            case DatabaseBase::DatabaseType::SQLite:
                queryText="INSERT INTO dictionary_allow(id,allow) VALUES("+std::to_string(lastId)+",'clan');";
            break;
            case DatabaseBase::DatabaseType::PostgreSQL:
                queryText="INSERT INTO dictionary_allow(id,allow) VALUES("+std::to_string(lastId)+",'clan');";
            break;
        }
        if(!databaseBaseBase->asyncWrite(queryText))
        {
            std::cerr << "Sql error for: " << queryText << ", error: " << databaseBaseBase->errorMessage() << std::endl;
            abort();//stop because can't resolv the name
        }
        while(dictionary_allow_database_to_internal.size()<lastId)
            dictionary_allow_database_to_internal.push_back(ActionAllow_Nothing);
        dictionary_allow_database_to_internal.push_back(ActionAllow_Clan);
        dictionary_allow_internal_to_database[ActionAllow_Clan]=lastId;
        #else
        std::cerr << "Dictionary allow mismatch (abort)" << std::endl;
        abort();
        #endif
    }
    preload_dictionary_reputation();
}

void BaseServerMasterLoadDictionary::preload_dictionary_reputation()
{
    std::string queryText;
    switch(databaseBaseBase->databaseType())
    {
        default:
        case DatabaseBase::DatabaseType::Mysql:
            queryText="SELECT `id`,`reputation` FROM `dictionary_reputation` ORDER BY `reputation`";
        break;
        case DatabaseBase::DatabaseType::SQLite:
            queryText="SELECT id,reputation FROM dictionary_reputation ORDER BY reputation";
        break;
        case DatabaseBase::DatabaseType::PostgreSQL:
            queryText="SELECT id,reputation FROM dictionary_reputation ORDER BY reputation";
        break;
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
        lastId=stringtouint32(databaseBaseBase->value(0),&ok);
        const std::string &reputation=std::string(databaseBaseBase->value(1));
        if(dictionary_reputation_database_to_internal.size()<=lastId)
        {
            unsigned int index=dictionary_reputation_database_to_internal.size();
            while(index<=lastId)
            {
                dictionary_reputation_database_to_internal.push_back(-1);
                index++;
            }
        }
        if(reputationResolution.find(reputation)!=reputationResolution.end())
        {
            dictionary_reputation_database_to_internal[lastId]=reputationResolution.at(reputation);
            foundReputation.insert(reputation);
            CommonDatapack::commonDatapack.reputation[reputationResolution.at(reputation)].reverse_database_id=lastId;
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
                case DatabaseBase::DatabaseType::Mysql:
                    queryText="INSERT INTO `dictionary_reputation`(`id`,`reputation`) VALUES("+std::to_string(lastId)+",'"+reputation+"');";
                break;
                case DatabaseBase::DatabaseType::SQLite:
                    queryText="INSERT INTO dictionary_reputation(id,reputation) VALUES("+std::to_string(lastId)+",'"+reputation+"');";
                break;
                case DatabaseBase::DatabaseType::PostgreSQL:
                    queryText="INSERT INTO dictionary_reputation(id,reputation) VALUES("+std::to_string(lastId)+",'"+reputation+"');";
                break;
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
            qDebug() << std::stringLiteral("Dictionary reputation mismatch (abort)");
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
        case DatabaseBase::DatabaseType::Mysql:
            queryText="SELECT `id`,`skin` FROM `dictionary_skin` ORDER BY `skin`";
        break;
        case DatabaseBase::DatabaseType::SQLite:
            queryText="SELECT id,skin FROM dictionary_skin ORDER BY skin";
        break;
        case DatabaseBase::DatabaseType::PostgreSQL:
            queryText="SELECT id,skin FROM dictionary_skin ORDER BY skin";
        break;
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
        lastId=stringtouint32(databaseBaseBase->value(0),&ok);
        const std::string &skin=databaseBaseBase->value(1);
        if(dictionary_skin_database_to_internal.size()<=lastId)
        {
            unsigned int index=dictionary_skin_database_to_internal.size();
            while(index<=lastId)
            {
                dictionary_skin_database_to_internal.push_back(0);
                index++;
            }
        }
        if(BaseServerMasterSendDatapack::skinList.find(skin)!=BaseServerMasterSendDatapack::skinList.end())
        {
            const uint8_t &internalValue=BaseServerMasterSendDatapack::skinList.at(skin);
            dictionary_skin_database_to_internal[lastId]=internalValue;
            dictionary_skin_internal_to_database[internalValue]=lastId;
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
                case DatabaseBase::DatabaseType::Mysql:
                    queryText="INSERT INTO `dictionary_skin`(`id`,`skin`) VALUES("+std::to_string(lastId)+",'"+skin+"');";
                break;
                case DatabaseBase::DatabaseType::SQLite:
                    queryText="INSERT INTO dictionary_skin(id,skin) VALUES("+std::to_string(lastId)+",'"+skin+"');";
                break;
                case DatabaseBase::DatabaseType::PostgreSQL:
                    queryText="INSERT INTO dictionary_skin(id,skin) VALUES("+std::to_string(lastId)+",'"+skin+"');";
                break;
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
            qDebug() << std::stringLiteral("Dictionary skin mismatch (abort)");
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
        case DatabaseBase::DatabaseType::Mysql:
            queryText="SELECT `id`,`starter` FROM `dictionary_starter` ORDER BY `starter`";
        break;
        case DatabaseBase::DatabaseType::SQLite:
            queryText="SELECT id,starter FROM dictionary_starter ORDER BY starter";
        break;
        case DatabaseBase::DatabaseType::PostgreSQL:
            queryText="SELECT id,starter FROM dictionary_starter ORDER BY starter";
        break;
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
            profileNameToId[profile.id]=index;
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
        lastId=stringtouint32(databaseBaseBase->value(0),&ok);
        const std::string &starter=std::string(databaseBaseBase->value(1));
        if(dictionary_starter_database_to_internal.size()<=lastId)
        {
            unsigned int index=dictionary_starter_database_to_internal.size();
            while(index<=lastId)
            {
                dictionary_starter_database_to_internal.push_back(0);
                index++;
            }
        }
        if(profileNameToId.find(starter)!=profileNameToId.end())
        {
            const uint8_t &internalValue=profileNameToId.at(starter);
            dictionary_starter_database_to_internal[lastId]=internalValue;
            dictionary_starter_internal_to_database[internalValue]=lastId;
            foundstarter.insert(CommonDatapack::commonDatapack.profileList.at(internalValue).id);
        }
    }
    databaseBaseBase->clear();
    unsigned int index=0;
    while(index<CommonDatapack::commonDatapack.profileList.size())
    {
        const Profile &profile=CommonDatapack::commonDatapack.profileList.at(index);
        if(foundstarter.find(profile.id)==foundstarter.end())
        {
            #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
            lastId++;
            std::string queryText;
            switch(databaseBaseBase->databaseType())
            {
                default:
                case DatabaseBase::DatabaseType::Mysql:
                    queryText="INSERT INTO `dictionary_starter`(`id`,`starter`) VALUES("+std::to_string(lastId)+",'"+profile.id+"');";
                break;
                case DatabaseBase::DatabaseType::SQLite:
                    queryText="INSERT INTO dictionary_starter(id,starter) VALUES("+std::to_string(lastId)+",'"+profile.id+"');";
                break;
                case DatabaseBase::DatabaseType::PostgreSQL:
                    queryText="INSERT INTO dictionary_starter(id,starter) VALUES("+std::to_string(lastId)+",'"+profile.id+"');";
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
            qDebug() << std::stringLiteral("Dictionary starter mismatch (abort)");
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
    dictionary_allow_database_to_internal.clear();
    dictionary_allow_internal_to_database.clear();
    databaseBaseBase=NULL;
}
