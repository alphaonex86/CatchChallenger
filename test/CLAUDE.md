# test/ — testing harness

**Always read `../CLAUDE.md` first.** When Claude is launched with
`test/` as the working directory, the root CLAUDE.md is NOT auto-
loaded — open `../CLAUDE.md` before doing anything else and combine
its rules with the ones below. The root file owns project-wide
conventions: C++/Qt style, CMake-per-binary layout, never-search-
from-`/`, MXE prefix, "don't ask — just continue", … and they apply
to the testing harness too.

## Datapack `map/main/test/` — intentional bugs as fixtures

`datapack/map/main/test/` maincode is **intentionally broken** to exercise defensive parsing. **Do NOT "fix" these.** Expected messages:

* `city.xml`: `Missing attribute: child->Name(): monster`; `map have empty monster layer: ... type:lava total luck is not egal to 100 (0)`.
* `gym.tmx`: `Missing "object" properties (item) for the bot:...`; `Missing map,x or y, type: teleport on it`; `wrong teleporter on the map: gym(1,19), to gym(120,120) because the tp is out of range`.

When adding new validation, add a matching broken fixture. Absence of message (or crash on load) = real failure.

* Run `test/all.sh` for all testing*.py sequentially. `--continue` continues on failure.
* **Fix as many bugs per iteration as possible.** Scan `/mnt/data/perso/tmpfs/failed.json` for ALL failures, group by root cause, fix all in this turn.
* **Keep host CPU busy.** Always overlap remote compile threads with local work; prefetch next-phase data; pre-warm ccache. At any minute host should have cc1plus running OR a binary in test phase.
* **Run testing*.py in parallel where they don't conflict.** Group by mutable resources:
  - `testing-cpu/`, `testing-gl/` — testingclient solo only
  - `testing-filedb/` + port `61917` + `database/` — testingclient multi, testingbots, testingserver, testinghttp, testingmulti (sequential among themselves)
  - `testing-multi-{cpu,gl}/` + port `61917` — testingmulti only
  - Cross-compile scripts — own toolchains, can run alongside above
  Safe parallel lanes: (1) testingmap2png + testingmap4client; (2) testingcompilation{windows,mac,android}.
* **Compile under `nice -n 19 ionice -c 3`; runtime tests at NORMAL priority.** `NICE_PREFIX_COMPILE = ["nice","-n","19","ionice","-c","3"]` for cmake/make/qmake; `NICE_PREFIX_RUNTIME = []` for start_server/run_client/bot-actions/autosolo.
* Do NOT use Monitor to poll testing*.py. Run with run_in_background (timeout up to 12h), wait for completion.
* Compilation on remote (982.vps.confiared.com, 2803:1920::3:440a) and local can parallelize.
* Client valid + on map: `success_marker="MapVisualiserPlayer::mapDisplayedSlot()"`.
* testingserver.py PASS: client console shows `CommonSettingsServer::commonSettingsServer.httpDatapackMirrorServer is now: \n` AND `CommonSettingsCommon::commonSettingsCommon.httpDatapackMirrorBase is now: \n`.
* testinghttp.py PASS: NOT have those messages.
* Server startup OK: console shows `correctly bind:`.
* NOXML: generate `datapack-cache.bin` via server `--save`, then recompile with CATCHCHALLENGER_NOXML. Server checks `datapack-cache.bin` mtime matches `server-properties.xml`. Sync mtime if XML modified. Clean cache after each test.
* `datapack-cache.bin` is NOT endianless: float/double via raw memcpy (HPS), so LE-built cache won't load on BE.
* Client connects → datapack syncs to server's.
* Resume with `failed.json`: `test/all.sh --continue` → failures in `/mnt/data/perso/tmpfs/failed.json` → fix+`make -j32` → `--continue` re-runs only failed → repeat → `rm failed.json` → full run → repeat if failures.
* Force fresh: `rm -f /mnt/data/perso/tmpfs/failed.json`.
* Target node: `--node <label>` (repeatable) or `CC_NODE_FILTER=mips-lxc,x86-lxc`. Bypasses per-node `enabled` flag.

## All remote_nodes.json fields are mandatory

