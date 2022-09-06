#include "TimerDetectTimeout.hpp"
#include "EpollClientLoginSlave.hpp"
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

    if(CatchChallenger::EpollClientLoginSlave::client_list.size()>500)
    {
        std::unordered_map<CatchChallenger::EpollClientLoginSlave::EpollClientLoginStat,uint64_t> countByStat;
        std::unordered_map<CatchChallenger::LinkToGameServer::Stat,uint64_t> countLinkByStat;
        //std cout the number of client by stats
        unsigned int index=0;
        while(index<CatchChallenger::EpollClientLoginSlave::client_list.size())
        {
            CatchChallenger::EpollClientLoginSlave * client=CatchChallenger::EpollClientLoginSlave::client_list.at(index);
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
        std::cout << "Overload: ";
        index=0;
        for( const std::pair<CatchChallenger::EpollClientLoginSlave::EpollClientLoginStat,uint64_t> n : countByStat )
        {
            if(index>0)
                std::cout << ", ";
            switch(n.first)
            {
            case CatchChallenger::EpollClientLoginSlave::EpollClientLoginStat::CharacterSelected:
                std::cout << "CharacterSelected";
                break;
            case CatchChallenger::EpollClientLoginSlave::EpollClientLoginStat::CharacterSelecting:
                std::cout << "CharacterSelecting";
                break;
            case CatchChallenger::EpollClientLoginSlave::EpollClientLoginStat::GameServerConnected:
                std::cout << "GameServerConnected";
                break;
            case CatchChallenger::EpollClientLoginSlave::EpollClientLoginStat::GameServerConnecting:
                std::cout << "GameServerConnecting";
                break;
            case CatchChallenger::EpollClientLoginSlave::EpollClientLoginStat::Logged:
                std::cout << "Logged";
                break;
            case CatchChallenger::EpollClientLoginSlave::EpollClientLoginStat::LoggedStatClient:
                std::cout << "LoggedStatClient";
                break;
            case CatchChallenger::EpollClientLoginSlave::EpollClientLoginStat::LoginInProgress:
                std::cout << "LoginInProgress";
                break;
            case CatchChallenger::EpollClientLoginSlave::EpollClientLoginStat::None:
                std::cout << "None";
                break;
            case CatchChallenger::EpollClientLoginSlave::EpollClientLoginStat::ProtocolGood:
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
    while(index<CatchChallenger::EpollClientLoginSlave::client_list.size())
    {
        CatchChallenger::EpollClientLoginSlave * client=CatchChallenger::EpollClientLoginSlave::client_list.at(index);
        if(client->stat==CatchChallenger::EpollClientLoginSlave::GameServerConnected)
        {
            if((client->get_lastActivity()+10*60*1000)<timeCurrent)
                client->disconnectClient();
        }
        else
        {
            if((client->get_lastActivity()+1*60*1000)<timeCurrent)
                client->disconnectClient();
        }
        if(client->linkToGameServer!=nullptr)
        {
            if(client->linkToGameServer->stat==CatchChallenger::LinkToGameServer::Logged)
            {
                if((client->linkToGameServer->get_lastActivity()+10*60*1000)<timeCurrent)
                    client->linkToGameServer->disconnectClient();
            }
            else
            {
                if((client->linkToGameServer->get_lastActivity()+1*60*1000)<timeCurrent)
                    client->linkToGameServer->disconnectClient();
            }
        }
        index++;
    }


    if(CatchChallenger::EpollClientLoginSlave::stat_client_list.size()>10)
    {
        std::unordered_map<CatchChallenger::EpollClientLoginSlave::EpollClientLoginStat,uint64_t> countByStat;
        std::unordered_map<CatchChallenger::LinkToGameServer::Stat,uint64_t> countLinkByStat;
        //std cout the number of client by stats
        unsigned int index=0;
        while(index<CatchChallenger::EpollClientLoginSlave::stat_client_list.size())
        {
            CatchChallenger::EpollClientLoginSlave * client=CatchChallenger::EpollClientLoginSlave::stat_client_list.at(index);
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
        std::cout << "Overload: ";
        index=0;
        for( const std::pair<CatchChallenger::EpollClientLoginSlave::EpollClientLoginStat,uint64_t> n : countByStat )
        {
            if(index>0)
                std::cout << ", ";
            switch(n.first)
            {
            case CatchChallenger::EpollClientLoginSlave::EpollClientLoginStat::CharacterSelected:
                std::cout << "CharacterSelected";
                break;
            case CatchChallenger::EpollClientLoginSlave::EpollClientLoginStat::CharacterSelecting:
                std::cout << "CharacterSelecting";
                break;
            case CatchChallenger::EpollClientLoginSlave::EpollClientLoginStat::GameServerConnected:
                std::cout << "GameServerConnected";
                break;
            case CatchChallenger::EpollClientLoginSlave::EpollClientLoginStat::GameServerConnecting:
                std::cout << "GameServerConnecting";
                break;
            case CatchChallenger::EpollClientLoginSlave::EpollClientLoginStat::Logged:
                std::cout << "Logged";
                break;
            case CatchChallenger::EpollClientLoginSlave::EpollClientLoginStat::LoggedStatClient:
                std::cout << "LoggedStatClient";
                break;
            case CatchChallenger::EpollClientLoginSlave::EpollClientLoginStat::LoginInProgress:
                std::cout << "LoginInProgress";
                break;
            case CatchChallenger::EpollClientLoginSlave::EpollClientLoginStat::None:
                std::cout << "None";
                break;
            case CatchChallenger::EpollClientLoginSlave::EpollClientLoginStat::ProtocolGood:
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
    index=0;
    while(index<CatchChallenger::EpollClientLoginSlave::stat_client_list.size())
    {
        CatchChallenger::EpollClientLoginSlave * client=CatchChallenger::EpollClientLoginSlave::stat_client_list.at(index);
        if(client->stat==CatchChallenger::EpollClientLoginSlave::GameServerConnected)
        {
            if((client->get_lastActivity()+10*60*1000)<timeCurrent)
                client->disconnectClient();
        }
        else
        {
            if((client->get_lastActivity()+1*60*1000)<timeCurrent)
                client->disconnectClient();
        }
        if(client->linkToGameServer!=nullptr)
        {
            if(client->linkToGameServer->stat==CatchChallenger::LinkToGameServer::Logged)
            {
                if((client->linkToGameServer->get_lastActivity()+10*60*1000)<timeCurrent)
                    client->linkToGameServer->disconnectClient();
            }
            else
            {
                if((client->linkToGameServer->get_lastActivity()+1*60*1000)<timeCurrent)
                    client->linkToGameServer->disconnectClient();
            }
        }
        index++;
    }
}
