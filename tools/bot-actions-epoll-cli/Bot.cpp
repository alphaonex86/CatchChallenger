#include "Bot.hpp"
#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#include "../../server/gateway/DatapackDownloaderBase.hpp"
#include "../../server/gateway/DatapackDownloaderMainSub.hpp"
#include "../../general/base/FacilityLibGeneral.hpp"
#include "../../general/xxhash/xxhash.h"
#include <sys/stat.h>

sockaddr_in6 Bot::serv_addr;
std::string Bot::login;
std::string Bot::pass;

static const std::string text_double_slash="//";
static const std::string text_dotslash="./";
static const std::string text_antislash="\\";
static const std::string text_slash="/";

Bot::Bot(const int &infd) :
    EpollClient(infd)
{
    Api_protocol::resetAll();//->needed for     mLastGivenXP=0; and other stuff
    if(stageConnexion!=StageConnexion::Stage1 && stageConnexion!=StageConnexion::Stage2 && stageConnexion!=StageConnexion::Stage3)
    {
        newError(std::string("Internal problem"),std::string("Api_protocol_Qt::readForFirstHeader() stageConnexion!=StageConnexion::Stage1 && stageConnexion!=StageConnexion::Stage2: ")+std::to_string(stageConnexion));
        abort();
    }
    parseIncommingData();

    if(login.empty() || pass.empty())
    {
        std::cerr << "login or pass is empty()" << std::endl;
        abort();
    }
    std::string tempLogin(login);
    if(tempLogin.find("%number%")!=std::string::npos)
        tempLogin.replace(tempLogin.find("%number%"), sizeof("%number%") - 1, "1");
    std::string tempPass(pass);
    if(tempPass.find("%number%")!=std::string::npos)
        tempPass.replace(tempPass.find("%number%"), sizeof("%number%") - 1, "1");
    tryLogin(tempLogin,tempPass);
}

Bot::~Bot()
{
/*    if(replySelectListInWait!=NULL)
    {
        vectorremoveOne(DatapackDownloaderBase::datapackDownloaderBase->clientInSuspend,this);
        delete replySelectListInWait;
        replySelectListInWait=NULL;
    }
    if(replySelectCharInWait!=NULL)
    {
        if(DatapackDownloaderMainSub::datapackDownloaderMainSub.find(main)==DatapackDownloaderMainSub::datapackDownloaderMainSub.cend())
        {}
        else if(DatapackDownloaderMainSub::datapackDownloaderMainSub.at(main).find(sub)==DatapackDownloaderMainSub::datapackDownloaderMainSub.at(main).cend())
        {}
        else
        {
            DatapackDownloaderMainSub * const downloader=DatapackDownloaderMainSub::datapackDownloaderMainSub.at(main).at(sub);
            vectorremoveOne(downloader->clientInSuspend,this);
        }
        delete replySelectCharInWait;
        replySelectCharInWait=NULL;
    }
    if(DatapackDownloaderBase::datapackDownloaderBase==NULL)
    {
        delete DatapackDownloaderBase::datapackDownloaderBase;
        DatapackDownloaderBase::datapackDownloaderBase=NULL;
    }*/
}

bool Bot::parseServerHostAndPort(const char * const host,const char * const port)
{
    const int PORT=std::stoi(port);
    if(PORT<1 || PORT>65535)
    {
        std::cerr << "wrong port number of not a number: " << port << std::endl;
        return false;
    }

    memset(&serv_addr,0,sizeof(serv_addr));
    serv_addr.sin6_family = AF_INET;
    serv_addr.sin6_port = htons(PORT);
    if (inet_pton(AF_INET6, host, &serv_addr.sin6_addr) <= 0)
    {
        if (inet_pton(AF_INET, host, &serv_addr.sin6_addr) <= 0)
        {
            serv_addr.sin6_family = AF_INET;
            printf("Invalid address/Address not supported\n");
            return false;
        }
    }
    return true;
}

