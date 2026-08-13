#ifndef CATCHCHALLENGER_HPS_QUIET_HPP
#define CATCHCHALLENGER_HPS_QUIET_HPP

// Single entry point for the vendored general/hps/ serializer.
//
// hps is vendored as-is and must not be modified (nor its build flags), but it
// is header-only template code and accounts for ~230 of the -Wconversion
// warnings on a single binary — enough to bury the ones in our own code. Our
// sources include it by relative path, so -isystem on the include dir cannot
// reach it; the fix therefore lives on our side, here, and the vendor tree stays
// untouched.
//
// Include this instead of general/hps/*.h. The pragmas are guarded because MSVC
// reports C4068 on an unknown pragma and this tree also builds there.

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#endif

#include "hps/hps.h"
#include "hps/serializer.h"
#include "hps/basic_type/int_serializer.h"

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#endif // CATCHCHALLENGER_HPS_QUIET_HPP
