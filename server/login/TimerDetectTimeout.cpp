#include "TimerDetectTimeout.hpp"
#include <iostream>
#include "EventLoopClientLoginSlave.hpp"
#include "LinkToMaster.hpp"
#include "LinkToGameServer.hpp"

TimerDetectTimeout::TimerDetectTimeout()
{
    //todo start it
}

void TimerDetectTimeout::exec()
{
    CatchChallenger::LinkToMaster::linkToMaster->detectTimeout();
    const uint64_t timeCurrent=CatchChallenger::LinkToGameServer::msFrom1970();

    if(CatchChallenger::EventLoopClientLoginSlave::client_list.size()>500)
    {
        std::unordered_map<CatchChallenger::EventLoopClientLoginSlave::EventLoopClientLoginStat,uint64_t> countByStat;
        std::unordered_map<CatchChallenger::LinkToGameServer::Stat,uint64_t> countLinkByStat;
        //std cout the number of client by stats
        unsigned int index=0;
        while(index<CatchChallenger::EventLoopClientLoginSlave::client_list.size())
        {
            CatchChallenger::EventLoopClientLoginSlave * client=CatchChallenger::EventLoopClientLoginSlave::client_list.at(index);
            if(countByStat.find(client->stat)!=countByStat.cend())
                countByStat[client->stat]++;
            else
                countByStat[client->stat]=1;

            if(client->linkToGameServer!=nullptr)
            {
                if(countLinkByStat.find(client->linkToGameServer->stat)!=countLinkByStat.cend())
                    countLinkByStat[client->linkToGameServer->stat]++;
                else
                    countLinkByStat[client->linkToGameServer->stat]=1;
            }
            index++;
        }
        std::cout << "Overload 1): ";
        index=0;
        for( const std::pair<CatchChallenger::EventLoopClientLoginSlave::EventLoopClientLoginStat,uint64_t> n : countByStat )
        {
            if(index>0)
                std::cout << ", ";
            switch(n.first)
            {
            case CatchChallenger::EventLoopClientLoginSlave::EventLoopClientLoginStat::CharacterSelected:
                std::cout << "CharacterSelected";
                break;
            case CatchChallenger::EventLoopClientLoginSlave::EventLoopClientLoginStat::CharacterSelecting:
                std::cout << "CharacterSelecting";
                break;
            case CatchChallenger::EventLoopClientLoginSlave::EventLoopClientLoginStat::GameServerConnected:
                std::cout << "GameServerConnected";
                break;
            case CatchChallenger::EventLoopClientLoginSlave::EventLoopClientLoginStat::GameServerConnecting:
                std::cout << "GameServerConnecting";
                break;
            case CatchChallenger::EventLoopClientLoginSlave::EventLoopClientLoginStat::Logged:
                std::cout << "Logged";
                break;
            case CatchChallenger::EventLoopClientLoginSlave::EventLoopClientLoginStat::LoggedStatClient:
                std::cout << "LoggedStatClient ERROR";
                break;
            case CatchChallenger::EventLoopClientLoginSlave::EventLoopClientLoginStat::LoginInProgress:
                std::cout << "LoginInProgress";
                break;
            case CatchChallenger::EventLoopClientLoginSlave::EventLoopClientLoginStat::None:
                std::cout << "None";
                break;
            case CatchChallenger::EventLoopClientLoginSlave::EventLoopClientLoginStat::ProtocolGood:
                std::cout << "ProtocolGood";
                break;
            default:
                std::cout << "stat " << std::to_string(n.first);
                break;
            }
            std::cout << ": " << std::to_string(n.second);
            index++;
        }
        for( const std::pair<CatchChallenger::LinkToGameServer::Stat,uint64_t> n : countLinkByStat )
        {
            if(index>0)
                std::cout << ", ";
            std::cout << "Link ";
            switch(n.first)
            {
            case CatchChallenger::LinkToGameServer::Stat::Connected:
                std::cout << "Connected";
                break;
            case CatchChallenger::LinkToGameServer::Stat::Connecting:
                std::cout << "Connecting";
                break;
            case CatchChallenger::LinkToGameServer::Stat::Logged:
                std::cout << "Logged";
                break;
            case CatchChallenger::LinkToGameServer::Stat::LoginInProgress:
                std::cout << "LoginInProgress";
                break;
            case CatchChallenger::LinkToGameServer::Stat::Unconnected:
                std::cout << "Unconnected";
                break;
            case CatchChallenger::LinkToGameServer::Stat::ProtocolGood:
                std::cout << "ProtocolGood";
                break;
            default:
                std::cout << "stat " << std::to_string(n.first);
                break;
            }
            std::cout << ": " << std::to_string(n.second);
            index++;
        }
        std::cout << std::endl;
    }
    unsigned int index=0;
    while(index<CatchChallenger::EventLoopClientLoginSlave::client_list.size())
    {
        CatchChallenger::EventLoopClientLoginSlave * client=CatchChallenger::EventLoopClientLoginSlave::client_list.at(index);
        if(client->stat==CatchChallenger::EventLoopClientLoginSlave::GameServerConnected)
        {
            if((client->get_lastActivity()+10*60*1000)<timeCurrent)
            {
                std::cerr << client << " (" << std::to_string(client->stat) << ") disconnect due to last Activity: " << client->get_lastActivity() << " (" << timeCurrent << ") " << __FILE__ << ":" << __LINE__ << std::endl;
                client->disconnectClient();
            }
        }
        else
        {
            if((client->get_lastActivity()+1*60*1000)<timeCurrent)
            {
                std::cerr << client << " (" << std::to_string(client->stat) << ") disconnect due to last Activity: " << client->get_lastActivity() << " (" << timeCurrent << ") " << __FILE__ << ":" << __LINE__ << std::endl;
                client->disconnectClient();
            }
        }
        if(client->linkToGameServer!=nullptr)
        {
            if(client->linkToGameServer->stat==CatchChallenger::LinkToGameServer::Logged)
            {
                if((client->linkToGameServer->get_lastActivity()+10*60*1000)<timeCurrent)
                {
                    std::cerr << client << " (" << std::to_string(client->stat) << ") disconnect due to last Activity: " << client->get_lastActivity() << " (" << timeCurrent << ") " << __FILE__ << ":" << __LINE__ << std::endl;
                    client->linkToGameServer->disconnectClient();
                }
            }
            else
            {
                if((client->linkToGameServer->get_lastActivity()+1*60*1000)<timeCurrent)
                {
                    std::cerr << client << " (" << std::to_string(client->stat) << ") disconnect due to last Activity: " << client->get_lastActivity() << " (" << timeCurrent << ") " << __FILE__ << ":" << __LINE__ << std::endl;
                    client->linkToGameServer->disconnectClient();
                }
            }
        }
        index++;
    }


    if(CatchChallenger::EventLoopClientLoginSlave::stat_client_list.size()>10)
    {
        std::unordered_map<CatchChallenger::EventLoopClientLoginSlave::EventLoopClientLoginStat,uint64_t> countByStat;
        std::unordered_map<CatchChallenger::LinkToGameServer::Stat,uint64_t> countLinkByStat;
        //std cout the number of client by stats
        unsigned int index=0;
        while(index<CatchChallenger::EventLoopClientLoginSlave::stat_client_list.size())
        {
            CatchChallenger::EventLoopClientLoginSlave * client=CatchChallenger::EventLoopClientLoginSlave::stat_client_list.at(index);
            if(countByStat.find(client->stat)!=countByStat.cend())
                countByStat[client->stat]++;
            else
                countByStat[client->stat]=1;

            if(client->linkToGameServer!=nullptr)
            {
                if(countLinkByStat.find(client->linkToGameServer->stat)!=countLinkByStat.cend())
                    countLinkByStat[client->linkToGameServer->stat]++;
                else
                    countLinkByStat[client->linkToGameServer->stat]=1;
            }
            index++;
        }
        std::cout << "Overload 2): ";
        index=0;
        for( const std::pair<CatchChallenger::EventLoopClientLoginSlave::EventLoopClientLoginStat,uint64_t> n : countByStat )
        {
            if(index>0)
                std::cout << ", ";
            switch(n.first)
            {
            case CatchChallenger::EventLoopClientLoginSlave::EventLoopClientLoginStat::CharacterSelected:
                std::cout << "CharacterSelected ERROR";
                break;
            case CatchChallenger::EventLoopClientLoginSlave::EventLoopClientLoginStat::CharacterSelecting:
                std::cout << "CharacterSelecting ERROR";
                break;
            case CatchChallenger::EventLoopClientLoginSlave::EventLoopClientLoginStat::GameServerConnected:
                std::cout << "GameServerConnected ERROR";
                break;
            case CatchChallenger::EventLoopClientLoginSlave::EventLoopClientLoginStat::GameServerConnecting:
                std::cout << "GameServerConnecting ERROR";
                break;
            case CatchChallenger::EventLoopClientLoginSlave::EventLoopClientLoginStat::Logged:
                std::cout << "Logged ERROR";
                break;
            case CatchChallenger::EventLoopClientLoginSlave::EventLoopClientLoginStat::LoggedStatClient:
                std::cout << "LoggedStatClient";
                break;
            case CatchChallenger::EventLoopClientLoginSlave::EventLoopClientLoginStat::LoginInProgress:
                std::cout << "LoginInProgress ERROR";
                break;
            case CatchChallenger::EventLoopClientLoginSlave::EventLoopClientLoginStat::None:
                std::cout << "None ERROR";
                break;
            case CatchChallenger::EventLoopClientLoginSlave::EventLoopClientLoginStat::ProtocolGood:
                std::cout << "ProtocolGood ERROR";
                break;
            default:
                std::cout << "stat " << std::to_string(n.first) << " ERROR";
                break;
            }
            std::cout << ": " << std::to_string(n.second);
            index++;
        }
        for( const std::pair<CatchChallenger::LinkToGameServer::Stat,uint64_t> n : countLinkByStat )
        {
            if(index>0)
                std::cout << ", ";
            std::cout << "Link ";
            switch(n.first)
            {
            case CatchChallenger::LinkToGameServer::Stat::Connected:
                std::cout << "Connected";
                break;
            case CatchChallenger::LinkToGameServer::Stat::Connecting:
                std::cout << "Connecting";
                break;
            case CatchChallenger::LinkToGameServer::Stat::Logged:
                std::cout << "Logged";
                break;
            case CatchChallenger::LinkToGameServer::Stat::LoginInProgress:
                std::cout << "LoginInProgress";
                break;
            case CatchChallenger::LinkToGameServer::Stat::Unconnected:
                std::cout << "Unconnected";
                break;
            case CatchChallenger::LinkToGameServer::Stat::ProtocolGood:
                std::cout << "ProtocolGood";
                break;
            default:
                std::cout << "stat " << std::to_string(n.first);
                break;
            }
            std::cout << ": " << std::to_string(n.second);
            index++;
        }
        std::cout << std::endl;
    }
    index=0;
    while(index<CatchChallenger::EventLoopClientLoginSlave::stat_client_list.size())
    {
        CatchChallenger::EventLoopClientLoginSlave * client=CatchChallenger::EventLoopClientLoginSlave::stat_client_list.at(index);
        if(client->stat==CatchChallenger::EventLoopClientLoginSlave::GameServerConnected)
        {
            if((client->get_lastActivity()+10*60*1000)<timeCurrent)
            {
                std::cerr << client << " (" << std::to_string(client->stat) << ") disconnect due to last Activity: " << client->get_lastActivity() << " (" << timeCurrent << ") " << __FILE__ << ":" << __LINE__ << std::endl;
                client->disconnectClient();
            }
        }
        else
        {
/*            if((client->get_lastActivity()+1*60*1000)<timeCurrent)
            {
                std::cerr << client << " (" << std::to_string(client->stat) << ") disconnect due to last Activity: " << client->get_lastActivity() << " (" << timeCurrent << ") " << __FILE__ << ":" << __LINE__ << std::endl;
                client->disconnectClient();
            }*/
        }
        if(client->linkToGameServer!=nullptr)
        {
            if(client->linkToGameServer->stat==CatchChallenger::LinkToGameServer::Logged)
            {
                if((client->linkToGameServer->get_lastActivity()+10*60*1000)<timeCurrent)
                {
                    std::cerr << client << " (" << std::to_string(client->stat) << ") disconnect due to last Activity: " << client->get_lastActivity() << " (" << timeCurrent << ") " << __FILE__ << ":" << __LINE__ << std::endl;
                    client->linkToGameServer->disconnectClient();
                }
            }
            else
            {
                if((client->linkToGameServer->get_lastActivity()+1*60*1000)<timeCurrent)
                {
                    std::cerr << client << " (" << std::to_string(client->stat) << ") disconnect due to last Activity: " << client->get_lastActivity() << " (" << timeCurrent << ") " << __FILE__ << ":" << __LINE__ << std::endl;
                    client->linkToGameServer->disconnectClient();
                }
            }
        }
        index++;
    }
}
