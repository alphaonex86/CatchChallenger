#include "Client.hpp"
#include "GlobalServerData.hpp"
#include "../../general/base/FacilityLibGeneral.hpp"
#include <openssl/sha.h>
#include <sys/stat.h>

/// \todo solve disconnecting/destroy during the SQL loading

using namespace CatchChallenger;

//load linked data (like item, quests, ...)
/*void Client::loadLinkedData()
{
    //loadPlayerAllow();
}*/

std::unordered_map<std::string,BaseServerMasterSendDatapack::DatapackCacheFile> Client::datapack_file_list(const std::string &path, const std::string &exclude, const bool withHash)
{
    std::unordered_map<std::string,BaseServerMasterSendDatapack::DatapackCacheFile> filesList;

    std::vector<std::string> returnList;
    if(exclude.empty())
        returnList=FacilityLibGeneral::listFolder(path);
    else
        returnList=FacilityLibGeneral::listFolderWithExclude(path,exclude);

    #ifdef CATCHCHALLENGER_SERVER_DATAPACK_ONLYBYMIRROR
    const std::vector<std::string> &extensionAllowedTemp=stringsplit(std::string(CATCHCHALLENGER_EXTENSION_ALLOWED+std::string(";")+CATCHCHALLENGER_EXTENSION_COMPRESSED),';');
    const std::unordered_set<std::string> &extensionAllowed=std::unordered_set<std::string>(extensionAllowedTemp.begin(),extensionAllowedTemp.end());
    #endif

    unsigned int index=0;
    while(index<returnList.size())
    {
        #ifdef _WIN32
        std::string fileName=returnList.at(index);
        #else
        const std::string &fileName=returnList.at(index);
        #endif
        const std::string &suffix=FacilityLibGeneral::getSuffixAndValidatePathFromFS(fileName);
        //if(regex_search(fileName,GlobalServerData::serverPrivateVariables.datapack_rightFileName))
        //try replace by better algo
        if(!suffix.empty())
        {
//            const std::string &suffix=FacilityLibGeneral::getSuffix(fileName);
            if(!suffix.empty() &&
            #ifndef CATCHCHALLENGER_SERVER_DATAPACK_ONLYBYMIRROR
                    BaseServerMasterSendDatapack::extensionAllowed.find(suffix)
                    !=BaseServerMasterSendDatapack::extensionAllowed.cend())
            #else
                    extensionAllowed.find(suffix)!=extensionAllowed.cend())
            #endif
            {
                BaseServerMasterSendDatapack::DatapackCacheFile datapackCacheFile;
                if(withHash)
                {
                    struct stat buf;
                    if(::stat((path+returnList.at(index)).c_str(),&buf)==0)
                    {
                        if(buf.st_size<=CATCHCHALLENGER_MAX_FILE_SIZE)
                        {
                            std::string fullPathFileToOpen=path+returnList.at(index);
                            #ifdef Q_OS_WIN32
                            stringreplaceAll(fullPathFileToOpen,"/","\\");
                            #endif
                            FILE *filedesc=fopen(fullPathFileToOpen.c_str(),"rb");
                            if(filedesc!=NULL)
                            {
                                #ifdef _WIN32
                                stringreplaceAll(fileName,"\\","/");//remplace if is under windows server
                                #endif
                                const std::vector<char> &data=FacilityLibGeneral::readAllFileAndClose(filedesc);
                                SHA224(reinterpret_cast<const unsigned char *>(data.data()),data.size(),reinterpret_cast<unsigned char *>(ProtocolParsingBase::tempBigBufferForOutput));
                                ::memcpy(&datapackCacheFile.partialHash,ProtocolParsingBase::tempBigBufferForOutput,sizeof(uint32_t));
                            }
                            else
                            {
                                datapackCacheFile.partialHash=0;
                                std::cerr << "Client::datapack_file_list fopen failed on " +path+returnList.at(index)+ ":"+std::to_string(errno) << std::endl;
                            }
                        }
                        else
                        {
                            datapackCacheFile.partialHash=0;
                            std::cerr << "Client::datapack_file_list file too big failed on " +path+returnList.at(index)+ ":"+std::to_string(buf.st_size) << std::endl;
                        }
                    }
                    else
                    {
                        datapackCacheFile.partialHash=0;
                        std::cerr << "Client::datapack_file_list stat failed on " +path+returnList.at(index)+ ":"+std::to_string(errno) << std::endl;
                    }
                }
                else
                    datapackCacheFile.partialHash=0;
                filesList[fileName]=datapackCacheFile;
            }
        }
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        else
        {
            std::cerr << "For Client::datapack_file_list(" << path << "," << exclude << "," << withHash << ")" << std::endl;
            std::cerr << "FacilityLibGeneral::getSuffixAndValidatePathFromFS(" << fileName << ") return empty result" << std::endl;
            //const std::string &suffix2=FacilityLibGeneral::getSuffixAndValidatePathFromFS(fileName);
        }
        #endif
        index++;
    }
    return filesList;
}

