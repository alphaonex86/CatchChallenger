#ifndef CATCHCHALLENGER_DENSEPLAYERSTATE_H
#define CATCHCHALLENGER_DENSEPLAYERSTATE_H

#include <cstdint>

namespace CatchChallenger {
/* Per-slot player state, shared by MapVisibilityAlgorithm::tempDenseBuffer
 * (the per-tick map snapshot) and ClientWithMap::sendedStatus (the
 * per-recipient last-sent state). The min_network() diff loop talks ONLY to
 * the inline helpers below, so the same algorithm compiles against either
 * layout:
 *
 * - default (NOT truncated): {db_id, xyd} pair, 8 bytes per slot. The FULL
 *   32-bit character database id is compared, so "a different character now
 *   occupies this slot" is detected exactly.
 *
 * - -DCATCHCHALLENGER_VISIBILITY_TRUNCATED_DB_ID: ONE 32-bit word per slot
 *   (x | y<<8 | direction<<16 | (db_id&0xff)<<24). Halves snapshot +
 *   sent-state memory and the per-slot diff is a single uint32_t compare —
 *   measured +5..+63% min_network throughput across the fleet (see
 *   doc/algo/visibility/dense-player-state.md) — at the cost of the db id
 *   truncated to its LOW 8 BITS: a slot reuse within one tick where the old
 *   and new character share the low byte (~1/256 of an already-rare event)
 *   is sent as a move instead of a re-insert until the next map change
 *   refreshes every slot.
 *
 * Both layouts:
 * - empty slot marker: every field all-ones, written ONLY via setEmpty()
 *   (canonical, so isEqual() of two empty slots is always true). It cannot
 *   collide with a live player: direction is 1..8 (never 0xff) and x is
 *   COORD_TYPE 0..127.
 * - isEmpty() MUST be checked before isSameCharacter(): the empty marker's
 *   db byte is 0xff, which a live id ending in 0xff would alias under the
 *   truncated layout.
 * - compose/extract via shifts only (endian-neutral: same wire bytes on the
 *   big-endian MIPS nodes), trivially copyable (sent-state refresh is one
 *   flat memcpy of the dense snapshot). */
#ifdef CATCHCHALLENGER_VISIBILITY_TRUNCATED_DB_ID
union DensePlayerState {
    uint32_t whole;//x | y<<8 | direction<<16 | (db_id&0xff)<<24
    uint8_t bytes[4];//host-endian debugger view, never used for wire writes

    inline void set(const uint8_t &x,const uint8_t &y,const uint8_t &direction,const uint32_t &database_id)
    {
        whole=(uint32_t)x | ((uint32_t)y<<8) | ((uint32_t)direction<<16) | ((database_id&0xff)<<24);
    }
    inline void setEmpty()
    {
        whole=0xffffffff;
    }
    inline bool isEmpty() const
    {
        return whole==0xffffffff;
    }
    inline bool isEqual(const DensePlayerState &other) const
    {
        return whole==other.whole;
    }
    //same character database id (low byte only here). Only meaningful when
    //neither side isEmpty() — see the header comment.
    inline bool isSameCharacter(const DensePlayerState &other) const
    {
        return ((whole^other.whole)&0xff000000)==0;
    }
    //host-order word for a 0x66 change entry [slot][x][y][direction]:
    //slot | x<<8 | y<<16 | direction<<24. Caller stores it with
    //htole32+memcpy so the wire bytes are the same on every host.
    inline uint32_t wireChangeWord(const uint8_t &slotIndex) const
    {
        return ((whole&0x00ffffff)<<8)|slotIndex;
    }
    inline uint8_t getX() const
    {
        return (uint8_t)whole;
    }
    inline uint8_t getY() const
    {
        return (uint8_t)(whole>>8);
    }
};
static_assert(sizeof(DensePlayerState)==4,
              "DensePlayerState (truncated layout) must be exactly one 32-bit word per slot");
#else
struct DensePlayerState {
    uint32_t db_id;//full character database id, 0xffffffff if empty
    uint32_t xyd;//x | y<<8 | direction<<16 (live slot: high byte 0; empty slot: 0xffffffff)

    inline void set(const uint8_t &x,const uint8_t &y,const uint8_t &direction,const uint32_t &database_id)
    {
        db_id=database_id;
        xyd=(uint32_t)x | ((uint32_t)y<<8) | ((uint32_t)direction<<16);
    }
    inline void setEmpty()
    {
        db_id=0xffffffff;
        xyd=0xffffffff;
    }
    inline bool isEmpty() const
    {
        return db_id==0xffffffff;
    }
    inline bool isEqual(const DensePlayerState &other) const
    {
        return db_id==other.db_id && xyd==other.xyd;
    }
    inline bool isSameCharacter(const DensePlayerState &other) const
    {
        return db_id==other.db_id;
    }
    inline uint32_t wireChangeWord(const uint8_t &slotIndex) const
    {
        return ((xyd&0x00ffffff)<<8)|slotIndex;
    }
    inline uint8_t getX() const
    {
        return (uint8_t)xyd;
    }
    inline uint8_t getY() const
    {
        return (uint8_t)(xyd>>8);
    }
};
static_assert(sizeof(DensePlayerState)==8,
              "DensePlayerState (full-id layout) must be exactly 8 bytes with no padding");
#endif
}

#endif
