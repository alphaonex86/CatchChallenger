#include "BaseServer.h"
#include "GlobalServerData.h"

using namespace CatchChallenger;

void BaseServer::preload_randomBlock()
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

void BaseServer::preload_the_events()
{
    GlobalServerData::serverPrivateVariables.events.clear();
    unsigned int index=0;
    while(index<CommonDatapack::commonDatapack.events.size())
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
            while(index<CommonDatapack::commonDatapack.events.size())
            {
                const Event &event=CommonDatapack::commonDatapack.events.at(index);
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
                            pastEventStart[previousStart]=sub_index;
                            #ifdef EPOLLCATCHCHALLENGERSERVER
                                TimerEvents * const timer=new TimerEvents(index,sub_index);
                                GlobalServerData::serverPrivateVariables.timerEvents.push_back(timer);
                                timer->start(programmedEvent.cycle*1000*60,(time-nextStart-1000));
                            #else
                            GlobalServerData::serverPrivateVariables.timerEvents.push_back(new QtTimerEvents(programmedEvent.cycle*1000*60,(time-nextStart-1000),index,sub_index));
                            #endif
                        }
                        else
                            GlobalServerData::serverSettings.programmedEventList[i->first].erase(i->first);
                        ++j;
                    }
                    #ifdef CATCHCHALLENGER_GAMESERVER_EVENTSTARTONLOCALTIME
                    if(!pastEventStart.empty())
                    {
                        const uint32_t value=pastEventStart.crbegin()->second;
                        CatchChallenger::Client::setEvent(index,value);
                    }
                    #endif
                    break;
                }
                index++;
            }
            if(index==CommonDatapack::commonDatapack.events.size())
                GlobalServerData::serverSettings.programmedEventList.erase(i->first);
            ++i;
        }
    }
}

void BaseServer::preload_the_ddos()
{
    unload_the_ddos();
    int index=0;
    while(index<CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE)
    {
        Client::generalChatDrop.push_back(0);
        Client::clanChatDrop.push_back(0);
        Client::privateChatDrop.push_back(0);
        index++;
    }
    Client::generalChatDropTotalCache=0;
    Client::generalChatDropNewValue=0;
    Client::clanChatDropTotalCache=0;
    Client::clanChatDropNewValue=0;
    Client::privateChatDropTotalCache=0;
    Client::privateChatDropNewValue=0;
}

#ifndef EPOLLCATCHCHALLENGERSERVER
bool BaseServer::preload_zone_init()
{
    const unsigned int &listsize=entryListZone.size();
    unsigned int index=0;
    while(index<listsize)
    {
        if(!regex_search(entryListZone.at(index).name,regexXmlFile))
        {
            std::cerr << entryListZone.at(index).name << " the zone file name not match" << std::endl;
            index++;
            continue;
        }
        std::string zoneCodeName=entryListZone.at(index).name;
        stringreplaceOne(zoneCodeName,CACHEDSTRING_dotxml,"");
        const std::string &file=entryListZone.at(index).absoluteFilePath;
        CATCHCHALLENGER_XMLDOCUMENT *domDocument;
        #ifndef EPOLLCATCHCHALLENGERSERVER
        if(CommonDatapack::commonDatapack.xmlLoadedFile.find(file)!=CommonDatapack::commonDatapack.xmlLoadedFile.cend())
            domDocument=&CommonDatapack::commonDatapack.xmlLoadedFile[file];
        else
        {
            domDocument=&CommonDatapack::commonDatapack.xmlLoadedFile[file];
            #else
            domDocument=new CATCHCHALLENGER_XMLDOCUMENT();
            #endif
            const auto loadOkay = domDocument->CATCHCHALLENGER_XMLDOCUMENTLOAD(CATCHCHALLENGER_XMLSTDSTRING_TONATIVESTRING(file));
            if(!CATCHCHALLENGER_XMLDOCUMENTRETURNISLOADED(loadOkay))
            {
                std::cerr << file+", "+CATCHCHALLENGER_XMLDOCUMENTERROR(domDocument) << std::endl;
                index++;
                continue;
            }
            #ifndef EPOLLCATCHCHALLENGERSERVER
        }
        #endif
        #ifndef EPOLLCATCHCHALLENGERSERVER
        auto search = GlobalServerData::serverPrivateVariables.captureFightIdListByZoneToCaptureCity.find(zoneCodeName);
        if(search != GlobalServerData::serverPrivateVariables.captureFightIdListByZoneToCaptureCity.end())
        {
            std::cerr << "Unable to open the file: " << file.c_str() << ", zone code name already found" << std::endl;
            index++;
            continue;
        }
        #endif
        const CATCHCHALLENGER_XMLELEMENT* root = domDocument->RootElement();
        if(root==NULL)
        {
            index++;
            continue;
        }
        if(!CATCHCHALLENGER_XMLNATIVETYPECOMPAREISSAME(root->CATCHCHALLENGER_XMLELENTVALUE(),XMLCACHEDSTRING_zone))
        {
            std::cerr << "Unable to open the file: " << file.c_str() << ", \"zone\" root balise not found for the xml file" << std::endl;
            index++;
            continue;
        }

        //load capture
        std::vector<uint16_t> fightIdList;
        const CATCHCHALLENGER_XMLELEMENT * capture=root->FirstChildElement(XMLCACHEDSTRING_capture);
        if(capture!=NULL)
        {
            if(CATCHCHALLENGER_XMLELENTISXMLELEMENT(capture) && capture->Attribute(XMLCACHEDSTRING_fightId)!=NULL)
            {
                bool ok;
                const std::vector<std::string> &fightIdStringList=stringsplit(CATCHCHALLENGER_XMLATTRIBUTETOSTRING(capture->Attribute(XMLCACHEDSTRING_fightId)),';');
                int sub_index=0;
                const int &listsize=fightIdStringList.size();
                while(sub_index<listsize)
                {
                    const uint16_t &fightId=stringtouint16(fightIdStringList.at(sub_index),&ok);
                    if(ok)
                    {
                        if(CommonDatapackServerSpec::commonDatapackServerSpec.botFights.find(fightId)==CommonDatapackServerSpec::commonDatapackServerSpec.botFights.end())
                            std::cerr << "bot fightId " << fightId << " not found for capture zone " << zoneCodeName << std::endl;
                        else
                            fightIdList.push_back(fightId);
                    }
                    sub_index++;
                }
                if(sub_index==listsize && !fightIdList.empty())
                    GlobalServerData::serverPrivateVariables.captureFightIdListByZoneToCaptureCity[zoneCodeName]=fightIdList;
                break;
            }
            else
                std::cerr << "Unable to open the file: " << file << ", is not an element: (at line: " << CATCHCHALLENGER_XMLELENTATLINE(capture) << ")" << std::endl;
        }
        #ifdef EPOLLCATCHCHALLENGERSERVER
        delete domDocument;
        #endif
        index++;
    }

    std::cout << GlobalServerData::serverPrivateVariables.captureFightIdListByZoneToCaptureCity.size() << " zone(s) loaded" << std::endl;
    return true;
}
#endif

