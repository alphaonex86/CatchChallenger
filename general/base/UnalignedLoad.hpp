#ifndef CATCHCHALLENGER_UNALIGNED_LOAD_HPP
#define CATCHCHALLENGER_UNALIGNED_LOAD_HPP

// Aligned-safe protocol field load / store helpers.
//
// Why: the wire protocol packs uint16/32/64 fields back-to-back in a
// byte buffer, so any given field can land on an odd offset.  On x86
// `*reinterpret_cast<uint32_t*>(ptr)` against an unaligned pointer is
// fine; on strict-alignment archs (MIPS32-BE under qemu-user, RISC-V,
// older ARM, anything with CPU_ALIGNMENT_FAULT enabled) it traps
// SIGBUS and the server vanishes mid-handshake.  The unfixed pattern
// gave a "::accept() return -1 ... qemu: uncaught target signal 10
// (Bus error)" crash on every first client→mips connect.
//
// memcpy of a fixed small size is folded by every supported compiler
// (gcc/clang at -O0+) into a single MOV / LW so this is zero-cost on
// x86 and uses safe byte loads on MIPS.  Pair load* with store* on
// the write side: same reasoning, same fix.

#include <cstdint>
#include <cstring>

#if defined(_WIN32) || defined(_WIN64)
    // Windows: define le16/32/64toh / htole16/32/64 as identity since
    // Win is always little-endian on the targets we support.
    #ifndef le16toh
        #define le16toh(x) (x)
        #define le32toh(x) (x)
        #define le64toh(x) (x)
        #define htole16(x) (x)
        #define htole32(x) (x)
        #define htole64(x) (x)
    #endif
#elif defined(__APPLE__)
    // macOS doesn't ship a Linux-style <endian.h>; it has
    // <libkern/OSByteOrder.h> with OSSwap* helpers and CPU_LITTLE_ENDIAN
    // / CPU_BIG_ENDIAN macros. Map the Linux names onto OSSwap so the
    // rest of this file (and every caller) compiles unchanged. macOS
    // is little-endian on every supported target (x86_64, arm64),
    // hence the noop branch.
    #include <libkern/OSByteOrder.h>
    #ifndef le16toh
        #define le16toh(x) OSSwapLittleToHostInt16(x)
        #define le32toh(x) OSSwapLittleToHostInt32(x)
        #define le64toh(x) OSSwapLittleToHostInt64(x)
        #define htole16(x) OSSwapHostToLittleInt16(x)
        #define htole32(x) OSSwapHostToLittleInt32(x)
        #define htole64(x) OSSwapHostToLittleInt64(x)
    #endif
#elif defined(__DJGPP__)
    // MS-DOS (DJGPP): little-endian x86, no <endian.h>. host<->little no-op.
    #ifndef le16toh
        #define le16toh(x) (x)
        #define le32toh(x) (x)
        #define le64toh(x) (x)
        #define htole16(x) (x)
        #define htole32(x) (x)
        #define htole64(x) (x)
    #endif
#else
    #include <endian.h>
#endif

namespace CatchChallenger {

inline uint16_t loadLe16(const void *p)
{
    uint16_t v;
    std::memcpy(&v, p, sizeof(v));
    return le16toh(v);
}

inline uint32_t loadLe32(const void *p)
{
    uint32_t v;
    std::memcpy(&v, p, sizeof(v));
    return le32toh(v);
}

inline uint64_t loadLe64(const void *p)
{
    uint64_t v;
    std::memcpy(&v, p, sizeof(v));
    return le64toh(v);
}

inline void storeLe16(void *p, uint16_t v)
{
    v = htole16(v);
    std::memcpy(p, &v, sizeof(v));
}

inline void storeLe32(void *p, uint32_t v)
{
    v = htole32(v);
    std::memcpy(p, &v, sizeof(v));
}

inline void storeLe64(void *p, uint64_t v)
{
    v = htole64(v);
    std::memcpy(p, &v, sizeof(v));
}

}  // namespace CatchChallenger

#endif  // CATCHCHALLENGER_UNALIGNED_LOAD_HPP