bool CatchChallenger::operator<(const CatchChallenger::FileToSend &fileToSend1,const CatchChallenger::FileToSend &fileToSend2)
{
    if(fileToSend1.file<fileToSend2.file)
        return false;
    return true;
}

bool CatchChallenger::operator!=(const CatchChallenger::PlayerMonster &monster1,const CatchChallenger::PlayerMonster &monster2)
{
    if(monster1.remaining_xp!=monster2.remaining_xp)
        return true;
    if(monster1.sp!=monster2.sp)
        return true;
    if(monster1.egg_step!=monster2.egg_step)
        return true;
    if(monster1.id!=monster2.id)
        return true;
/* transfer in 0.6 and check it
    if(monster1.character_origin!=monster2.character_origin)
        return true;*/
    if(monster1.skills.size()!=monster2.skills.size())
        return true;
    unsigned int index=0;
    while(index<monster1.skills.size())
    {
        const PlayerMonster::PlayerSkill &skill1=monster1.skills.at(index);
        const PlayerMonster::PlayerSkill &skill2=monster2.skills.at(index);
        if(skill1.endurance!=skill2.endurance)
            return true;
        if(skill1.level!=skill2.level)
            return true;
        if(skill1.skill!=skill2.skill)
            return true;
        index++;
    }
    return false;
}

#ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
void Client::dbQueryWriteLogin(const std::string &queryText)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(queryText.empty())
    {
        errorOutput("dbQueryWriteLogin() Query is empty, bug");
        return;
    }
    if(stringStartWith(queryText,"SELECT"))
    {
            errorOutput("dbQueryWriteLogin() Query is SELECT but here is the write queue, bug");
            return;
    }
    #endif
    #ifdef DEBUG_MESSAGE_CLIENT_SQL
    normalOutput("Do db_login write: "+queryText);
    #endif
    GlobalServerData::serverPrivateVariables.db_login->asyncWrite(queryText);
}
#endif

void Client::dbQueryWriteCommon(const std::string &queryText)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(queryText.empty())
    {
        errorOutput("dbQueryWriteCommon() Query is empty, bug");
        return;
    }
    if(stringStartWith(queryText,"SELECT"))
    {
            errorOutput("dbQueryWriteCommon() Query is SELECT but here is the write queue, bug");
            return;
    }
    #endif
    #ifdef DEBUG_MESSAGE_CLIENT_SQL
    normalOutput("Do db_common write: "+queryText);
    #endif
    GlobalServerData::serverPrivateVariables.db_common->asyncWrite(queryText);
}

void Client::dbQueryWriteServer(const std::string &queryText)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(queryText.empty())
    {
        errorOutput("dbQueryWriteServer() Query is empty, bug");
        return;
    }
    if(stringStartWith(queryText,"SELECT"))
    {
            errorOutput("dbQueryWriteServer() Query is SELECT but here is the write queue, bug");
            return;
    }
    #endif
    #ifdef DEBUG_MESSAGE_CLIENT_SQL
    normalOutput("Do db_server write: "+queryText);
    #endif
    GlobalServerData::serverPrivateVariables.db_server->asyncWrite(queryText);
}