void Bot::setLoginPass(const char * const login,const char * const pass)
{
    Bot::login=login;
    Bot::pass=pass;
}

sockaddr_in6 Bot::get_serv_addr()
{
    return serv_addr;
}

ssize_t Bot::readFromSocket(char * data, const size_t &size)
{
    return CatchChallenger::EpollClient::read(data,size);
}

ssize_t Bot::writeToSocket(const char * const data, const size_t &size)
{
    return CatchChallenger::EpollClient::write(data,size);
}

bool Bot::haveBeatBot(const uint16_t &botFightId) const
{
    (void)botFightId;
    return false;
}
void Bot::sendDatapackContentBase(const std::string &hashBase)
{
    (void)hashBase;
}
void Bot::sendDatapackContentMainSub(const std::string &hashMain,const std::string &hashSub)
{
    (void)hashMain;
    (void)hashSub;
}
void Bot::tryDisconnect()
{}
void Bot::readForFirstHeader()
{
    if(haveFirstHeader)
        return;
    if(!EpollClient::isValid())
    {
        newError(std::string("Internal problem"),std::string("Api_protocol_Qt::readForFirstHeader() socket->sslSocket==NULL"));
        abort();
    }
    if(stageConnexion!=StageConnexion::Stage1 && stageConnexion!=StageConnexion::Stage2 && stageConnexion!=StageConnexion::Stage3)
    {
        newError(std::string("Internal problem"),std::string("Api_protocol_Qt::readForFirstHeader() stageConnexion!=StageConnexion::Stage1 && stageConnexion!=StageConnexion::Stage2: ")+std::to_string(stageConnexion));
        abort();
    }
    if(stageConnexion==StageConnexion::Stage2)
    {
        message("stageConnexion=CatchChallenger::Api_protocol_Qt::StageConnexion::Stage3 set at "+std::string(__FILE__)+":"+std::to_string(__LINE__));
        stageConnexion=StageConnexion::Stage3;
    }
    {
        uint8_t value;
        if(EpollClient::read((char*)&value,sizeof(value))==sizeof(value))
        {
            haveFirstHeader=true;
            if(value==0x01)
            {
                std::cerr << "Sorry but TLS not supported for now (abort)" << std::endl;
                abort();
            }
            else
                connectTheExternalSocketInternal();
        }
    }
}
void Bot::defineMaxPlayers(const uint16_t &maxPlayers)
{
    (void)maxPlayers;
}
void Bot::newError(const std::string &error,const std::string &detailedError)
{
    std::cerr << error << ": " << detailedError << std::endl;
}
void Bot::message(const std::string &message)
{
    std::cerr << message << std::endl;
}
void Bot::lastReplyTime(const uint32_t &time)
{
    (void)time;
}

//protocol/connection info
void Bot::disconnected(const std::string &reason)
{
    std::cerr << "disconnected: " << reason << std::endl;
}
void Bot::notLogged(const std::string &reason)
{
    std::cerr << "notLogged: " << reason << std::endl;
}
//const std::vector<ServerFromPoolForDisplay> &serverOrdenedList,
void Bot::logged(const std::vector<std::vector<CatchChallenger::CharacterEntry> > &characterEntryList)
{
    std::cerr << "logged: " << std::endl;
    unsigned int index=0;
    while(index<characterEntryList.size())
    {
        unsigned int subindex=0;
        while(subindex<characterEntryList.at(index).size())
        {
            const CatchChallenger::CharacterEntry &c=characterEntryList.at(index).at(subindex);
            std::cerr << index << ") " << c.pseudo << std::endl;
            subindex++;
        }
        index++;
    }
    sendDatapackContentBase();
}
void Bot::protocol_is_good()
{
}
void Bot::connectedOnLoginServer()
{
    std::cerr << "connectedOnLoginServer" << std::endl;
}
void Bot::connectingOnGameServer()
{}
void Bot::connectedOnGameServer()
{
    std::cerr << "connectedOnGameServer" << std::endl;
}
void Bot::haveDatapackMainSubCode()
{
    std::cerr << "haveDatapackMainSubCode" << std::endl;
}
void Bot::gatewayCacheUpdate(const uint8_t gateway,const uint8_t progression)
{
    (void)gateway;
    (void)progression;
}

