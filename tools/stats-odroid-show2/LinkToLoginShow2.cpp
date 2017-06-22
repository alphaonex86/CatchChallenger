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
}

void LinkToLoginShow2::tryReconnect()
{
    LinkToLoginShow2::writeData("\ec\e[2s\e[1r\e[33m");
    writeData("Try reconnect...");
    LinkToLogin::tryReconnect();
}

void LinkToLoginShow2::updateJsonFile()
{
    LinkToLoginShow2::writeData("\ec\e[2s\e[1r\e[37m");

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
    LinkToLogin::writeData("Number of server: \e[32m"+std::to_string(serverList.size())+"\e[37m\n\r");
    LinkToLogin::writeData("\n\r");
    LinkToLogin::writeData("Players: \e[32m"+std::to_string(connectedPlayer)+"\e[37m\n\r");
    LinkToLogin::writeData("Max players: \e[33m"+std::to_string(maxPlayer)+"\e[37m\n\r");
    LinkToLogin::writeData("\n\r");
    LinkToLogin::writeData("CatchChallenger ----------\n\rServices: \e[32mUP\e[37m\n\r");
}
