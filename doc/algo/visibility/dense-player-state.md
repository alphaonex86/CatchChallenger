# DensePlayerState — 32-bit packed slot state for min_network()

`MapVisibilityAlgorithm::min_network()` (server/base/MapManagement) sends
each recipient only the players that changed since the last tick. That
needs, per recipient, a copy of "what was last sent" (`ClientWithMap::
sendedStatus`) diffed every tick against the map's current snapshot
(`MapVisibilityAlgorithm::tempDenseBuffer`). The diff is O(N²) per map
per tick — the hottest loop in the server — so the slot state layout IS
the performance.

## Layout (DensePlayerState.hpp)

Both sides of the diff use the SAME type: a union holding ONE 32-bit
word per slot.

```
whole = x | y<<8 | direction<<16 | (database_id & 0xff)<<24
empty slot: whole == 0xffffffff
```

* One `uint32_t` compare answers "anything to send for this slot?" —
  including the empty==empty case. No branch on a separate id field.
* A 255-slot snapshot is ~1 KB (was 2 KB with the previous 8-byte
  `{db_id, xyd}` pair): fits L1 on every target, and the per-client
  `sendedStatus` RAM is halved.
* The empty marker cannot collide with a live player: direction is
  always 1..8 (never 0xff) and x is COORD_TYPE 0..127.
* Compose and extract ONLY via shifts on `whole` (endian-neutral —
  same wire bytes on big-endian MIPS). The `bytes[]` view is for
  debugger inspection, not for wire writes or compares.

## Database-id truncation — accepted tradeoff

The character database id is stored truncated to its LOW 8 BITS. It
only serves to detect "a DIFFERENT character now occupies this slot"
between two consecutive ticks of the same map. If the old and the new
character share the low byte (~1/256 of an already-rare slot-reuse-
within-one-tick event), the full re-insert downgrades to a position
update; the recipient keeps the stale identity until the next map
change, where reinsert-all refreshes every slot. The client already
tolerates/drops inconsistent entries (the visibility cache has no
coherency by design). Decided by the project owner 2026-06-10.

On mismatch the decode order matters: empty checks MUST run before the
db-byte compare, because the empty marker's db byte is 0xff and a live
id ending in 0xff would alias it.

## 0x66 change entry — single-store compose

The wire entry [slot][x][y][direction] is composed in a register from
the packed word (`((whole & 0xffffff)<<8) | slot`) and flushed with one
htole32+memcpy 32-bit store instead of 4 byte stores. On the previous
8-byte layout this trick measured as noise and was removed (ff93e543);
on the packed word it is a real win because the source is already one
register.

## Measured results (2026-06-10, benchmarkmapmanager.py fleet)

Baseline: champion bc24abf4-era (8-byte slot pair). Candidates
`2026-06-10T12-28-43Z` (truncation only) and `2026-06-10T12-58-21Z`
(+single-store entry, the version kept). 11 load-gated exec nodes +
32-bit siblings; local workstation rows discarded (contended).

* Throughput (ticks/s): +63% pentium-3 @200p, +24% MIPS rtl9607c
  @200p, +42..+49% @5p on atom-n455/odroid-c2/rpi-3, +5..+15% across
  50–300p on geode/odroid-n2/rpi-4/pentium-m. In CPU-per-tick terms at
  >20 players: 9 of 10 nodes flat-to−39% cheaper.
* p95 tick latency mostly down (rpi-3 −8..−25%, pentium-3 −20..−23%
  @200/300p).
* RSS −2..−11% on every node; wire bytes/tick within ±3% (identical
  protocol behaviour); binary size unchanged.
* Soft spots: the 10-player count on atom-n455/odroid-c2 (−9..−16%)
  and odroid-c2 mildly negative at 50–300p (−2..−5%) under the
  pessimistic everyone-moves-every-tick workload. Production ticks have
  few movers, so the dominant path is the equality fast path — strictly
  cheaper than before (4-byte vs 8-byte compare). Owner decision: KEEP.

## Tried and rejected (don't redo blindly)

* **Byte-view compose/extract** (`bytes[0]=x; ...`, no shifts —
  candidate `2026-06-10T15-42-56Z`): −4..−13% @50–300p on 17/20
  node-rows, callgrind instructions +7..+13%. The byte view re-loads
  bytes from memory for the db compare and the wire write; the shift
  version derives everything from the word already in a register.
* **SIMD** (SSE2 skip-equal prescan): +41% slower — double-reads the
  arrays; see benchmark history. Data layout beats SIMD here.
* **Collapsing the per-recipient diff into one shared broadcast**:
  rejected by design — that is what min_CPU is for; min_network exists
  to minimise network per recipient.

History/raw data: `benchmark/history/benchmarkmapmanager/` (local-only,
git-ignored); correctness: `test/testingmapmanagement.py` (21 cases,
incl. wire-byte parsing of every packet type).