//general info
void Bot::number_of_player(const uint16_t &number,const uint16_t &max_players)
{
    (void)number;
    (void)max_players;
}
void Bot::random_seeds(const std::string &data)
{
    (void)data;
}

//character
void Bot::newCharacterId(const uint8_t &returnCode,const uint32_t &characterId)
{
    (void)returnCode;
    (void)characterId;
}
void Bot::haveCharacter()
{
    std::cerr << "haveCharacter" << std::endl;
    sendDatapackContentMainSub();
}
//events
void Bot::setEvents(const std::vector<std::pair<uint8_t,uint8_t> > &events)
{
    (void)events;
}
void Bot::newEvent(const uint8_t &event,const uint8_t &event_value)
{
    (void)event;
    (void)event_value;
}

//map move
void Bot::insert_player(const CatchChallenger::Player_public_informations &player,const uint32_t &mapId,const uint8_t &x,const uint8_t &y,const CatchChallenger::Direction &direction)
{
    std::cerr << "insert_player" << std::endl;
    (void)player;
    (void)mapId;
    (void)x;
    (void)y;
    (void)direction;
}
void Bot::move_player(const uint16_t &id,const std::vector<std::pair<uint8_t,CatchChallenger::Direction> > &movement)
{
    (void)id;
    (void)movement;
}
void Bot::remove_player(const uint16_t &id)
{
    (void)id;
}
void Bot::reinsert_player(const uint16_t &id,const uint8_t &x,const uint8_t &y,const CatchChallenger::Direction &direction)
{
    (void)id;
    (void)x;
    (void)y;
    (void)direction;
}
void Bot::full_reinsert_player(const uint16_t &id,const uint32_t &mapId,const uint8_t &x,const uint8_t y,const CatchChallenger::Direction &direction)
{
    std::cerr << "full_reinsert_player" << std::endl;
    (void)id;
    (void)mapId;
    (void)x;
    (void)y;
    (void)direction;
}
void Bot::dropAllPlayerOnTheMap()
{}
void Bot::teleportTo(const uint32_t &mapId,const uint8_t &x,const uint8_t &y,const CatchChallenger::Direction &direction)
{
    (void)mapId;
    (void)x;
    (void)y;
    (void)direction;
}

//plant
void Bot::insert_plant(const uint32_t &mapId,const uint8_t &x,const uint8_t &y,const uint8_t &plant_id,const uint16_t &seconds_to_mature)
{
    (void)mapId;
    (void)x;
    (void)y;
    (void)plant_id;
    (void)seconds_to_mature;
}
void Bot::remove_plant(const uint32_t &mapId,const uint8_t &x,const uint8_t &y)
{
    (void)mapId;
    (void)x;
    (void)y;
}
void Bot::seed_planted(const bool &ok)
{
    (void)ok;
}
void Bot::plant_collected(const CatchChallenger::Plant_collect &stat)
{
    (void)stat;
}
//crafting
void Bot::recipeUsed(const CatchChallenger::RecipeUsage &recipeUsage)
{
    (void)recipeUsage;
}
//inventory
void Bot::objectUsed(const CatchChallenger::ObjectUsage &objectUsage)
{
    (void)objectUsage;
}
void Bot::monsterCatch(const bool &success)
{
    (void)success;
}

//chat
void Bot::new_chat_text(const CatchChallenger::Chat_type &chat_type,const std::string &text,const std::string &pseudo,const CatchChallenger::Player_type &player_type)
{
    (void)chat_type;
    (void)text;
    (void)pseudo;
    (void)player_type;
}
void Bot::new_system_text(const CatchChallenger::Chat_type &chat_type,const std::string &text)
{
    (void)chat_type;
    (void)text;
}