Every key in `_doc` MUST exist on every node/execution_node. No defaults — `remote_build.py` validates at load. Required: `_REQUIRED_NODE_KEYS`, `_REQUIRED_EXEC_NODE_KEYS`. Adding a field: update `_doc` + `_REQUIRED_…` tuple + **every** existing entry. Empty != missing — write `compile_sql=[]` explicitly.

## Database backends in testing*.py — file_db on host, SQL gated to remote_nodes.json

**testing*.py never opens an SQL DB connection.** Host runs only file-db (`CATCHCHALLENGER_DB_FILE`); no remote/exec node has SQL pre-configured. Direct `psycopg2.connect`/`mysql.connector.connect` forbidden. State wiping for SQL = let server binary do it.

SQL coverage opt-in per node: compile `"compile_sql": ["MySQL","PostgreSQL","SQLite"]` (extra build passes); execution `"execute_sql": [...]` (extra start-server+connect-client passes; daemon on `localhost`, default port, DB **`catchchallenger`**, harness asks server to wipe — drop tables, replay `server/databases/<backend>/*.sql`). Default `[]` = file-db only.

testingserver.py + execute_sql contract: operator has installed daemon on localhost default port, created DB+user `catchchallenger`/`catchchallenger` with full DDL+DML. testingserver.py drops tables, replays `.sql`, connects via server with standard creds; MUST NOT systemctl/launch daemon, CREATE/DROP DATABASE, CREATE USER, or hardcode different host/port/user.

SQLite: no daemon, no auth — fixed path `<paths.tmpfs_root>/catchchallenger.sqlite3`, `sqlite3` CLI on $PATH.

## SSH endpoints for remote / exec nodes

Authoritative source: `paths.remote_nodes_json` (default `/home/user/Desktop/CatchChallenger/remote_nodes.json`). Each compile `node` has `ssh` dict (`user`,`host`,`port`); each `execution_nodes[]` has same fields **flat** (no nested `ssh`). Always read it — table is a snapshot.

| label | user@host:port | role |
|---|---|---|
| mips-lxc | `root@2803:1920::2:ff03` | compile + exec |
| x86-lxc | `root@2803:1920::2:ff04` | compile + exec |
| pentium-m | `user@2803:1920::2:ff01` | exec |
| atom-n455 | `user@2803:1920::2:ff02` | exec |
| osxcross | `root@2803:1920::2:ff08` | compile (mac) |

Bracket IPv6 hosts for rsync (`user@[2803:1920::2:ff04]:`); plain ssh accepts unbracketed. `cmd_helpers._rsync_host()` handles brackets.

## Some "remote" nodes AND execution nodes are local LXC containers

`mips-lxc` (`2803:1920::2:ff03`), `x86-lxc` (`2803:1920::2:ff04`), `osxcross` (`2803:1920::2:ff08`) are **local LXC payloads**. Same containers also appear as exec nodes for other compile entries. Reach via host's `2803:1920::2:/112` private prefix; share host CPU/RAM; live under `/sys/fs/cgroup/lxc.payload.<name>/cgroup.procs`.

* Own PID namespace — `ps` on host won't show container binaries by internal PID. Iterate `/sys/fs/cgroup/lxc.payload.*/cgroup.procs` for host-side pids.
* ccache slot at `<work_dir>/ccache/` on container FS. Read via `ssh <user>@<lxc-ip> 'CCACHE_DIR=<path> ccache -s'`.
* Kill hung processes via SSH (`ssh root@<lxc-ip> kill <pid-inside-container>`), not from host (cross-ns kill privileged).
* Monitor cc1plus across BOTH host and LXC payload cgroups.

## Cross-platform client phases (testingclient.py)

Three optional client phases at end of `testingclient.py`, all use **local Linux `server-filedb`**. Server/tools NOT cross-compiled.

**`--host` for multi runs:** clients on host (wine64) use `SERVER_HOST` (localhost/127.0.0.1); clients on remote (qemu mac VM, Android emulator) use this host's LAN IP — `SERVER_HOST_REMOTE = "192.168.158.10"`. Solo runs (`--autosolo --closewhenonmap`) need no server.

## Environment hygiene for test scripts

Behave as if from fresh post-restart shell:

