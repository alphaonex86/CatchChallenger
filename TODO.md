# CatchChallenger TODO

## Migrate the build system from qmake to cmake

Concrete advantages **for CatchChallenger specifically**, ranked by impact:

1. **Drop the 8 `make*.py` scripts.** They exist because qmake-make is slow
   on big targets and we wanted parallel per-file compile.
   `cmake -G Ninja` + `ninja -j$(nproc)` does that natively, with proper
   dependency tracking, no shell-out, no Python overhead.  The
   `make*.py` overhaul work and the `resolvCtoO` / state-aware
   incremental-rebuild work in `test/qmake_helpers.py` would all become
   unnecessary.

2. **Vendored libs we already have are CMake-native.**
   `client/libqtcatchchallenger/libogg/CMakeLists.txt`,
   `libopus/CMakeLists.txt`, `libopusfile/CMakeLists.txt`,
   `general/libzstd/CMakeLists.txt` all exist.  Right now `.pri` files
   manually re-state their headers/sources/defines; with cmake we would
   just `add_subdirectory(libogg)` and link `target_link_libraries(t
   PRIVATE ogg)`.  Less duplication, no risk of `.pri` and upstream
   `CMakeLists.txt` drifting.

3. **Cross-compile is the documented path.** Qt-Android docs are 100%
   cmake (we already have `qt-cmake` ready under
   `/mnt/data/perso/progs/CatchChallenger-android/`); the MXE Windows
   wrapper `x86_64-w64-mingw32.shared-cmake` is a one-line invocation;
   macOS uses `qt-cmake`.  Currently we juggle `qmake -spec
   android-clang`, `qmake -spec macx-clang`, `qmake -spec linux-g++`,
   MXE qmake — cmake unifies these behind toolchain files.

4. **Faster rebuilds.** The state-aware incremental work we just shipped
   (`prepare_qmake_build_dir` etc.) approximates what cmake+ninja gives
   for free, and ninja's dep graph catches more (header transitive
   deps, generated moc/uic/rcc, etc.).  Typically 2–4× faster
   incremental builds vs qmake+make on a tree this size.

5. **Long-term Qt direction.** Qt 6 is cmake-first; Qt 7 will likely drop
   qmake.  Existing qmake stays maintained but gets no new features.
   Not urgent for CatchChallenger — qmake works — but it is a one-way
   risk.

6. **Better testing-harness integration.** `ctest` lets us wire the
   `testing*.py` scripts as ctest tests; `cmake -D OPTION=ON` replaces
   the powerset-of-DEFINES dance more cleanly than `flag_combinations()`.
   We already have a flag-combo system, so this is "nice", not
   transformative.

7. **IDE / ccache / clang-tidy / sanitizer support is more mature on
   cmake.**  All three sanitizer modes we support today plug in via
   `-DCMAKE_C_FLAGS=-fsanitize=…` automatically.

What we would **lose**:

- `CONFIG +=` is slightly nicer than cmake's `option()` + `if()` for a
  couple patterns (debug/release toggles).
- Engineers familiar with qmake have a real (small) cmake learning
  curve.
- A few weeks of conversion work + the risk of subtle behavior changes
  during the migration.

### Bottom line

The biggest practical win is killing the 8 `make*.py` scripts and the
cross-build complexity.  If we ship releases on Android / Windows /
macOS regularly, cmake pays for itself fast.  If qmake builds suit our
dev cadence and the `make*.py` scripts are stable, the migration cost
may not be worth the disruption right now — keep qmake until something
else (Qt 7 dropping qmake, or a new platform target) forces the move.

### Migration plan (when we decide to do it)

Inventory: 25 `.pro`, 32 `.pri`, 8 `make*.py`.  Conversion is target-
by-target rather than one big bang:

1. Convert one simple leaf target as a proof of concept
   (e.g. `tools/stats/stats.pro`).  Build it with `cmake -G Ninja`,
   compare its `-D` flags + sources list against the qmake build via
   the existing `compare_qmake_make` infrastructure (which would be
   adapted to `compare_qmake_cmake`).
2. Once the sample target is green, convert in waves: server-side
   targets first (`gateway`, `login`, `master`, `game-server-alone`,
   `epoll/*`), then tools, then clients (the qtopengl /
   libqtcatchchallenger trees with their vendored libs are the
   riskiest, so they go last).
3. Adapt `test/build_project()` (in all 5 testing scripts) to invoke
   `cmake --build` instead of `qmake && make`.
4. Delete `.pro`, `.pri`, and `make*.py` only after the test suite
   passes end-to-end on cmake.
5. Update `CLAUDE.md` to reflect the new build system, drop the
   make*.py / resolvCtoO sections.
