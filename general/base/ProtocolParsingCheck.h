#if ! defined (ONLYMAPRENDER)
#ifndef CATCHCHALLENGER_PROTOCOLPARSINGCHECK_H
#define CATCHCHALLENGER_PROTOCOLPARSINGCHECK_H

#include "ProtocolParsing.h"
#include "GeneralVariable.h"

#ifdef CATCHCHALLENGER_EXTRA_CHECK

namespace CatchChallenger {

class ProtocolParsingCheck : public ProtocolParsingBase
{
    public:
        ProtocolParsingCheck(const PacketModeTransmission &packetModeTransmission);
        friend class Client;
        friend class ProtocolParsingBase;
        friend class ProtocolParsingInputOutput;
        bool valid;
        int8_t parseIncommingDataRaw(const char *commonBuffer, const uint32_t &size,uint32_t &cursor);
        void storeInputQuery(const uint8_t &packetCode,const uint8_t &queryNumber);
        void registerOutputQuery(const uint8_t &queryNumber, const uint8_t &packetCode);
    private:
        bool parseMessage(const uint8_t &packetCode,const char *data,const unsigned int &size);
        //have query with reply
        bool parseQuery(const uint8_t &packetCode,const uint8_t &queryNumber,const char *data,const unsigned int &size);
        //send reply
        bool parseReplyData(const uint8_t &packetCode,const uint8_t &queryNumber,const char *data,const unsigned int &size);
        //message
        void errorParsingLayer(const std::string &error);
        void messageParsingLayer(const std::string &message) const;
        void moveClientFastPath(const uint8_t &previousMovedUnit,const uint8_t &direction);
        void breakNeedMoreData();

        #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
        ProtocolParsing::CompressionType getCompressType() const; /// if client get server because it's check then mirror
        #endif

        bool disconnectClient();

        ssize_t read(char * data, const size_t &size);
        ssize_t write(const char * data, const size_t &size);
};

}

#endif

#endif // PROTOCOLPARSING_H
#endif
