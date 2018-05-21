#if ! defined (ONLYMAPRENDER)
#include "ProtocolParsing.h"
#include "GeneralVariable.h"
#include "ProtocolParsingCheck.h"
#include "cpp11addition.h"
#include <iostream>
#include <cstring>

using namespace CatchChallenger;

void ProtocolParsingInputOutput::registerOutputQuery(const uint8_t &queryNumber,const uint8_t &packetCode)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(outputQueryNumberToPacketCode[queryNumber]!=0x00)
    {
        errorParsingLayer("Query with this query number already found");
        return;
    }
    //if not in query type done into ProtocolParsingCheck
    #endif
    //registrer on the check
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    protocolParsingCheck->registerOutputQuery(queryNumber,packetCode);
    #endif
    outputQueryNumberToPacketCode[queryNumber]=packetCode;
}


bool ProtocolParsingBase::internalPackOutcommingData(const char * const data,const int &size)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(size<=0)
    {
        std::cerr << "ProtocolParsingInputOutput::internalPackOutcommingData size is null: " << __LINE__ << std::endl;
        return false;
    }
    if(size>CATCHCHALLENGER_MAX_PACKET_SIZE)
    {
        std::cerr << "ProtocolParsingInputOutput::internalPackOutcommingData size is null: " << __LINE__ << std::endl;
        abort();
    }
    #endif
    #ifdef PROTOCOLPARSINGDEBUG
    messageParsingLayer("internalPackOutcommingData(): start");
    #endif
    #ifdef DEBUG_PROTOCOLPARSING_RAW_NETWORK
    std::cout << "Sended packet size: " << size << ": " << binarytoHexa(data,size) << std::endl;
    #endif // DEBUG_PROTOCOLPARSING_RAW_NETWORK

    if(size<=CATCHCHALLENGER_MAX_PACKET_SIZE)
    {
        const ssize_t &byteWriten = write(data,size);
        if(Q_UNLIKELY(size!=byteWriten))
        {
            disconnectClient();
            return false;
        }
        return true;
    }
    else
    {
        int cursor=0;
        ssize_t byteWriten=0;
        while(Q_LIKELY(cursor<size))
        {
            const int &remaining_size=size-cursor;
            int size_to_send;
            if(remaining_size<CATCHCHALLENGER_MAX_PACKET_SIZE)
                size_to_send=remaining_size;
            else
                size_to_send=CATCHCHALLENGER_MAX_PACKET_SIZE;
            byteWriten = write(data+cursor,size_to_send);
            if(Q_UNLIKELY(size_to_send!=byteWriten))
            {
                disconnectClient();
                return false;
            }
            cursor+=size_to_send;
        }
        return true;
    }
    return true;
}

//no control to be more fast
bool ProtocolParsingBase::internalSendRawSmallPacket(const char * const data,const int &size)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(size<=0)
    {
        std::cerr << "ProtocolParsingInputOutput::internalSendRawSmallPacket size is null " << __FILE__ << ":" << __LINE__ << std::endl;
        abort();
        return false;
    }
    #endif
    #ifdef PROTOCOLPARSINGDEBUG
    messageParsingLayer("internalPackOutcommingData(): start");
    #endif
    #ifdef DEBUG_PROTOCOLPARSING_RAW_NETWORK
    std::cout << "Sended packet size: " << size << ": " << binarytoHexa(data,size) << std::endl;
    #endif // DEBUG_PROTOCOLPARSING_RAW_NETWORK
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(size>CATCHCHALLENGER_MAX_PACKET_SIZE)
    {
        errorParsingLayer("ProtocolParsingInputOutput::sendRawSmallPacket(): Packet to big: "+std::to_string(size));
        return false;
    }
    #endif

    const ssize_t &byteWriten = write(data,size);
    if(Q_UNLIKELY(size!=byteWriten))
    {
        disconnectClient();
        return false;
    }
    return true;
}

bool ProtocolParsingBase::removeFromQueryReceived(const uint8_t &queryNumber)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(queryNumber>(CATCHCHALLENGER_MAXPROTOCOLQUERY-1))
    {
        std::cerr << "queryNumber: " << queryNumber << " < CATCHCHALLENGER_MAXPROTOCOLQUERY: " << CATCHCHALLENGER_MAXPROTOCOLQUERY << std::endl;
        abort();
    }
    if(inputQueryNumberToPacketCode[queryNumber]==0x00)
    {
        errorParsingLayer("ProtocolParsingBase::removeFromQueryReceived already replied");
        return false;
        //abort();
    }
    #endif
    inputQueryNumberToPacketCode[queryNumber]=0x00;
    return true;
}
#endif