//player info
void Bot::have_current_player_info(const CatchChallenger::Player_private_and_public_informations &informations)
{
    std::cerr << "have_current_player_info" << std::endl;
    (void)informations;
}
void Bot::have_inventory(const std::unordered_map<uint16_t,uint32_t> &items,const std::unordered_map<uint16_t,uint32_t> &warehouse_items)
{
    (void)items;
    (void)warehouse_items;
}
void Bot::add_to_inventory(const std::unordered_map<uint16_t,uint32_t> &items)
{
    (void)items;
}
void Bot::remove_to_inventory(const std::unordered_map<uint16_t,uint32_t> &items)
{
    (void)items;
}

//datapack
void Bot::haveTheDatapack()
{
    std::cerr << "haveTheDatapack" << std::endl;

    if(httpDatapackMirrorBaseList.empty())
        std::cout << "Have the datapack base" << std::endl;

    //regen the datapack cache
    /*if(CatchChallenger::LinkToGameServer::httpDatapackMirrorRewriteBase.size()<=1)
        EpollClientLoginSlave::datapack_file_base.datapack_file_hash_cache=EpollClientLoginSlave::datapack_file_list(mDatapackBase);*/
}
void Bot::haveTheDatapackMainSub()
{
    std::cerr << "haveTheDatapackMainSub" << std::endl;

    if(httpDatapackMirrorServerList.empty())
        std::cout << "Have the datapack main sub" << std::endl;

    //regen the datapack cache
    /*if(LinkToGameServer::httpDatapackMirrorRewriteMainAndSub.size()<=1)
    {
        //mDatapackMain(mDatapackBase+"map/main/"+mainDatapackCode+"/"),
        EpollClientLoginSlave::datapack_file_main[mainDatapackCode].datapack_file_hash_cache=EpollClientLoginSlave::datapack_file_list(mDatapackMain);
        if(!mDatapackSub.empty())
            //mDatapackSub=mDatapackBase+"map/main/"+mainDatapackCode+"/sub/"+subDatapackCode+"/";
            EpollClientLoginSlave::datapack_file_sub[mainDatapackCode][subDatapackCode].datapack_file_hash_cache=EpollClientLoginSlave::datapack_file_list(mDatapackSub);
        else
            EpollClientLoginSlave::datapack_file_sub[mainDatapackCode][subDatapackCode].datapack_file_hash_cache.clear();
    }*/
}
//base
void Bot::newFileBase(const std::string &fileName,const std::string &data)
{
    writeNewFileBase(fileName,data);
}
void Bot::newHttpFileBase(const std::string &url,const std::string &fileName)
{
    (void)url;
    (void)fileName;
}
void Bot::removeFileBase(const std::string &fileName)
{
    (void)fileName;
}
void Bot::datapackSizeBase(const uint32_t &datapckFileNumber,const uint32_t &datapckFileSize)
{
    (void)datapckFileNumber;
    (void)datapckFileSize;
}
//main
void Bot::newFileMain(const std::string &fileName,const std::string &data)
{
    writeNewFileMain(fileName,data);
}
void Bot::newHttpFileMain(const std::string &url,const std::string &fileName)
{
    (void)url;
    (void)fileName;
}
void Bot::removeFileMain(const std::string &fileName)
{
    (void)fileName;
}
void Bot::datapackSizeMain(const uint32_t &datapckFileNumber,const uint32_t &datapckFileSize)
{
    (void)datapckFileNumber;
    (void)datapckFileSize;
}
//sub
void Bot::newFileSub(const std::string &fileName,const std::string &data)
{
    writeNewFileSub(fileName,data);
}
void Bot::newHttpFileSub(const std::string &url,const std::string &fileName)
{
    (void)url;
    (void)fileName;
}
void Bot::removeFileSub(const std::string &fileName)
{
    (void)fileName;
}
void Bot::datapackSizeSub(const uint32_t &datapckFileNumber,const uint32_t &datapckFileSize)
{
    (void)datapckFileNumber;
    (void)datapckFileSize;
}

