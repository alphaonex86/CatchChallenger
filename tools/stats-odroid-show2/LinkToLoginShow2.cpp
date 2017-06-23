#include "LinkToLoginShow2.h"

LinkToLoginShow2 *LinkToLoginShow2::linkToLogin=NULL;

LinkToLoginShow2::LinkToLoginShow2(
        #ifdef SERVERSSL
            const int &infd, SSL_CTX *ctx
        #else
            const int &infd
        #endif
            ) :
    #ifdef SERVERSSL
        LinkToLogin(infd,ctx)
    #else
        LinkToLogin(infd)
    #endif
{
    displayedServerNumber=0;
    displayedPlayer=0;
    displayedMaxPlayer=0;
    displayedMaxServer=0;
}

void LinkToLoginShow2::tryReconnect()
{
    LinkToLogin::writeData("\ec\e[2s\e[1r\e[37m");
    LinkToLogin::writeData("Servers: \e[31m?\e[37m/\e[32m"+std::to_string(displayedMaxServer)+"\e[37m\n\r");
    LinkToLogin::writeData("\n\r");
    LinkToLogin::writeData("Players: \e[32m?\e[37m\n\r");
    LinkToLogin::writeData("Max players: \e[33m"+std::to_string(displayedMaxPlayer)+"\e[37m\n\r");
    LinkToLogin::writeData("\n\r");
    LinkToLogin::writeData("CatchChallenger ----------\n\r");
    LinkToLogin::writeData("Services: \e[33mTry reconnect...\e[37m\n\r");

    LinkToLogin::tryReconnect();
    displayedServerNumber=0;
    displayedPlayer=0;
    displayedMaxPlayer=0;
}

void LinkToLoginShow2::updateJsonFile()
{
    unsigned int connectedPlayer=0;
    unsigned int maxPlayer=0;
    unsigned int index=0;
    while(index<serverList.size())
    {
        const ServerFromPoolForDisplay &server=serverList.at(index);
        if(server.maxPlayer<65535)
        {
            connectedPlayer+=server.currentPlayer;
            maxPlayer+=server.maxPlayer;
        }
        index++;
    }
    if(displayedServerNumber==serverList.size() &&
        displayedPlayer==connectedPlayer &&
        displayedMaxPlayer==maxPlayer)
        return;
    displayedServerNumber=serverList.size();
    displayedPlayer=connectedPlayer;
    displayedMaxPlayer=maxPlayer;
    if(displayedMaxServer<serverList.size())
        displayedMaxServer=serverList.size();
    LinkToLoginShow2::writeData("\ec\e[2s\e[1r\e[37m");
    if(serverList.size()<displayedMaxServer)
        LinkToLogin::writeData("Servers: \e[31m"+std::to_string(serverList.size())+"\e[37m/\e[32m"+std::to_string(displayedMaxServer)+"\e[37m\n\r");
    else
        LinkToLogin::writeData("Servers: \e[32m"+std::to_string(serverList.size())+"\e[37m/\e[32m"+std::to_string(displayedMaxServer)+"\e[37m\n\r");
    LinkToLogin::writeData("\n\r");
    LinkToLogin::writeData("Players: \e[32m"+std::to_string(connectedPlayer)+"\e[37m\n\r");
    LinkToLogin::writeData("Max players: \e[33m"+std::to_string(maxPlayer)+"\e[37m\n\r");
    LinkToLogin::writeData("\n\r");
    LinkToLogin::writeData("CatchChallenger ----------\n\r");
    LinkToLogin::writeData("Services: \e[32mUP\e[37m\n\r");
}
