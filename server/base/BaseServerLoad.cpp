#include "BaseServer.hpp"
#include "Client.hpp"
#include "GlobalServerData.hpp"
#include "../../general/tinyXML2/tinyxml2.hpp"
#include "../../general/tinyXML2/customtinyxml2.hpp"
#include "../../general/base/CommonDatapack.hpp"
#include "../../general/base/CommonDatapackServerSpec.hpp"

using namespace CatchChallenger;

void BaseServer::preload_4_sync_randomBlock()
{
    //don't use BaseServerLogin::fpRandomFile to prevent lost entropy
    GlobalServerData::serverPrivateVariables.randomData.resize(CATCHCHALLENGER_SERVER_RANDOM_INTERNAL_SIZE);
    int index=0;
    while(index<CATCHCHALLENGER_SERVER_RANDOM_INTERNAL_SIZE)
    {
        GlobalServerData::serverPrivateVariables.randomData[index]=uint8_t(rand()%256);
        index++;
    }
}

void BaseServer::preload_5_sync_the_events()
{
    GlobalServerData::serverPrivateVariables.events.clear();
    unsigned int index=0;
    while(index<CommonDatapack::commonDatapack.get_events().size())
    {
        GlobalServerData::serverPrivateVariables.events.push_back(0);
        index++;
    }
    {
        const uint64_t &time=sFrom1970();
        auto i = GlobalServerData::serverSettings.programmedEventList.begin();
        while(i!=GlobalServerData::serverSettings.programmedEventList.end())
        {
            unsigned int index=0;
            while(index<CommonDatapack::commonDatapack.get_events().size())
            {
                const Event &event=CommonDatapack::commonDatapack.get_events().at(index);
                if(event.name==i->first)
                {
                    #ifdef CATCHCHALLENGER_GAMESERVER_EVENTSTARTONLOCALTIME
                    std::map<uint64_t,uint32_t> pastEventStart;
                    #endif
                    const std::unordered_map<std::string/*groupName, example: day/night*/,CatchChallenger::GameServerSettings::ProgrammedEvent> &programmedEvent=i->second;
                    auto j = programmedEvent.begin();
                    while (j!=programmedEvent.cend())
                    {
                        // event.values is std::vector<std::string >
                        const CatchChallenger::GameServerSettings::ProgrammedEvent &programmedEvent=j->second;
                        const auto &iter = std::find(event.values.begin(), event.values.end(), programmedEvent.value);
                        const size_t &sub_index = std::distance(event.values.begin(), iter);
                        if(sub_index<event.values.size())
                        {
                            uint64_t nextStart=time/(programmedEvent.cycle*60)*(programmedEvent.cycle*60);
                            while(nextStart<=time)
                                nextStart+=(programmedEvent.cycle*60);
                            uint64_t previousStart=nextStart;
                            while(previousStart>time)
                                previousStart-=(programmedEvent.cycle*60);
                            pastEventStart[previousStart]=static_cast<uint8_t>(sub_index);
                            setEventTimer(static_cast<uint8_t>(index),static_cast<uint8_t>(sub_index),static_cast<unsigned int>(programmedEvent.cycle*1000*60),static_cast<unsigned int>(time-nextStart-1000));
                        }
                        else
                            GlobalServerData::serverSettings.programmedEventList[i->first].erase(i->first);
                        ++j;
                    }
                    #ifdef CATCHCHALLENGER_GAMESERVER_EVENTSTARTONLOCALTIME
                    if(!pastEventStart.empty())
                    {
                        const uint32_t value=pastEventStart.crbegin()->second;
                        CatchChallenger::Client::setEvent(static_cast<uint8_t>(index),static_cast<uint8_t>(value));
                    }
                    #endif
                    break;
                }
                index++;
            }
            if(index==CommonDatapack::commonDatapack.get_events().size())
                GlobalServerData::serverSettings.programmedEventList.erase(i->first);
            ++i;
        }
    }
}

void BaseServer::preload_3_sync_the_ddos()
{
    unload_the_ddos();
    Client::generalChatDrop.reset();
    Client::clanChatDrop.reset();
    Client::privateChatDrop.reset();
}

