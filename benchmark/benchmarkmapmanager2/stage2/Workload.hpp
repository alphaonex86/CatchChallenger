#ifndef CATCHCHALLENGER_BENCH_WORKLOAD_H
#define CATCHCHALLENGER_BENCH_WORKLOAD_H

#include <stdint.h>

/* The workload stage 1 generated for THIS execution node and THIS datapack.
 * The definitions live in the generated .cpp that CMake compiles alongside
 * (CC_WORKLOAD_CPP); this header is only its contract.
 *
 * Everything is const, so it lands in .rodata -- or in flash on a board with
 * no filesystem to read a datapack from, which is the point of generating it.
 *
 * REPLAY is player-major, ENTRIES_PER_PLAYER bytes each. One byte is one
 * movement vector:
 *     dir = byte >> 5     0 = stand still, 1..4 = walk top/right/bottom/left
 *     len = (byte & 31)+1 how many TICKS it lasts (one cell per tick when
 *                         walking), i.e. the next vector of that player is
 *                         only fetched len ticks later.
 * Every walking vector was validated cell by cell against the datapack's real
 * collisions by the production predicate at generation time, so replaying it
 * needs no map data and can never walk into a wall. */
namespace CCBenchWorkload {
extern const char NODE[];
extern const char DATAPACK[];
extern const uint32_t PLAYERS;
extern const uint32_t CYCLE_TICKS;          // replay length before it loops
extern const uint32_t ENTRIES_PER_PLAYER;
extern const uint16_t MAPS;
extern const uint64_t CYCLE_END_HASH;       // oracle: state after one cycle

extern const uint8_t  MAP_W[];               // all stage 2 needs of a map
extern const uint8_t  MAP_H[];
/* 4 by map, top/bottom/left/right: neighbour index (65535 = none) and the
 * RESOLVED delta offset, the very values CommonMap::border carries on the
 * server. min_network() reaches a border map only through them. */
extern const uint16_t MAP_BORDER[];
extern const int8_t   MAP_BORDER_OFFSET[];
extern const uint8_t  WORLD_ZOOM;           // datapack map/layers.xml zoom

extern const uint16_t SPAWN_MAP[];
extern const uint8_t  SPAWN_X[];
extern const uint8_t  SPAWN_Y[];
extern const uint8_t  SPAWN_FACING[];

extern const uint8_t  REPLAY[];

extern const uint32_t MIGRATIONS;           // map changes, sorted by tick
extern const uint32_t MIG_TICK[];
extern const uint32_t MIG_PLAYER[];
extern const uint16_t MIG_MAP[];
extern const uint8_t  MIG_X[];
extern const uint8_t  MIG_Y[];
}

#endif
