#include "../base/Client.hpp"

using namespace CatchChallenger;

bool Client::sendInventory()
{
    if(sizeof(ProtocolParsingBase::tempBigBufferForOutput)<=(1+4+
                                                            (2+4)*public_and_private_informations.items.size()
                                                            ))
    {
        errorOutput("Too many items to be send");
        return false;
    }
    //send the network message
    uint32_t posOutput=0;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x54;
    posOutput+=1+4;

    *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole16(public_and_private_informations.items.size());
    posOutput+=2;
    {
        auto i=public_and_private_informations.items.begin();
        while(i!=public_and_private_informations.items.cend())
        {
            *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole16(i->first);
            posOutput+=2;
            *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole32(i->second);
            posOutput+=4;
            ++i;
        }
    }

    *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1)=htole32(posOutput-1-4);//set the dynamic size
    return sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
}
