#!/usr/bin/env python3
"""verify_workload.py -- check a generated stage-2 workload, independently.

    python3 benchmark/benchmarkmapmanager2/verify_workload.py <workload.cpp>

Stage 1 SIMULATES a walk and then EMITS the vectors that reproduce it; stage 2
replays those vectors. Both agree by construction, so a bug in the encoding or
in the meaning of an entry would be invisible to both -- they would simply be
wrong together. This is a THIRD implementation of the same spec, written from
the header contract rather than from either stage, and it checks:

  * the end-of-cycle state it reaches equals the CYCLE_END_HASH stage 1
    computed from its own simulation, so the emitted vectors really do
    reproduce the simulated walk; and
  * no step ever leaves its map, using the dimensions the workload carries.

Exit code 0 when both hold. The harness runs it on the local workload of every
run; point it at any generated file by hand.
"""
import re, sys

src = open(sys.argv[1]).read()

def scalar(name, cast=int):
    m = re.search(r'^const \w+ %s = (\d+)u?u?l?l?;' % name, src, re.M)
    return cast(m.group(1)) if m else None

def array(name):
    m = re.search(r'^const \w+ %s\[\d+\] = \{(.*?)\n\};' % name, src, re.M | re.S)
    if not m:
        return []
    return [int(x) for x in m.group(1).replace('u', '').replace('\n', '').split(',') if x.strip()]

P      = scalar("PLAYERS")
CYCLE  = scalar("CYCLE_TICKS")
K      = scalar("ENTRIES_PER_PLAYER")
MAPS   = scalar("MAPS")
HASH   = scalar("CYCLE_END_HASH")
MIGS   = scalar("MIGRATIONS")
MAP_W, MAP_H = array("MAP_W"), array("MAP_H")
smap, sx, sy, sf = array("SPAWN_MAP"), array("SPAWN_X"), array("SPAWN_Y"), array("SPAWN_FACING")
replay = array("REPLAY")
mt, mp, mm, mx, my = (array("MIG_TICK"), array("MIG_PLAYER"), array("MIG_MAP"),
                      array("MIG_X"), array("MIG_Y"))

assert len(replay) == P * K, (len(replay), P * K)
assert len(smap) == P and len(sx) == P and len(sy) == P and len(sf) == P

pmap, px, py, pf = list(smap), list(sx), list(sy), list(sf)
entry = [0] * P
rem   = [0] * P
cur   = [0] * P
oob = 0
nxt = 0
for tick in range(CYCLE):
    for i in range(P):
        if rem[i] == 0:
            e = replay[i * K + entry[i]]
            cur[i] = e >> 5
            rem[i] = (e & 31) + 1
            entry[i] += 1
            if entry[i] >= K:
                entry[i] = 0
            if cur[i]:
                pf[i] = cur[i] - 1
        if cur[i]:
            if   pf[i] == 0: py[i] -= 1
            elif pf[i] == 1: px[i] += 1
            elif pf[i] == 2: py[i] += 1
            else:            px[i] -= 1
            if not (0 <= px[i] < MAP_W[pmap[i]] and 0 <= py[i] < MAP_H[pmap[i]]):
                oob += 1
        rem[i] -= 1
    while nxt < MIGS and mt[nxt] == tick:
        i = mp[nxt]
        pmap[i], px[i], py[i] = mm[nxt], mx[nxt], my[nxt]
        rem[i] = 0
        cur[i] = 0
        nxt += 1

h = 1469598103934665603
for i in range(P):
    for v in (pmap[i], px[i], py[i], pf[i]):
        h = ((h ^ v) * 1099511628211) & 0xFFFFFFFFFFFFFFFF
print("players=%d cycle=%d entries=%d maps=%d migrations=%d" % (P, CYCLE, K, MAPS, MIGS))
print("out_of_map_steps =", oob)
print("hash python      = %d" % h)
print("hash stage1      = %d" % HASH)
print("MATCH" if h == HASH and oob == 0 else "MISMATCH")
sys.exit(0 if (h == HASH and oob == 0) else 1)