1. **No global pip installs.** Use venv under workspace dir (Android: `/mnt/data/perso/progs/CatchChallenger-android/venv/`).
2. **Build env from scratch.** Construct fresh `dict` (not `os.environ.copy()`); explicitly set every var. Forward only minimum (`HOME`, `USER`, `LANG`, `LC_*`, `TERM`, `TMPDIR`, `DISPLAY`). Don't inherit `ANDROID_*`, `QT_*`, `JAVA_HOME`, `LD_LIBRARY_PATH`, `PYTHONPATH`, `QT_QPA_*`.
3. **No leakage to parent shell.** Never `os.environ[X] = ...`. Each subprocess gets env via `subprocess.run(..., env=...)`.
4. **Workspace dirs exempt.** `/mnt/data/perso/progs/CatchChallenger-android/` etc. allowed to grow caches.

Reference: `android_env()` in `test/testingclient.py`.

* **Windows** — MXE at `/mnt/data/perso/progs/catchchallenger-mxe/` (`MXE_TARGETS="x86_64-w64-mingw32.shared"`). cmake+ninja with ccache+mold. Run under `wine64`. Deploy `windeployqt.exe` + MXE mingw DLLs (libstdc++/libgcc/libwinpthread). Datapack at `<exe_dir>/datapack/internal/`. Both clients tested with `QT_QPA_PLATFORM=offscreen`: autosolo + multi against local `server-filedb`. Marker `MapVisualiserPlayer::mapDisplayedSlot()`. Produces NSIS .exe (or .zip fallback), .msi via WiX 3.11 under wine64, Authenticode signatures. MSI/signing at `/mnt/data/perso/progs/msi/`: `wix3/`, `test-codesign.pfx` (RSA-2048/SHA-256, password `catchchallenger`), `osslsigncode` 2.9. Timestamp `http://timestamp.digicert.com`; falls back untimestamped.

* **macOS** — osxcross container `root@2803:1920::2:ff08`, target `darwin20.4`, Qt 6.5.3 at `/root/qt6-macos/6.5.3/macos`. Setup: `/mnt/data/perso/lxc/osxcross.txt`. Sources rsynced to `/root/catchchallenger-test/`. cmake wrapper + ninja+ccache+lld (no mold for Mach-O). Then `macdeployqt`, datapack sibling of `.app`, ad-hoc `--sign -`. Portable `.zip`. Self-skips on ssh timeout. Compile+package only — no runtime/multi.

* **Android** — local Qt-for-Android cross-compile + local emulator. **Only `client/qtopengl`**. Tooling: `/mnt/data/perso/progs/CatchChallenger-android/{sdk,avd,apk,build}/`. Self-skips when VPS unreachable or SDK/adb/emulator/AVD missing.

## Diagnostic-tool runs — clang+sanitizer **and** gcc+valgrind

Two mutually-exclusive modes. Both wired into every `testing*.py` and `all.sh`, propagated to remote nodes.

1. **Clang + sanitizer** — `--sanitize asan|lsan|msan` — clang-built. ~2x slowdown.
2. **Gcc + valgrind** — `--valgrind memcheck|helgrind|drd` — gcc debug under valgrind. 10-50x slowdown.

`test/diagnostic.py` shared helper.

### Clang + sanitizer
* `asan` — `-fsanitize=address,undefined`. Default-and-broadest.
* `lsan` — `-fsanitize=leak`. Only leaks at exit.
* `msan` — `-fsanitize=memory,undefined -fsanitize-memory-track-origins=2`. False-positives on uninstrumented deps. Targeted use.

All add `-fno-omit-frame-pointer -fno-sanitize-recover=all -O1`. Env: `*_OPTIONS` with `halt_on_error=1`, `abort_on_error=1` (asan/msan), `exitcode=23` (lsan). Build dirs: `build-sanitize-<tool>/`, `build-remote-sanitize-<tool>/`.

### Gcc + valgrind
* `memcheck` — `--leak-check=full --show-leak-kinds=all --track-origins=yes --errors-for-leak-kinds=definite,possible`.
* `helgrind` — lock-ordering. EventLoop server is single-threaded so should be no-op.
* `drd` — alternative race detector.

Scales every timeout by **10x**. `--sanitize` and `--valgrind` mutually exclusive.

