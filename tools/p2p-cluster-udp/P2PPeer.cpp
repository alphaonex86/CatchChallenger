#include "P2PPeer.h"
#include "P2PServerUDP.h"
#include <cstring>
#include <iostream>

//[8(current sequence number)+8(acknowledgement number)+1(request type)+ED25519_SIGNATURE_SIZE(node)]
char P2PPeer::buffer[];

P2PPeer::P2PPeer(const uint8_t * const publickey, const uint64_t &local_sequence_number,
                 const uint64_t &remote_sequence_number, const sockaddr_in &si_other) :
    local_sequence_number_validated(local_sequence_number),
    remote_sequence_number(remote_sequence_number),
    si_other(si_other)
{
    memcpy(this->publickey,publickey,sizeof(this->publickey));
    /*memset(P2PPeer::ack,0,sizeof(P2PPeer::ack));

    memcpy(P2PPeer::ack+8+8,ackNumber,sizeof(ackNumber));*/
}

void P2PPeer::sign(uint8_t *msg,const size_t &length)
{
    ::ed25519_sha512_sign(CatchChallenger::P2PServerUDP::p2pserver->get_ca_publickey(),
                          CatchChallenger::P2PServerUDP::p2pserver->get_privatekey(),
                          length,msg,msg+length);
}

void P2PPeer::emitAck()
{
    const uint8_t ackNumber=0xFF;
    sendData(&ackNumber,sizeof(ackNumber));
}

bool P2PPeer::discardBuffer(const uint64_t &ackNumber)
{
    if(ackNumber==local_sequence_number_validated)
    {
        if(!dataToSend.empty())
            dataToSend.erase(dataToSend.cbegin());

        //reemit the last packet
        const std::string &data=dataToSend.front();
        const int returnVal=CatchChallenger::P2PServerUDP::p2pserver->write(data.data(), data.size(), si_other);
        if (returnVal < 0)
        {
            std::cerr << "P2PServerUDP::parseIncommingData(): sendto() problem" << std::endl;
            return false;
        }
        else if ((uint32_t)returnVal != data.size())
        {
            std::cerr << "P2PServerUDP::parseIncommingData(): sendto() problem dataSize" << std::endl;
            return false;
        }
        if (data.size() > 1200)
        {
            return false;
            std::cerr << "P2PServerUDP::parseIncommingData(): sendto() problem dataSize (2)" << std::endl;
        }
        return true;
    }
    else
    {
        std::cerr << "P2PServerUDP::discardBuffer(): problem" << std::endl;
        return true;
    }
}

const uint8_t *P2PPeer::getPublickey() const
{
    return publickey;
}

const uint64_t &P2PPeer::get_remote_sequence_number() const
{
    return remote_sequence_number;
}

void P2PPeer::incremente_remote_sequence_number()
{
    remote_sequence_number++;
}

bool P2PPeer::sendData(const uint8_t * const data, const uint16_t &size)
{
    if(size>=(sizeof(P2PPeer::buffer)-8-8-1-ED25519_SIGNATURE_SIZE))
        return false;
    if(size==0)
        return false;
    memcpy(P2PPeer::buffer,&local_sequence_number_validated,sizeof(local_sequence_number_validated));
    local_sequence_number_validated++;
    memcpy(P2PPeer::buffer+8,&remote_sequence_number,sizeof(remote_sequence_number));
    memcpy(P2PPeer::buffer+8+8,data,size);
    P2PPeer::sign(reinterpret_cast<uint8_t *>(P2PPeer::buffer),8+8+size);
    const unsigned int dataSize=8+8+size;

    dataToSend.push_back(std::string(P2PPeer::buffer+8+8+size+ED25519_SIGNATURE_SIZE));

    const int returnVal=CatchChallenger::P2PServerUDP::p2pserver->write(P2PPeer::buffer, dataSize, si_other);
    if (returnVal < 0)
    {
        std::cerr << "P2PServerUDP::parseIncommingData(): sendto() problem" << std::endl;
        return false;
    }
    else if ((uint32_t)returnVal != dataSize)
    {
        std::cerr << "P2PServerUDP::parseIncommingData(): sendto() problem dataSize" << std::endl;
        return false;
    }
    if (dataSize > 1200)
    {
        return false;
        std::cerr << "P2PServerUDP::parseIncommingData(): sendto() problem dataSize (2)" << std::endl;
    }
    return true;
}