#ifndef EPOLLCATCHCHALLENGERSERVER
bool BaseServer::preload_the_city_capture()
{
    if(GlobalServerData::serverPrivateVariables.timer_city_capture!=NULL)
        delete GlobalServerData::serverPrivateVariables.timer_city_capture;
    GlobalServerData::serverPrivateVariables.timer_city_capture=new QTimer();
    GlobalServerData::serverPrivateVariables.timer_city_capture->setSingleShot(true);
    return load_next_city_capture();
}
#endif

void BaseServer::preload_the_skin()
{
    std::vector<std::string> skinFolderList=FacilityLibGeneral::skinIdList(GlobalServerData::serverSettings.datapack_basePath+DATAPACK_BASE_PATH_SKIN);
    int index=0;
    const int &listsize=skinFolderList.size();
    while(index<listsize)
    {
        GlobalServerData::serverPrivateVariables.skinList[skinFolderList.at(index)]=index;
        index++;
    }

    std::cout << listsize << " skin(s) loaded" << std::endl;
}

void BaseServer::preload_the_players()
{
    Client::simplifiedIdList.clear();
    Client::marketObjectIdList.reserve(GlobalServerData::serverSettings.max_players);
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
        case MapVisibilityAlgorithmSelection_WithBorder:
        if(GlobalServerData::serverPrivateVariables.timer_to_send_insert_move_remove!=NULL)
        {
            #ifndef EPOLLCATCHCHALLENGERSERVER
            GlobalServerData::serverPrivateVariables.timer_to_send_insert_move_remove->stop();
            GlobalServerData::serverPrivateVariables.timer_to_send_insert_move_remove->deleteLater();
            #else
            delete GlobalServerData::serverPrivateVariables.timer_to_send_insert_move_remove;
            #endif
        }
        #ifndef EPOLLCATCHCHALLENGERSERVER
        GlobalServerData::serverPrivateVariables.timer_to_send_insert_move_remove=new QTimer();
        #else
        GlobalServerData::serverPrivateVariables.timer_to_send_insert_move_remove=new TimerSendInsertMoveRemove();
        #endif
        GlobalServerData::serverPrivateVariables.timer_to_send_insert_move_remove->start(CATCHCHALLENGER_SERVER_MAP_TIME_TO_SEND_MOVEMENT);
        break;
        case MapVisibilityAlgorithmSelection_None:
        default:
        break;
    }

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