### Remote nodes
Two opt-in layers:
* **Compile**: `"compilers": ["gcc"]` or `["gcc","clang"]`. `--sanitize` requires `clang`; `--valgrind` requires `gcc`. Missing compiler = skip.
* **Execution** (mandatory booleans): `sanitizer_gcc` (true opts into `--valgrind`), `sanitizer_clang` (true opts into `--sanitize`). Effective only if compile node has matching compiler.

Both default `false` on new nodes. Operator must verify (`valgrind` or `lib*san` runtime). Harness won't auto-install. Layers AND together.

## Network — test box ↔ remote / execution nodes

Test box IPv6: `2803:1920::2:10/112`. Remote nodes in same `/112` (mips-lxc=`ff03`, x86-lxc=`ff04`, pentium-m=`ff01`, atom-n455=`ff02`).

Firewall (one-way for NEW):
* **NEW: test box → `2803:1920::2:/112` allowed.** OUTPUT chain accepts every dest.
* **NEW: `2803:1920::2:/112` → test box blocked.** Only RELATED/ESTABLISHED return packets pass.

**Remote/exec nodes are receive-only.** SSH keys only test box → remote (not reverse). Always drive ssh/rsync/staging from test box outward.

## Datapack staging cache

Solves dominant per-test cost (5-15s × dozens). Staged once at startup, symlinked thereafter.

* **Local** — `/mnt/data/perso/tmpfs/cc-datapack/<datapack-id>/` (ID = source basename). tmpfs RAM-speed. Persistent; `all.sh` excludes from per-run wipe.
* **Remote** — per-exec-node `datapack_cache` in `remote_nodes.json` (default `/home/user/datapack-cache/`). Layout `<datapack_cache>/<datapack-id>/...`. NEVER wiped — `--delete` keeps in sync.

`test/stage_datapacks.py` runs once at `all.sh` startup (after tmpfs cleanup, before testing*.py). Every datapack from `~/.config/catchchallenger-testing/config.json:paths.datapacks` rsynced (`-art --delete`) in parallel: one local + one ssh-rsync per exec_node × per datapack. Blocks until done. Failure → `[ABORT]`. Direction always test-box → remote.

Use:
* Local — `os.symlink(datapack_stage.staged_local(src), <build>/datapack)` instead of `shutil.copytree`. ~5-15s → ~1ms.
* Remote — `ssh: mkdir -p <work> && rm -rf <work>/datapack && ln -sfn <datapack_cache>/<id> <work>/datapack`.
* Maincode pruning dropped — cache shared, mustn't mutate. server-properties.xml's `mainDatapackCode` selects which maincode loads.

Must copy (not symlink) when destination is mutated: `setup_client_cache_partial` (testinghttp); per-player XDG cache dirs (testingmulti); cross-compile harnesses (symlinks across emulation flaky).

Modules: `test/datapack_stage.py` (`staged_local`/`staged_remote`/`remote_cache_for`/`datapack_id`/`stage_all`); `test/stage_datapacks.py` (one-shot driver); `test/all.sh` wipes tmpfs except `cc-datapack/`+`ccache-catchchallenger/`.

## Per-child resource limits — CPU + memory

Every testing*.py that spawns long-lived server binaries should set two `resource.setrlimit` caps in the child's `preexec_fn`:

* **`RLIMIT_CPU = 15 min soft / 15 min+60s hard`** — counts CPU-seconds, not wall. An idle server in `epoll_wait`/`io_uring_wait_cqe` accrues ~zero, runs as long as the test needs. Only catches the "wedged 100% one core" pattern: tight retry loops, infinite kernel/user-space wakeup loops (missed read drain, EPOLLIN never cleared, recv_multishot re-armed under back-pressure). 14-hour orphan motivated this. 15 min sits well inside the per-script wall cap (2h) and is small enough to coexist with other tenants on a shared VPS.
* **`RLIMIT_AS = 128 MiB hard`** — caps total virtual memory. Catches unbounded alloc patterns (recursive buffer regrowth, leaked alloc inside retry, parser eating gigabytes on malformed input). Kernel returns `-ENOMEM` from mmap; glibc converts to `std::bad_alloc`; the unhandled exception lands a clean abort stack the wall-watchdog can collect. 128 MiB fits master+login (each ~50 MiB peak) and gsa with max-players=2000 (~100 MiB after datapack map-in); larger workloads (testingbots crowd-burst, multi-thousand-monster fights) must raise the cap explicitly.