/*void Client::loadReputation()
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(PreparedDBQueryCommon::db_query_select_reputation_by_id.empty())
    {
        errorOutput("loadReputation() Query is empty, bug");
        return;
    }
    #endif
    std::string queryText=PreparedDBQueryCommon::db_query_select_reputation_by_id;
    stringreplaceOne(queryText,"%1",std::to_string(character_id));
    CatchChallenger::DatabaseBase::CallBack *callback=GlobalServerData::serverPrivateVariables.db_common->asyncRead(queryText,this,&Client::loadReputation_static);
    if(callback==NULL)
    {
        std::cerr << "Sql error for: " << queryText << ", error: " << GlobalServerData::serverPrivateVariables.db_common->errorMessage() << std::endl;
        loadQuests();
        return;
    }
    else
        callbackRegistred.push(callback);
}

void Client::loadReputation_static(void *object)
{
    if(object!=NULL)
        static_cast<Client *>(object)->loadReputation_return();
}

void Client::loadReputation_return()
{
    callbackRegistred.pop();
    bool ok;
    //parse the result
    while(GlobalServerData::serverPrivateVariables.db_common->next())
    {
        const unsigned int &type=GlobalServerData::serverPrivateVariables.db_common->stringtouint32(GlobalServerData::serverPrivateVariables.db_common->value(0),&ok);
        if(!ok)
        {
            normalOutput("reputation type is not a number, skip: "+GlobalServerData::serverPrivateVariables.db_common->value(0));
            continue;
        }
        int32_t point=GlobalServerData::serverPrivateVariables.db_common->stringtoint32(GlobalServerData::serverPrivateVariables.db_common->value(1),&ok);
        if(!ok)
        {
            normalOutput("reputation point is not a number, skip: "+GlobalServerData::serverPrivateVariables.db_common->value(1));
            continue;
        }
        const int32_t &level=GlobalServerData::serverPrivateVariables.db_common->stringtoint32(GlobalServerData::serverPrivateVariables.db_common->value(2),&ok);
        if(!ok)
        {
            normalOutput("reputation level is not a number, skip: "+GlobalServerData::serverPrivateVariables.db_common->value(2));
            continue;
        }
        if(level<-100 || level>100)
        {
            normalOutput("reputation level is <100 or >100, skip: "+std::to_string(type));
            continue;
        }
        if(type>=DictionaryLogin::dictionary_reputation_database_to_internal.size())
        {
            normalOutput("The reputation: "+std::to_string(type)+" don't exist");
            continue;
        }
        if(DictionaryLogin::dictionary_reputation_database_to_internal.at(type)==-1)
        {
            normalOutput("The reputation: "+std::to_string(type)+" not resolved");
            continue;
        }
        if(level>=0)
        {
            if((uint32_t)level>=CommonDatapack::commonDatapack.reputation.at(DictionaryLogin::dictionary_reputation_database_to_internal.at(type)).reputation_positive.size())
            {
                normalOutput("The reputation level "+std::to_string(type)+
                             " is wrong because is out of range (reputation level: "+std::to_string(level)+
                             " > max level: "+
                             std::to_string(CommonDatapack::commonDatapack.reputation.at(DictionaryLogin::dictionary_reputation_database_to_internal.at(type)).reputation_positive.size())+
                             ")");
                continue;
            }
        }
        else
        {
            if((uint32_t)(-level)>CommonDatapack::commonDatapack.reputation.at(DictionaryLogin::dictionary_reputation_database_to_internal.at(type)).reputation_negative.size())
            {
                normalOutput("The reputation level "+std::to_string(type)+
                             " is wrong because is out of range (reputation level: "+std::to_string(level)+
                             " < max level: "+std::to_string(CommonDatapack::commonDatapack.reputation.at(DictionaryLogin::dictionary_reputation_database_to_internal.at(type)).reputation_negative.size())+")");
                continue;
            }
        }
        if(point>0)
        {
            if(CommonDatapack::commonDatapack.reputation.at(DictionaryLogin::dictionary_reputation_database_to_internal.at(type)).reputation_positive.size()==(uint32_t)(level+1))//start at level 0 in positive
            {
                normalOutput("The reputation level is already at max, drop point");
                point=0;
            }
            if(point>=CommonDatapack::commonDatapack.reputation.at(DictionaryLogin::dictionary_reputation_database_to_internal.at(type)).reputation_positive.at(level+1))//start at level 0 in positive
            {
                normalOutput("The reputation point "+std::to_string(point)+
                             " is greater than max "+std::to_string(CommonDatapack::commonDatapack.reputation.at(DictionaryLogin::dictionary_reputation_database_to_internal.at(type)).reputation_positive.at(level)));
                continue;
            }
        }
        else if(point<0)
        {
            if(CommonDatapack::commonDatapack.reputation.at(DictionaryLogin::dictionary_reputation_database_to_internal.at(type)).reputation_negative.size()==(uint32_t)-level)//start at level -1 in negative
            {
                normalOutput("The reputation level is already at min, drop point");
                point=0;
            }
            if(point<CommonDatapack::commonDatapack.reputation.at(DictionaryLogin::dictionary_reputation_database_to_internal.at(type)).reputation_negative.at(-level))//start at level -1 in negative
            {
                normalOutput("The reputation point "+std::to_string(point)+
                             " is greater than max "+std::to_string(CommonDatapack::commonDatapack.reputation.at(DictionaryLogin::dictionary_reputation_database_to_internal.at(type)).reputation_negative.at(level)));
                continue;
            }
        }
        PlayerReputation playerReputation;
        playerReputation.level=level;
        playerReputation.point=point;
        public_and_private_informations.reputation[DictionaryLogin::dictionary_reputation_database_to_internal.at(type)]=playerReputation;
    }
    loadQuests();
}

void Client::loadQuests()
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(PreparedDBQueryServer::db_query_select_quest_by_id.empty())
    {
        errorOutput("loadQuests() Query is empty, bug");
        return;
    }
    #endif
    std::string queryText=PreparedDBQueryServer::db_query_select_quest_by_id;
    stringreplaceOne(queryText,"%1",std::to_string(character_id));
    CatchChallenger::DatabaseBase::CallBack *callback=GlobalServerData::serverPrivateVariables.db_server->asyncRead(queryText,this,&Client::loadQuests_static);
    if(callback==NULL)
    {
        std::cerr << "Sql error for: " << queryText << ", error: " << GlobalServerData::serverPrivateVariables.db_server->errorMessage() << std::endl;
        loadBotAlreadyBeaten();
        return;
    }
    else
        callbackRegistred.push(callback);
}

void Client::loadQuests_static(void *object)
{
    if(object!=NULL)
        static_cast<Client *>(object)->loadQuests_return();
}

void Client::loadQuests_return()
{
    callbackRegistred.pop();
    //do the query
    bool ok,ok2;
    //parse the result
    while(GlobalServerData::serverPrivateVariables.db_server->next())
    {
        PlayerQuest playerQuest;
        const uint32_t &id=GlobalServerData::serverPrivateVariables.db_server->stringtouint32(GlobalServerData::serverPrivateVariables.db_server->value(0),&ok);
        playerQuest.finish_one_time=GlobalServerData::serverPrivateVariables.db_server->stringtobool(GlobalServerData::serverPrivateVariables.db_server->value(1));
        playerQuest.step=GlobalServerData::serverPrivateVariables.db_server->stringtouint32(GlobalServerData::serverPrivateVariables.db_server->value(2),&ok2);
        if(!ok || !ok2)
        {
            normalOutput("wrong value type for quest, skip: "+std::to_string(id));
            continue;
        }
        if(CommonDatapackServerSpec::commonDatapackServerSpec.quests.find(id)==CommonDatapackServerSpec::commonDatapackServerSpec.quests.cend())
        {
            normalOutput("quest is not into the quests list, skip: "+std::to_string(id));
            continue;
        }
        if((playerQuest.step<=0 && !playerQuest.finish_one_time) || playerQuest.step>CommonDatapackServerSpec::commonDatapackServerSpec.quests.at(id).steps.size())
        {
            normalOutput("step out of quest range, skip: "+std::to_string(id));
            continue;
        }
        if(playerQuest.step<=0 && !playerQuest.finish_one_time)
        {
            normalOutput("can't be to step 0 if have never finish the quest, skip: "+std::to_string(id));
            continue;
        }
        public_and_private_informations.quests[id]=playerQuest;
        if(playerQuest.step>0)
            addQuestStepDrop(id,playerQuest.step);
    }
    loadBotAlreadyBeaten();
}
*/
