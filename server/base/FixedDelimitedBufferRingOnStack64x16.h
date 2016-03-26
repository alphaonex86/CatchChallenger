#ifndef FIXEDDELIMITEDBUFFERRINGONSTACK64X16_H
#define FIXEDDELIMITEDBUFFERRINGONSTACK64X16_H

#include <array>

class FixedDelimitedBufferRingOnStack64x16
{
public:
    FixedDelimitedBufferRingOnStack64x16();
private:
    std::array<void *,64> data;
    std::array<uint8_t,16> delimiter;
    uint8_t numberOfItems;
    uint8_t delimiterIndex;
};

#endif // FIXEDDELIMITEDBUFFERRINGONSTACK64X16_H