**Gate on `--valgrind` being absent.** Valgrind/Memcheck/Helgrind/DRD run binaries with 10-20× their normal memory footprint and 10-50× CPU. Applying these limits under valgrind kills legitimate workloads. testingcluster.py + testingbyIA.py never invoke their spawned children under valgrind so they apply both limits unconditionally. testing*.py that DO run their children under valgrind (the diagnostic-tool path via `diagnostic.runtime_wrapper(DIAG)`) must wrap the `resource.setrlimit` calls with `if DIAG.valgrind_tool is None`.

If a script legitimately needs more than 2 GiB or 2 h CPU per child, raise the per-script cap explicitly — don't quietly raise the shared default. The point is bug detection, not capacity headroom.

## Event-rate tripwire — `CATCHCHALLENGER_TESTING_LIMIT_EVENT_RATE`

CMake option, OFF by default. When ON, `EventLoop::wait()` aborts the process the moment it delivers more than **1000 events/s** through the dispatcher (sliding 1 s window). The wall-watchdog catches the abort via `PR_SET_PDEATHSIG`/atexit and the test rig captures a full backtrace.

The point is to detect tight kernel ↔ user-space wakeup loops *before* they burn a CPU core for hours — the same class of bug as the 14 h `catchchallenger-server-cli` orphan that motivated the watchdog work. Symptoms it catches:
* EPOLLIN never cleared (missed `read()` drain)
* `epoll_wait` returning -1/EBADF in a tight loop
* `io_uring` multishot re-armed inside the CQE drain without backoff
* level-triggered fd you forgot to mask after a partial read

Use it in every `testing*.py` whose workload has **no** legitimate burst:
* `testingcluster.py` — light, 2 clients per variant → safe to pin ON.
* `testingbyIA.py` — adversarial fuzz, sends bursts on purpose → leave OFF.
* `testingbots.py`, `testingmulti.py`, `testinghttp.py` (DDOS section) — high-volume by design → leave OFF.
* New testing*.py → default to ON unless the workload deliberately floods.

To enable: pass `-DCATCHCHALLENGER_TESTING_LIMIT_EVENT_RATE=ON` to cmake configure for every server binary the harness builds. Wired binary-side in `server/cli/EventLoop.cpp::wait()`; threshold is hard-coded at 1000/s, change there + in this doc together.

Production deploys MUST NOT define this. Leaving it OFF (the default) makes it a zero-cost no-op (compiled out, no `clock_gettime` per wait).

## Per-script wall limit — table-driven, capped per workload

Each `testing*.py` has its own wall-time ceiling sized roughly to "twice the longest healthy observed run" — generous enough that the natural run never trips the cap, tight enough that a wedged bug surfaces as a `[TIMEOUT]` instead of soaking the build. all.sh enforces from outside; the script itself enforces from inside.

**Caps (kept in lock-step between `test/all.sh:PER_TEST_TIMEOUT_MAP` and `_WALL_LIMIT_SEC` in the script — bumping a number is a deliberate one-line change in both):**

| script                          | cap   |
|---------------------------------|-------|
| testingbots.py                  | 15 min |
| testingbyIA.py                  | 30 min |
| testingclient.py                | 30 min |
| testingcluster.py               | 10 min |
| testingcmake.py                 | 30 min |
| testingcompilationandroid.py    | 15 min |
| testingcompilationmac.py        | 15 min |
| testingcompilationwindows.py    | 15 min |
| testingfight.py                 | 15 min |
| testinggateway.py               | 15 min |
| testinghttp.py                  | 15 min |
| testingmap2png.py               | 15 min |
| testingmap4client.py            | 30 min |
| testingmulti.py                 | 30 min |
| testingqtserver.py              | 15 min |
| testingremote.py                | 45 min |
| testingserver.py                | 30 min |
| testingstats.py                 | 10 min |
| testingtools.py                 | 15 min |
| testingwebsocket.py             | 30 min |

Default for any script not listed: 30 min (set by `DEFAULT_PER_TEST_TIMEOUT` in `all.sh`).

