#ifndef CATCHCHALLENGER_DENSEPLAYERSTATE_H
#define CATCHCHALLENGER_DENSEPLAYERSTATE_H

#include <cstdint>

namespace CatchChallenger {
/* Packed per-slot player state, shared by MapVisibilityAlgorithm::
 * tempDenseBuffer (the per-tick map snapshot) and ClientWithMap::
 * sendedStatus (the per-recipient last-sent state). ONE 32-bit word per
 * slot, so the per-tick diff is a single uint32_t compare per slot and a
 * full 255-slot snapshot is ~1KB (fits L1 even on the constrained
 * targets).
 *
 *   whole = x | y<<8 | direction<<16 | (database_id & 0xff)<<24
 *
 * The character database id is TRUNCATED to its low 8 bits: it only
 * serves to detect "a DIFFERENT character now occupies this slot"
 * between two consecutive ticks of the same map. A truncation collision
 * (old and new character share the low byte, ~1/256 of an already-rare
 * slot-reuse-within-one-tick event) downgrades the full re-insert to a
 * position update; slot identity is fully refreshed by the reinsert-all
 * on any map change, so the stale identity never survives a map move.
 *
 * Empty slot marker: whole == 0xffffffff. It can never collide with a
 * live player because direction is always 1..8 (never 0xff) and x is
 * COORD_TYPE 0..127.
 *
 * Compose and extract ONLY via shifts on `whole` (endian-neutral: same
 * wire bytes on the MIPS big-endian nodes). The bytes[] view is
 * host-endian, for debugger inspection only — never use it for wire
 * writes or cross-slot compares. */
union DensePlayerState {
    uint32_t whole;
    uint8_t bytes[4];
};
}

#endif
