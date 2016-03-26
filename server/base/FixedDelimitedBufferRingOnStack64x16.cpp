#include "FixedDelimitedBufferRingOnStack64x16.h"

FixedDelimitedBufferRingOnStack64x16::FixedDelimitedBufferRingOnStack64x16() :
    delimiter({0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}),
    numberOfItems(0),
    delimiterIndex(0)
{
}