* **Outside (all.sh)** — `run_test()` wraps each script in `timeout --kill-after=30s <cap> python3 ...`. Exit code `124` (`timeout` SIGTERM) or `137` (SIGKILL after the grace) is reported as `[TIMEOUT]` and counted as a failure.
* **Inside (Python)** — `faulthandler.enable()` + `faulthandler.dump_traceback_later(WALL_LIMIT_SEC+10, exit=False)` prints a thread-by-thread Python traceback right around when `timeout` is about to fire, so the console captures *where* the script was stuck before SIGTERM lands. `faulthandler.register(signal.SIGUSR1)` is also armed — operator can `kill -USR1 <pid>` for a live traceback without killing the script.

When a script implements its own watchdog (testingcluster.py does), it should:
1. Log `[FAIL] wall-watchdog WALL_LIMIT_SEC exceeded`.
2. Dump `faulthandler.dump_traceback(file=sys.stdout, all_threads=True)` — shows where Python itself is stuck.
3. For every live subprocess it spawned, run `gdb -batch -ex 'thread apply all bt 40' -p <pid>` — shows where the C++ side is stuck.
4. `os._exit(3)` so the wrapper sees a distinct exit code and orphans get reaped by parent-death.

When a NEW testing*.py is added, copy the import block from testingserver.py or testingcluster.py (`faulthandler.enable()` + `dump_traceback_later`); don't roll your own. If a script legitimately needs more time than its cap, FIX the bug — don't widen the cap.

## Console + per-script timing logs

all.sh writes two operator-facing logs every run (truncated on each fresh run, kept across `--continue`):

* **`/mnt/data/perso/tmpfs/all.log`** — the full console output mirrored from the run, via `exec > >(tee -a ...)`. Lets the operator scroll back the entire run from disk after the terminal scrollback has aged out or when a 2 h run was unattended.
* **`/mnt/data/perso/tmpfs/testing-individual-time.json`** — a JSON array of `{"script": ..., "duration_s": ..., "rc": ...}` entries, one per testing*.py that COMPLETED (PASS or FAIL with non-timeout rc). Timed-out scripts (rc 124 / 137) are deliberately omitted: their "duration" is just the cap, not a measure of the test's natural runtime, and feeding that back as historical data would slowly inflate every cap that touched a flaky script. Use this to spot caps that are now far too generous (script consistently finishes in 1/5 of its cap → tighten) or far too tight (script regularly takes 90% of its cap → widen with a separate diagnostic before raising).

## Diagnosing a hung process — gdb attach, then decide

When stuck, NOT `pkill -9` first — `gdb -batch -p <pid> -ex "thread apply all bt 60" -ex detach -ex quit` to see what it's doing.

| Top of stack | Diagnosis | Action |
|---|---|---|
| `epoll_wait`/`poll`/`select`/`recvmsg`/`accept`/`read` | Idle, blocked on I/O. **Not a hang.** | Detach. Test timeout too tight or peer never sent expected packet. |
| `pthread_cond_wait`/`futex_wait` | Blocked on event-loop/mutex/condvar | Inspect siblings; if other thread waits on related lock, real deadlock. |
| `sqlite3_step`/`pq_get_result`/`mysql_real_query` | Waiting on SQL daemon | Detach. Check daemon. |
| `__libc_*`/`malloc_consolidate`/`arena_…` | Heap corruption | Capture bt, kill, look at recent allocator changes. |
| Tight loop in our code | Infinite loop | Real bug. Capture, kill, fix. |
| All `??` | Stripped/missing debug | Rebuild Debug, retry. |

Only "infinite loop" justifies kill. Variants: LXC use `ssh root@<lxc-ip> gdb -batch -p <container-pid>`. Under valgrind/sanitizer: skip (wrappers produce better trace). Solo/autosolo client: Qt event loop shows `QEventLoop::exec`/`epoll_wait` (idle, not hang). Don't use `strace -p` — gives syscall, not stack.

## Image-comparison tolerance (testingmap2png.py / testingmap4client.py)

Same two rules — keep in sync:
* **Per-pixel tolerance — ±10% per channel.** Pixel "different" only when at least one R/G/B/A differs by `> 0.10 * ref`. Reference 0 = new must be exactly 0.
* **Per-image budget — 0 pixels.** ANY failing pixel fails the test. Fix the source (pin rand, freeze animation, lock GL state) — don't widen budget.

