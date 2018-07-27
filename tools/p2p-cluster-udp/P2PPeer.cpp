#include "P2PPeer.h"
#include "P2PServerUDP.h"
#include <cstring>
#include <iostream>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

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

P2PPeer::~P2PPeer()
{
}

void P2PPeer::sign(uint8_t *msg,const size_t &length)
{
    ed25519_sha512_sign(reinterpret_cast<const uint8_t *>(P2PServerUDP::p2pserver->getPublicKey()),
                          P2PServerUDP::p2pserver->get_privatekey(),
                          length,msg,msg+length);

    #ifdef CATCHCHALLENGER_EXTRACHECK
    {
        const int returnFirm = ed25519_sha512_verify(
            reinterpret_cast<const uint8_t *>(P2PServerUDP::p2pserver->getPublicKey()),//pub
            length,//length
            reinterpret_cast<const uint8_t *>(msg),//msg
            reinterpret_cast<const uint8_t *>(msg+length)//signature
        );
        if(returnFirm != 1)
        {
            std::cerr << "firm packet error, abort" << std::endl;
            abort();
            return;
        }
    }
    #endif
}

void P2PPeer::emitAck()
{
    const uint8_t ackNumber=0xFF;
    sendDataWithMessageType(&ackNumber,sizeof(ackNumber));
}

