#include "P2PServerUDPSettings.h"
#include "../../general/base/cpp11addition.h"

#include <arpa/inet.h>
#include <iostream>

P2PServerUDPSettings::P2PServerUDPSettings(uint8_t *privatekey, uint8_t *ca_publickey, uint8_t *ca_signature) :
    settings("p2p.xml"),
    port(0)
{
    if(!settings.contains("privatekey"))
    {
        genPrivateKey(privatekey);
        settings.setValue("privatekey",binarytoHexa(privatekey,sizeof(privatekey)));
    }
    else
    {
        const std::vector<char> tempData=hexatoBinary(settings.value("privatekey"));
        if(tempData.size()==ED25519_KEY_SIZE)
            memcpy(privatekey,tempData.data(),ED25519_KEY_SIZE);
        else
        {
            genPrivateKey(privatekey);
            settings.setValue("privatekey",binarytoHexa(privatekey,sizeof(privatekey)));
            settings.sync();
        }
    }
    if(!settings.contains("ca_signature"))
    {
        std::cerr << "You need define CA signature of your public key" << std::endl;
        settings.setValue("ca_signature","");
        settings.sync();
        abort();
    }
    else
    {
        const std::vector<char> tempData=hexatoBinary(settings.value("ca_signature"));
        if(tempData.size()==ED25519_SIGNATURE_SIZE)
            memcpy(ca_signature,tempData.data(),ED25519_SIGNATURE_SIZE);
        else
        {
            std::cerr << "You need define CA signature of your public key " <<
                         std::to_string(tempData.size()) << "!=" << std::to_string(ED25519_SIGNATURE_SIZE) << std::endl;
            abort();
        }
    }
    if(!settings.contains("ca_publickey"))
    {
        std::cerr << "You need define CA public key" << std::endl;
        settings.setValue("ca_publickey","");
        settings.sync();
        abort();
    }
    else
    {
        const std::vector<char> tempData=hexatoBinary(settings.value("ca_publickey"));
        if(tempData.size()==ED25519_KEY_SIZE)
            memcpy(ca_publickey,tempData.data(),ED25519_KEY_SIZE);
        else
        {
            std::cerr << "You need define CA public key " <<
                         std::to_string(tempData.size()) << "!=" << std::to_string(ED25519_KEY_SIZE) << std::endl;
            abort();
        }
    }

    if(!settings.contains("known_node"))
        settings.setValue("known_node","");

    if(!settings.contains("port"))
        settings.setValue("port",rand()%40000+10000);
    port=stringtouint16(settings.value("port"));

    const std::vector<std::string> &known_nodes=stringsplit(settings.value("known_node"),',');
    if((known_nodes.size()%2)!=0)
    {
        std::cerr << "(known_nodes.size()%2)!=0" << std::endl;
        abort();
    }
    size_t index=0;
    while(index<known_nodes.size())
    {
        bool ok=false;

        P2PServerUDP::HostToConnect hostToConnect;
        const std::string host=known_nodes.at(index);
        const std::string &portstring=known_nodes.at(index+1);
        const uint16_t port=stringtouint16(portstring,&ok);
        if(ok)
        {
            sockaddr_in serv_addr;
            memset(&serv_addr, 0, sizeof(serv_addr));
            serv_addr.sin_family = AF_INET;
            serv_addr.sin_port = htobe16(port);
            const char * const hostC=host.c_str();
            int convertResult=inet_pton(AF_INET6,hostC,&serv_addr.sin_addr);
            if(convertResult!=1)
            {
                convertResult=inet_pton(AF_INET,hostC,&serv_addr.sin_addr);
                if(convertResult!=1)
                    std::cerr << "not IPv4 and IPv6 address, host: \"" << host << "\", portstring: \"" << portstring
                              << "\", errno: " << std::to_string(errno) << std::endl;
            }
            if(convertResult==1)
            {
                hostToConnect.round=0;
                hostToConnect.serv_addr=serv_addr;
                hostToConnect.serialised_serv_addr=P2PServerUDP::sockSerialised(serv_addr);

                const std::string &serialisedPeer=std::string(inet_ntoa(serv_addr.sin_addr))+","+std::to_string(ntohs(serv_addr.sin_port));
                knownPeers.insert(serialisedPeer);

                //pass to temp list because P2PServerUDP::p2pserver not init for now
                hostToConnectTemp.push_back(hostToConnect);
            }
        }

        index+=2;
    }

    settings.sync();

    setKey(privatekey,ca_publickey,ca_signature);
}

void P2PServerUDPSettings::genPrivateKey(uint8_t *privatekey)
{
    FILE *ptr = fopen("/dev/random","rb");  // r for read, b for binary
    if(ptr == NULL)
        abort();
    const int readSize=fread(privatekey,1,sizeof(privatekey),ptr);
    if(readSize != sizeof(privatekey))
        abort();
    fclose(ptr);
}

bool P2PServerUDPSettings::newPeer(const sockaddr_in &si_other)
{
    sockaddr_in socket;
    memcpy(&socket,&si_other,sizeof(socket));
    const std::string &serialisedPeer=std::string(inet_ntoa(socket.sin_addr))+","+std::to_string(ntohs(si_other.sin_port));

    P2PServerUDP::sockSerialised(si_other);
    if(knownPeers.find(serialisedPeer)!=knownPeers.cend())
        return false;
    knownPeers.insert(serialisedPeer);

    std::string peersList;
    for ( const auto &n : knownPeers )
    {
        if(!peersList.empty())
            peersList+=",";
        peersList+=n;
    }
    settings.setValue("known_node",peersList);
    settings.sync();
    return true;
}