FAIL line: `<failed>/<total> pixels diff > 10% (<pct>%) [hint]; first fail @ (x,y) channel <C>: new=<n> ref=<r>; diff mask saved to <path>`. Mask: `/mnt/data/perso/tmpfs/fail-map4client.png` or `/mnt/data/perso/tmpfs/fail.png` — black on white.

**Diff-percentage triage** (in `[hint]`):
* **`<10%`** → real renderer / save-state / RNG-seed bug. Investigate the source path that produced them.
* **`10-40%`** → ambiguous. Camera scrolled by one tile, different sprite frame, partial UI clip. Read the diff mask AND check savegame state.
* **`>40%`** → almost always **wrong datapack or wrong `--main-datapack-code`**. Verify launch flags match those used to capture reference PNG before chasing source regressions.

## Bisecting a failing test (testingbisect.py)

`paths.git_repo` is source repo; `paths.bisect_work` (default `<tmpfs_root>/bisec`) is per-commit scratch. Walks last `--max-commits` (default 20) newest→oldest, materialises each via `git archive`, overlays *current* test/ scripts (so harness fixes don't pollute target), runs `--script <name>` until `--test` fails (≤`--runs-per-commit` retries) or passes them all. Stops at first GOOD after BAD; prints last-BAD/first-GOOD. Flat-list walk friendlier than `git bisect` with flaky failures.

## Port probing — local only when server is local

* `cmd_helpers.assert_port_or_fail_with_remotes(...)` ssh's into exec_node and runs `bash /dev/tcp/<test-box>/<port>` — ALWAYS fails when server on test box (blocked direction). Use remote probe ONLY when target IS the exec_node (testingremote.py exec phase).
* Server on test box (testingserver/bots/multi/http): only LOCAL probe (`127.0.0.1:port`) meaningful.
* Never expect remote node to dial out. Bidirectional? Drive both halves from test box (ssh tunnels, or client local vs server remote).

## Optimising the global test wall time — `test/` only

Goal: cut total `test/all.sh` runtime **without** dropping cases or relaxing tolerances. Wins come from **inside `test/`** (parallelism, caching, skipping idle waits).

**Observability** — two JSONL logs survive `all.sh` (kept across per-run tmpfs wipe; truncated only on no-`--continue` fresh run):
* `<tmpfs>/time.json` — per-phase wall clock by `test/phase_timer.py`. `log_info`/`log_pass`/`log_fail` each land a record; bracket blocks with `with phase_timer.phase("name"):`.
* `<tmpfs>/monitor.json` — host CPU/RAM/load/net/per-CC-process snapshots, sampled by `test/monitor.py` every 60s; all.sh manages sampler via EXIT trap.

Both appended under `O_APPEND + flock(LOCK_EX)`. `failed.json` lock-guarded with temp-file + atomic rename.

**Decision rule:**
* Phase dominates AND `cpu_pct.idle > 70` → host **idle waiting** (SSH RTT, sleep, peer). Fix: parallelise (kick off next independent phase, fan per-node loop across threads — `testingremote.py` thread pool is the template).
* Phase dominates AND ONE core pinned → **single-threaded hot path**. Speed via more `-j` / different tool / cache reuse, NOT extra threads.
* Phase dominates AND every core pinned → saturated; only algorithmic/caching wins help.

**Out of bounds:** no deletion of test cases; no widening image tolerances; no skipping slow nodes; no new `time.sleep(N)` (replace with event/poll on actual signal); no mock-swapping; new parallelism must be JSON-safe (`flock`) and lock-safe (`failed_cases._Locked`, `phase_timer` flock); compile stays under `nice -n 19 ionice -c 3`.

**Post-run review** after every full `all.sh` (or `--continue` round once green): top-N slowest phases (`jq -r 'select(.kind=="phase") | "\(.dt) \(.name)"' <tmpfs>/time.json | sort -rn | head -20`); cross-check `monitor.json`. Simple low-risk wins (parallelise loop, swap sleep for poll, cache recomputed value, add `-j$(nproc)`, drop redundant `rsync --checksum`) land same session. Touching test cases/assertions/tolerances/ports/features → STOP, ask operator.
