#ifndef CATCHCHALLENGER_HASH_H
#define CATCHCHALLENGER_HASH_H

#include <cstdint>
#include <cstddef>
#include "lib.h"

#define CATCHCHALLENGER_HASH_SIZE 32

namespace CatchChallenger {
class DLL_PUBLIC Hash
{
public:
    Hash();
    void init();
    void update(const unsigned char *data, unsigned int len);
    void final(unsigned char *digest);
    static const unsigned int DIGEST_SIZE = CATCHCHALLENGER_HASH_SIZE;
private:
    //opaque storage sized to hold blake3_hasher (1912 bytes)
    alignas(8) unsigned char state[1912];
};
}

#endif // CATCHCHALLENGER_HASH_H