//shop
void Bot::haveShopList(const std::vector<CatchChallenger::ItemToSellOrBuy> &items)
{
    (void)items;
}
void Bot::haveBuyObject(const CatchChallenger::BuyStat &stat,const uint32_t &newPrice)
{
    (void)stat;
    (void)newPrice;
}
void Bot::haveSellObject(const CatchChallenger::SoldStat &stat,const uint32_t &newPrice)
{
    (void)stat;
    (void)newPrice;
}

//factory
void Bot::haveFactoryList(const uint32_t &remainingProductionTime,const std::vector<CatchChallenger::ItemToSellOrBuy> &resources,const std::vector<CatchChallenger::ItemToSellOrBuy> &products)
{
    (void)remainingProductionTime;
    (void)resources;
    (void)products;
}
void Bot::haveBuyFactoryObject(const CatchChallenger::BuyStat &stat,const uint32_t &newPrice)
{
    (void)stat;
    (void)newPrice;
}
void Bot::haveSellFactoryObject(const CatchChallenger::SoldStat &stat,const uint32_t &newPrice)
{
    (void)stat;
    (void)newPrice;
}

//trade
void Bot::tradeRequested(const std::string &pseudo,const uint8_t &skinInt)
{
    (void)pseudo;
    (void)skinInt;
}
void Bot::tradeAcceptedByOther(const std::string &pseudo,const uint8_t &skinInt)
{
    (void)pseudo;
    (void)skinInt;
}
void Bot::tradeCanceledByOther()
{}
void Bot::tradeFinishedByOther()
{}
void Bot::tradeValidatedByTheServer()
{}
void Bot::tradeAddTradeCash(const uint64_t &cash)
{
    (void)cash;
}
void Bot::tradeAddTradeObject(const uint32_t &item,const uint32_t &quantity)
{
    (void)item;
    (void)quantity;
}
void Bot::tradeAddTradeMonster(const CatchChallenger::PlayerMonster &monster)
{
    (void)monster;
}

//battle
void Bot::battleRequested(const std::string &pseudo,const uint8_t &skinInt)
{
    (void)pseudo;
    (void)skinInt;
}
void Bot::battleAcceptedByOther(const std::string &pseudo,const uint8_t &skinId,const std::vector<uint8_t> &stat,const uint8_t &monsterPlace,const CatchChallenger::PublicPlayerMonster &publicPlayerMonster)
{
    (void)pseudo;
    (void)skinId;
    (void)stat;
    (void)monsterPlace;
    (void)publicPlayerMonster;
}
void Bot::battleCanceledByOther()
{}
void Bot::sendBattleReturn(const std::vector<CatchChallenger::Skill::AttackReturn> &attackReturn)
{
    (void)attackReturn;
}

//clan
void Bot::clanActionSuccess(const uint32_t &clanId)
{
    (void)clanId;
}
void Bot::clanActionFailed()
{}
void Bot::clanDissolved()
{}
void Bot::clanInformations(const std::string &name)
{
    (void)name;
}
void Bot::clanInvite(const uint32_t &clanId,const std::string &name)
{
    (void)clanId;
    (void)name;
}
void Bot::cityCapture(const uint32_t &remainingTime,const uint8_t &type)
{
    (void)remainingTime;
    (void)type;
}