bool P2PPeer::discardBuffer(const uint64_t &ackNumber)
{
    if(ackNumber==local_sequence_number_validated)
    {
        if(!dataToSend.empty())
            dataToSend.erase(dataToSend.cbegin());

        //reemit the last packet
        if(!dataToSend.empty())
        {
            const std::string &data=dataToSend.front();
            const int returnVal=P2PServerUDP::p2pserver->write(data.data(), data.size(), si_other);
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
    incremented_sequence_number(remote_sequence_number);
}

void P2PPeer::incremented_sequence_number(uint64_t &number)
{
    if(number==0xFFFFFFFFFFFFFFFF)
        number=0;
    else
        number++;
}

bool P2PPeer::sendDataWithMessageType(const uint8_t * const data, const uint16_t &size)
{
    if(size>=(sizeof(P2PPeer::buffer)-8-8-1-ED25519_SIGNATURE_SIZE))
        return false;
    if(size==0)
        return false;
    const unsigned int dataSize=8+8+size+ED25519_SIGNATURE_SIZE;
    memcpy(P2PPeer::buffer,&local_sequence_number_validated,sizeof(local_sequence_number_validated));
    local_sequence_number_validated++;
    memcpy(P2PPeer::buffer+8,&remote_sequence_number,sizeof(remote_sequence_number));
    memcpy(P2PPeer::buffer+8+8,data,size);
    P2PPeer::sign(reinterpret_cast<uint8_t *>(P2PPeer::buffer),8+8+size);
    #ifdef CATCHCHALLENGER_EXTRACHECK
    {
        const int returnFirm = ed25519_sha512_verify(
            reinterpret_cast<const uint8_t *>(P2PServerUDP::p2pserver->getPublicKey()),//pub
            dataSize-ED25519_SIGNATURE_SIZE,//length
            reinterpret_cast<const uint8_t *>(P2PPeer::buffer),//msg
            reinterpret_cast<const uint8_t *>(P2PPeer::buffer+dataSize-ED25519_SIGNATURE_SIZE)//signature
        );
        if(returnFirm != 1)
        {
            std::cerr << "out packet with wrong signature, blocked" << std::endl;
            abort();
            return -1;
        }
        uint8_t messageType=0;
        memcpy(&messageType,data,1);
        if(messageType!=0xFF && (messageType<0x01 || messageType>0x04))
        {
            std::cerr << "messageType wrong (" << std::to_string(messageType) << "), blocked" << std::endl;
            abort();
            return -1;
        }
    }
    #endif

    dataToSend.push_back(std::string(P2PPeer::buffer+8+8+size+ED25519_SIGNATURE_SIZE));

    const int returnVal=P2PServerUDP::p2pserver->write(P2PPeer::buffer, dataSize, si_other);
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

bool P2PPeer::sendData(const uint8_t * const data, const uint16_t &size)
{
    if(size>=(sizeof(P2PPeer::buffer)-8-8-1-ED25519_SIGNATURE_SIZE))
        return false;
    if(size==0)
        return false;
    const unsigned int dataSize=8+8+1+size+ED25519_SIGNATURE_SIZE;
    memcpy(P2PPeer::buffer,&local_sequence_number_validated,sizeof(local_sequence_number_validated));
    local_sequence_number_validated++;
    memcpy(P2PPeer::buffer+8,&remote_sequence_number,sizeof(remote_sequence_number));
    const uint8_t messageType=0xFF;
    memcpy(P2PPeer::buffer+8+8,&messageType,size);
    memcpy(P2PPeer::buffer+8+8+1,data,size);
    P2PPeer::sign(reinterpret_cast<uint8_t *>(P2PPeer::buffer),8+8+1+size);
    #ifdef CATCHCHALLENGER_EXTRACHECK
    {
        const int returnFirm = ed25519_sha512_verify(
            reinterpret_cast<const uint8_t *>(P2PServerUDP::p2pserver->getPublicKey()),//pub
            dataSize-ED25519_SIGNATURE_SIZE,//length
            reinterpret_cast<const uint8_t *>(P2PPeer::buffer),//msg
            reinterpret_cast<const uint8_t *>(P2PPeer::buffer+dataSize-ED25519_SIGNATURE_SIZE)//signature
        );
        if(returnFirm != 1)
        {
            std::cerr << "out packet with wrong signature, blocked" << std::endl;
            abort();
            return -1;
        }
        uint8_t messageType=0;
        memcpy(&messageType,P2PPeer::buffer+8+8,1);
        if(messageType!=0xFF && (messageType<0x01 || messageType>0x04))
        {
            std::cerr << "messageType wrong (" << std::to_string(messageType) << "), blocked" << std::endl;
            abort();
            return -1;
        }
    }
    #endif

    dataToSend.push_back(std::string(P2PPeer::buffer+8+8+size+ED25519_SIGNATURE_SIZE));

    const int returnVal=P2PServerUDP::p2pserver->write(P2PPeer::buffer, dataSize, si_other);
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

bool P2PPeer::sendRawDataWithoutPutInQueue(const uint8_t * const data, const uint16_t &size)
{
    if(size>=(sizeof(P2PPeer::buffer)-8-8-1-ED25519_SIGNATURE_SIZE))
        return false;
    if(size==0)
        return false;
    #ifdef CATCHCHALLENGER_EXTRACHECK
    {
        const int returnFirm = ed25519_sha512_verify(
            reinterpret_cast<const uint8_t *>(P2PServerUDP::p2pserver->getPublicKey()),//pub
            size-ED25519_SIGNATURE_SIZE,//length
            reinterpret_cast<const uint8_t *>(data),//msg
            reinterpret_cast<const uint8_t *>(data+size-ED25519_SIGNATURE_SIZE)//signature
        );
        if(returnFirm != 1)
        {
            std::cerr << "out packet with wrong signature, blocked" << std::endl;
            abort();
            return -1;
        }
        uint8_t messageType=0;
        memcpy(&messageType,data+8+8,1);
        if(messageType!=0xFF && (messageType<0x01 || messageType>0x04))
        {
            std::cerr << "messageType wrong (" << std::to_string(messageType) << "), blocked" << std::endl;
            abort();
            return -1;
        }
    }
    #endif

    const int returnVal=P2PServerUDP::p2pserver->write(reinterpret_cast<const char * const>(data), size, si_other);
    if (returnVal < 0)
    {
        std::cerr << "P2PServerUDP::parseIncommingData(): sendto() problem" << std::endl;
        return false;
    }
    else if ((uint32_t)returnVal != size)
    {
        std::cerr << "P2PServerUDP::parseIncommingData(): sendto() problem dataSize" << std::endl;
        return false;
    }
    if (size > 1200)
    {
        return false;
        std::cerr << "P2PServerUDP::parseIncommingData(): sendto() problem dataSize (2)" << std::endl;
    }
    return true;
}

std::string P2PPeer::toString() const
{
    sockaddr_in socket;
    memcpy(&socket,&si_other,sizeof(socket));
    return std::string(inet_ntoa(socket.sin_addr))+":"+std::to_string(ntohs(si_other.sin_port));
}