bool BaseServer::preload_zone_init()
{
    unsigned int index=0;
    while(index<entryListZone.size())
    {
        if(!stringEndsWith(entryListZone.at(index).name,".xml"))
        {
            std::cerr << entryListZone.at(index).name << " the zone file name not match" << std::endl;
            index++;
            continue;
        }
        std::string zoneCodeName=entryListZone.at(index).name;
        stringreplaceOne(zoneCodeName,".xml","");
        const std::string &file=entryListZone.at(index).absoluteFilePath;
        tinyxml2::XMLDocument *domDocument;
        domDocument=new tinyxml2::XMLDocument();
        const auto loadOkay = domDocument->LoadFile(file.c_str());
        if(loadOkay!=0)
        {
            std::cerr << file+", "+tinyxml2errordoc(domDocument) << std::endl;
            return false;
        }
        const tinyxml2::XMLElement *root = domDocument->RootElement();
        if(root==NULL)
        {
            index++;
            continue;
        }
        if(strcmp(root->Name(),"zone")!=0)
        {
            std::cerr << "Unable to open the file: " << file.c_str() << ", \"zone\" root balise not found for the xml file" << std::endl;
            index++;
            continue;
        }

        //load capture
        std::vector<uint16_t> fightIdList;
        const tinyxml2::XMLElement * capture=root->FirstChildElement("capture");
        if(capture!=NULL)
        {
            if(capture->Attribute("fightId")!=NULL)
            {
                /// TODO remake this part
                /*bool ok;
                const std::vector<std::string> &fightIdStringList=stringsplit(capture->Attribute("fightId"),';');
                unsigned int sub_index=0;
                while(sub_index<fightIdStringList.size())
                {
                    const uint16_t &fightId=stringtouint16(fightIdStringList.at(sub_index),&ok);
                    if(ok)
                    {
                        if(CommonDatapackServerSpec::commonDatapackServerSpec.get_botFights().find(fightId)==
                                CommonDatapackServerSpec::commonDatapackServerSpec.get_botFights().end())
                            std::cerr << "bot fightId " << fightId << " not found for capture zone " << zoneCodeName << std::endl;
                        else
                            fightIdList.push_back(fightId);
                    }
                    sub_index++;
                }
                if(sub_index==fightIdStringList.size() && !fightIdList.empty())
                {
                    const ZONE_TYPE &zoneId=CommonDatapackServerSpec::commonDatapackServerSpec.get_zoneToId().at(zoneCodeName);
                    while(GlobalServerData::serverPrivateVariables.captureFightIdListByZoneToCaptureCity.size()<zoneId)
                    {
                        std::vector<uint16_t> t;
                        GlobalServerData::serverPrivateVariables.captureFightIdListByZoneToCaptureCity.push_back(t);
                    }
                    GlobalServerData::serverPrivateVariables.captureFightIdListByZoneToCaptureCity[zoneId]=fightIdList;
                }*/
                break;
            }
            else
                std::cerr << "Unable to open the file: " << file << ", is not found" << std::endl;
        }
        #ifdef EPOLLCATCHCHALLENGERSERVER
        delete domDocument;
        #endif
        index++;
    }

    std::cout << GlobalServerData::serverPrivateVariables.captureFightIdListByZoneToCaptureCity.size() << " zone(s) loaded" << std::endl;
    return true;
}

bool BaseServer::preload_the_city_capture()
{
    return load_next_city_capture();
}

void BaseServer::preload_7_sync_the_skin()
{
    std::vector<std::string> skinFolderList=FacilityLibGeneral::skinIdList(GlobalServerData::serverSettings.datapack_basePath+DATAPACK_BASE_PATH_SKIN);
    unsigned int index=0;
    while(index<skinFolderList.size() && index<255)
    {
        GlobalServerData::serverPrivateVariables.skinList[skinFolderList.at(index)]=static_cast<uint8_t>(index);
        index++;
    }

    std::cout << skinFolderList.size() << " skin(s) loaded" << std::endl;
}

void BaseServer::preload_11_sync_the_players()
{
    Client::simplifiedIdList.clear();
    switch(GlobalServerData::serverSettings.mapVisibility.mapVisibilityAlgorithm)
    {
        default:
        case MapVisibilityAlgorithmSelection_Simple:
        case MapVisibilityAlgorithmSelection_WithBorder:
        {
            uint16_t index=0;
            while(index<GlobalServerData::serverSettings.max_players)
            {
                Client::simplifiedIdList.push_back(index);
                index++;
            }
            std::random_shuffle(Client::simplifiedIdList.begin(),Client::simplifiedIdList.end());
        }
        break;
        case MapVisibilityAlgorithmSelection_None:
        break;
    }
}

void BaseServer::preload_the_visibility_algorithm()
{
    switch(GlobalServerData::serverSettings.mapVisibility.mapVisibilityAlgorithm)
    {
        case MapVisibilityAlgorithmSelection_Simple:
            std::cout << "Visibility: MapVisibilityAlgorithmSelection_Simple" << std::endl;
        break;
        case MapVisibilityAlgorithmSelection_WithBorder:
            std::cout << "Visibility: MapVisibilityAlgorithmSelection_WithBorder" << std::endl;
        break;
        case MapVisibilityAlgorithmSelection_None:
            std::cout << "Visibility: MapVisibilityAlgorithmSelection_None" << std::endl;
        break;
    }
}