//city
void Bot::captureCityYourAreNotLeader()
{}
void Bot::captureCityYourLeaderHaveStartInOtherCity(const std::string &zone)
{
    (void)zone;
}
void Bot::captureCityPreviousNotFinished()
{}
void Bot::captureCityStartBattle(const uint16_t &player_count,const uint16_t &clan_count)
{
    (void)player_count;
    (void)clan_count;
}
void Bot::captureCityStartBotFight(const uint16_t &player_count,const uint16_t &clan_count,const uint32_t &fightId)
{
    (void)player_count;
    (void)clan_count;
    (void)fightId;
}
void Bot::captureCityDelayedStart(const uint16_t &player_count,const uint16_t &clan_count)
{
    (void)player_count;
    (void)clan_count;
}
void Bot::captureCityWin()
{}
std::string Bot::getLanguage() const
{
    return "en";
}

void Bot::parseIncommingData()
{
    if(!haveFirstHeader)
        readForFirstHeader();
    if(haveFirstHeader)
        Api_protocol::parseIncommingData();
}

void Bot::closeSocket()
{
    CatchChallenger::EpollClient::close();
}

std::unordered_map<std::string,uint32_t/*partialHash*/> Bot::datapack_file_list(const std::string &path,const bool withHash)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(path.empty())
    {
        //mostly for listFolderNotRecursive()/listFolder() protect
        std::cerr << "can't EpollClientLoginSlave::datapack_file_list(\"\")" << std::endl;
        abort();
    }
    #endif
    std::unordered_map<std::string,uint32_t> filesList;

    std::vector<std::string> returnList;
    returnList=CatchChallenger::FacilityLibGeneral::listFolder(path);

    unsigned int index=0;
    while(index<returnList.size())
    {
        #ifdef _WIN32
        std::string fileName=returnList.at(index);
        #else
        const std::string &fileName=returnList.at(index);
        #endif
        const std::string &suffix=CatchChallenger::FacilityLibGeneral::getSuffixAndValidatePathFromFS(fileName);
        //if(regex_search(fileName,GlobalServerData::serverPrivateVariables.datapack_rightFileName))
        //try replace by better algo
        if(!suffix.empty())
        {
//            const std::string &suffix=FacilityLibGeneral::getSuffix(fileName);
            if(!suffix.empty() &&
                    DatapackDownloaderBase::extensionAllowed.find(suffix)
                    !=DatapackDownloaderBase::extensionAllowed.cend())
            {
                uint32_t datapackCacheFile;
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
                                const std::vector<char> &data=CatchChallenger::FacilityLibGeneral::readAllFileAndClose(filedesc);

                                uint32_t h=0;
                                XXH32_canonical_t htemp;
                                XXH32_canonicalFromHash(&htemp,XXH32(data.data(),data.size(),0));
                                memcpy(&h,&htemp.digest,sizeof(h));

                                datapackCacheFile=h;
                                filesList[fileName]=datapackCacheFile;
                            }
                            else
                            {
                                datapackCacheFile=0;
                                std::cerr << "Client::datapack_file_list fopen failed on " +path+returnList.at(index)+ ":"+std::to_string(errno) << std::endl;
                            }
                        }
                        else
                        {
                            datapackCacheFile=0;
                            std::cerr << "Client::datapack_file_list file too big failed on " +path+returnList.at(index)+ ":"+std::to_string(buf.st_size) << std::endl;
                        }
                    }
                    else
                    {
                        datapackCacheFile=0;
                        std::cerr << "Client::datapack_file_list stat failed on " +path+returnList.at(index)+ ":"+std::to_string(errno) << std::endl;
                    }
                }
                else
                {
                    datapackCacheFile=0;
                    filesList[fileName]=datapackCacheFile;
                }
            }
        }
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        else
        {
            std::cerr << "For Client::datapack_file_list(" << path << "," << withHash << ")" << std::endl;
            std::cerr << "FacilityLibGeneral::getSuffixAndValidatePathFromFS(" << fileName << ") return empty result" << std::endl;
            //const std::string &suffix2=FacilityLibGeneral::getSuffixAndValidatePathFromFS(fileName);
        }
        #endif
        index++;
    }
    return filesList;
}
