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
        bool parseIncommingDataRaw(const char *commonBuffer, const uint32_t &size,uint32_t &cursor);
    private:
        void parseMessage(const uint8_t &packetCode,const char *data,const unsigned int &size);
        void parseFullMessage(const uint8_t &packetCode,const uint8_t &subCodeType,const char *data,const unsigned int &size);
        //have query with reply
        void parseQuery(const uint8_t &packetCode,const uint8_t &queryNumber,const char *data,const unsigned int &size);
        void parseFullQuery(const uint8_t &packetCode,const uint8_t &subCodeType,const uint8_t &queryNumber,const char *data,const unsigned int &size);
        //send reply
        void parseReplyData(const uint8_t &packetCode,const uint8_t &queryNumber,const char *data,const unsigned int &size);
        void parseFullReplyData(const uint8_t &packetCode,const uint8_t &subCodeType,const uint8_t &queryNumber,const char *data,const unsigned int &size);
        //message
        void errorParsingLayer(const std::string &error);
        void messageParsingLayer(const std::string &message) const;

        #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
        ProtocolParsing::CompressionType getCompressType() const; /// if client get server because it's check then mirror
        #endif

        void disconnectClient();

        ssize_t read(char * data, const size_t &size);
        ssize_t write(const char * data, const size_t &size);
};

}

#endif

#endif // PROTOCOLPARSING_H
