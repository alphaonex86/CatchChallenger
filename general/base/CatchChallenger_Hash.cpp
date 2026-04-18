#include "CatchChallenger_Hash.hpp"
#include "../blake3/blake3.h"
#include <cstring>

using namespace CatchChallenger;

Hash::Hash()
{
    init();
}

void Hash::init()
{
    blake3_hasher_init(reinterpret_cast<blake3_hasher *>(state));
}

void Hash::update(const unsigned char *data, unsigned int len)
{
    blake3_hasher_update(reinterpret_cast<blake3_hasher *>(state), data, len);
}

void Hash::final(unsigned char *digest)
{
    blake3_hasher_finalize(reinterpret_cast<const blake3_hasher *>(state), digest, CATCHCHALLENGER_HASH_SIZE);
}
