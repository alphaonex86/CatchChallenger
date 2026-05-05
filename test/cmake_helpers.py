"""
cmake_helpers.py — shared helpers for the testing*.py orchestrators.
Phase 5 of the qmake -> CMake migration; later updated for the
"one binary per CMakeLists.txt" layout (no root project).

Public surface:

  pro_to_cmake_target(pro_rel)
      -> (cmake_target_name, configure_flags, source_subdir) given a
         .pro path. `source_subdir` is the directory we point cmake -S
         at (each binary is its own self-contained CMake project under
         the new layout). `configure_flags` are functional toggles only
         (CATCHCHALLENGER_DB_FILE etc.) — gating flags are no longer
         needed since standalone subdirs have no aggregator.

  prepare_cmake_build_dir(build_dir, compiler, cxx_version, extra_defines, root)
      -> str: "full" | "incremental"

  cmake_extra_args(extra_defines)
      -> list of cmake -D args derived from EXTRA_DEFINES style macros.

  build_project(pro_file, build_dir, label, ...)
      -> bool. Runs `cmake -S <root>/<source_subdir> -B build_dir …` then
         `cmake --build build_dir --target <name>`. The produced binary
         lands at build_dir/<target> directly (no output_subdir copy).
"""
import os
import re
import subprocess
import shutil


def _ninja_or_default_generator_args():
    """Return ['-G', 'Ninja'] when the SYSTEM ninja-build binary is
    present, [] otherwise. The system ninja (/usr/bin/ninja, the real
    C++ binary) is the one we want — pip's `ninja` package installs a
    python shim at ~/.local/bin/ninja that lives in front of /usr/bin
    on PATH and hands stdin to a subprocess; cmake's invocation goes
    through the shim with extra latency and breaks under some env
    setups. shutil.which() would pick the shim first, so probe
    /usr/bin/ninja directly and only fall back to PATH lookup if the
    distro put ninja somewhere else.

    Ninja's content-hash dep tracking + tighter parallel scheduling is
    a clear win over make for incremental rebuilds (header edit
    triggers ~3 .o files instead of ~30). When neither path resolves,
    the empty list lets cmake pick its host default — Unix Makefiles
    on Linux."""
    if os.path.exists("/usr/bin/ninja"):
        return ["-G", "Ninja", "-DCMAKE_MAKE_PROGRAM=/usr/bin/ninja"]
    found = shutil.which("ninja")
    if found and not found.startswith(os.path.expanduser("~/.local/")):
        return ["-G", "Ninja", f"-DCMAKE_MAKE_PROGRAM={found}"]
    return []

# Keep this in sync with cmake_make_helper._OPTION_MACROS.
_OPTION_MACROS = {
    "CATCHCHALLENGER_NOXML",
    "CATCHCHALLENGER_HARDENED",
    "CATCHCHALLENGER_DUMP_DATATREE_CACHE_DATAPACK",
    "CATCHCHALLENGER_SERVER_DATAPACK_ONLYBYMIRROR",
    "CATCHCHALLENGER_IO_URING",
    "CATCHCHALLENGER_POLL",
    "CATCHCHALLENGER_SELECT",
    "CATCHCHALLENGER_CACHE_HPS",
    "CATCHCHALLENGER_DB_FILE",
    # NOAUDIO must be a CMake option (not a raw CMAKE_CXX_FLAGS define)
    # because libqtcatchchallenger's `if(CATCHCHALLENGER_NOAUDIO)` block
    # gates target_compile_definitions(... PUBLIC CATCHCHALLENGER_NOAUDIO).
    # Target compile definitions propagate to AUTOMOC; raw -D flags via
    # CMAKE_CXX_FLAGS do not. Without this the Q_OBJECT-tagged
    # audioLoopRestart() slot in BaseWindow.h is parsed by moc (which
    # never sees NOAUDIO) and emitted into moc_BaseWindow.cpp, then the
    # compiler (which DOES see NOAUDIO) rejects the moc'd definition
    # because the slot's declaration is #ifndef'd out.
    "CATCHCHALLENGER_NOAUDIO",
    # SINGLEPLAYER toggles aren't C++ defines — they're CMake options that
    # pull in server/qt + Qt6::Sql + QFakeSocket so --autosolo can spin
    # up an in-process server. The qmake era built that path
    # unconditionally; CMake made it opt-in. Recognise them as option
    # macros so testing*.py can request them via extra_defines.
    "CATCHCHALLENGER_BUILD_QTOPENGL_SINGLEPLAYER",
    "CATCHCHALLENGER_BUILD_QTCPU800X600_SINGLEPLAYER",
}

# _PRO_TO_CMAKE: legacy .pro path -> (cmake_target, configure_flags, source_subdir).
#
# After the "one binary per CMakeLists.txt" refactor (no root /CMakeLists.txt),
# each binary is its own self-contained CMake project. `source_subdir` is the
# directory we point `cmake -S` at; the binary lands at `build_dir/<target>`.
#
# `configure_flags` only carries FUNCTIONAL toggles that change the binary's
# behaviour (e.g. CATCHCHALLENGER_DB_FILE). We no longer pass the old
# CATCHCHALLENGER_BUILD_<X>=ON gating flags — those existed solely to gate
# the root project's add_subdirectory calls; standalone subdirs have no such
# gate.
_PRO_TO_CMAKE = {
    "server/epoll/catchchallenger-server-filedb.pro": (
        "catchchallenger-server-cli-epoll",
        ["-DCATCHCHALLENGER_DB_FILE=ON"],
        "server/epoll",
    ),
    "server/epoll/catchchallenger-server-cli-epoll.pro": (
        "catchchallenger-server-cli-epoll",
        ["-DCATCHCHALLENGER_DB_FILE=ON"],
        "server/epoll",
    ),
    "server/epoll/catchchallenger-server-test.pro": (
        "catchchallenger-server-cli-epoll",
        ["-DCATCHCHALLENGER_DB_FILE=ON"],
        "server/epoll",
    ),
    "server/login/login.pro": (
        "catchchallenger-server-login",
        [],
        "server/login",
    ),
    "server/master/master.pro": (
        "catchchallenger-server-master",
        [],
        "server/master",
    ),
    "server/gateway/gateway.pro": (
        "catchchallenger-gateway",
        [],
        "server/gateway",
    ),
    "server/game-server-alone/game-server-alone.pro": (
        "catchchallenger-game-server-alone",
        [],
        "server/game-server-alone",
    ),
    "server/epoll/filedb-converter/filedb-converter.pro": (
        "filedb-converter",
        [],
        "server/epoll/filedb-converter",
    ),
    "tools/stats/stats.pro": (
        "stats",
        [],
        "tools/stats",
    ),
    "tools/map2png/map2png.pro": (
        "map2png",
        [],
        "tools/map2png",
    ),
    "tools/tmxTileLayerConverterToZstd/tmxTileLayerConverterToZstd.pro": (
        "tmxTileLayerConverterToZstd",
        [],
        "tools/tmxTileLayerConverterToZstd",
    ),
    "tools/bot-test-connect-to-gameserver-cli/bot-test-connect-to-gameserver.pro": (
        "bot-test-connect-to-gameserver",
        [],
        "tools/bot-test-connect-to-gameserver-cli",
    ),
    "tools/datapack-explorer-generator-cli/datapack-explorer-generator.pro": (
        "datapack-explorer-generator",
        [],
        "tools/datapack-explorer-generator-cli",
    ),
    "tools/map-procedural-generation-terrain/map-procedural-generation-terrain.pro": (
        "map-procedural-generation-terrain",
        [],
        "tools/map-procedural-generation-terrain",
    ),
    "tools/map-procedural-generation/map-procedural-generation.pro": (
        "map-procedural-generation",
        [],
        "tools/map-procedural-generation",
    ),
    "tools/datapack-downloader-cli/datapack-downloader.pro": (
        "datapack-downloader",
        [],
        "tools/datapack-downloader-cli",
    ),
    "tools/bot-actions/bot-actions.pro": (
        "bot-actions",
        [],
        "tools/bot-actions",
    ),
    "client/qtcpu800x600/qtcpu800x600.pro": (
        "catchchallenger800x600",
        [],
        "client/qtcpu800x600",
    ),
    # qtopengl moved up to client/CMakeLists.txt; client/qtopengl/ is now
    # source-only (no CMakeLists.txt). Both legacy .pro paths point at
    # the new client/ project.
    "client/qtopengl/catchchallenger-qtopengl.pro": (
        "catchchallenger",
        [],
        "client",
    ),
    "client/catchchallenger.pro": (
        "catchchallenger",
        [],
        "client",
    ),
}


def pro_to_cmake_target(pro_rel):
    """Map a .pro path (relative to repo root, forward slashes) to
    (cmake_target, configure_flags, source_subdir). Raises KeyError if
    the .pro isn't in the table — add it here when migrating a new
    test target."""
    return _PRO_TO_CMAKE[pro_rel.replace("\\", "/")]


def classify_extra_defines(extra_defines):
    """Split an EXTRA_DEFINES-style list of macro names into:
      cmake_opts: -DCATCHCHALLENGER_FOO=ON for known options
      flag_defs: -DFOO entries (raw compile defines for the rest).
    Mirrors cmake_make_helper._classify_extra_defines."""
    if not extra_defines:
        return [], []
    cmake_opts = []
    flag_defs = []
    for macro in extra_defines:
        if "=" in macro:
            flag_defs.append("-D" + macro)
            continue
        if macro in _OPTION_MACROS:
            cmake_opts.append(f"-D{macro}=ON")
        else:
            flag_defs.append("-D" + macro)
    return cmake_opts, flag_defs


def cmake_extra_args(extra_defines):
    """Build a flat list of cmake -D arguments from an EXTRA_DEFINES list.
    Raw defines that don't map to a CMake option are appended to
    CMAKE_CXX_FLAGS / CMAKE_C_FLAGS in a single -D each."""
    cmake_opts, flag_defs = classify_extra_defines(extra_defines)
    args = list(cmake_opts)
    if flag_defs:
        joined = " ".join(flag_defs)
        args.append(f"-DCMAKE_CXX_FLAGS={joined}")
        args.append(f"-DCMAKE_C_FLAGS={joined}")
    return args


# State file mirrors qmake_helpers' .qh_make_state.txt — drives the
# "full vs incremental" decision when compiler or C++ standard changes.
_STATE_FILE = ".cc_cmake_state.txt"


def prepare_cmake_build_dir(build_dir, compiler, cxx_version, extra_defines, root):
    """Decide whether `build_dir` needs a fresh configure (compiler or
    C++ standard changed) or whether the existing build can be reused
    incrementally. Returns ("full" | "incremental", drop_options) where
    drop_options is the list of `_OPTION_MACROS` that were ON in the
    previous configure but aren't requested now — caller passes them as
    `-D<MACRO>=OFF` so CMake's cached value gets cleared.

    Why drop_options: CMake retains cache values across re-configures,
    so once `extra_defines=["CATCHCHALLENGER_NOXML"]` flips
    CATCHCHALLENGER_NOXML=ON in the cache, a subsequent
    `extra_defines=None` configure inherits NOXML=ON and the resulting
    binary aborts with "CATCHCHALLENGER_NOXML defined but no binary
    cache available". Tracking previously-on options here lets the
    build dir stay reusable (no rmtree) while still propagating define
    flips correctly. Options that weren't toggled before keep their
    CMakeLists.txt defaults (HARDENED=ON, CACHE_HPS=ON), which is what
    qmake-era tests relied on."""
    state_path = os.path.join(build_dir, _STATE_FILE)
    last_compiler = ""
    last_std = ""
    last_defines = ""
    if os.path.isfile(state_path):
        try:
            with open(state_path, "r") as f:
                for line in f:
                    line = line.strip()
                    if "=" not in line:
                        continue
                    k, v = line.split("=", 1)
                    if k == "compiler":
                        last_compiler = v
                    elif k == "std":
                        last_std = v
                    elif k == "defines":
                        last_defines = v
        except IOError:
            pass

    cur_compiler = compiler or ""
    cur_std = cxx_version or ""
    cur_defines = " ".join(sorted(extra_defines or []))

    abi_changed = ((last_compiler != "" and last_compiler != cur_compiler) or
                   (last_std != "" and last_std != cur_std))

    last_set = set(last_defines.split()) if last_defines else set()
    cur_set = set(extra_defines or [])
    drop_options = sorted(
        (last_set - cur_set) & _OPTION_MACROS
    )

    os.makedirs(build_dir, exist_ok=True)
    try:
        with open(state_path, "w") as f:
            f.write(f"compiler={cur_compiler}\n")
            f.write(f"std={cur_std}\n")
            f.write(f"defines={cur_defines}\n")
    except IOError:
        pass

    if abi_changed:
        return "full", drop_options
    return "incremental", drop_options


# ── shared build_project for the testing*.py orchestrators ──────────────

def _spec_to_compiler(compiler_spec):
    """Translate a qmake compiler spec (linux-g++, linux-clang, etc.)
    to (cc_bin, cxx_bin, comp_short)."""
    if compiler_spec and "clang" in compiler_spec:
        return "clang", "clang++", "clang"
    return "gcc", "g++", "gcc"


def build_project(pro_file, build_dir, label, *,
                  root,
                  nproc,
                  log_info,
                  log_pass,
                  log_fail,
                  ensure_dir,
                  run_cmd,
                  diag=None,
                  compiler_spec="linux-g++",
                  extra_defines=None,
                  clean_first=False,
                  cxx_version=None,
                  extra_qmake_args=None,
                  diag_module=None):
    """Drive cmake configure + build for a target identified by its
    legacy .pro path. Returns True on success. The keyword-only
    callbacks (log_info / log_pass / log_fail / ensure_dir / run_cmd)
    let each testing*.py keep its own logging style.

    The function intentionally accepts a `diag` and `diag_module` so
    sanitizer / valgrind builds can override the compiler spec — same
    behaviour the qmake era had via test/diagnostic.py.
    """
    ensure_dir(build_dir)
    name = f"compile {label}"

    if diag_module is not None:
        diag_spec = diag_module.compiler_spec(diag)
        if diag_spec:
            compiler_spec = diag_spec
    cc_bin, cxx_bin, comp_short = _spec_to_compiler(compiler_spec)

    pro_rel = os.path.relpath(pro_file, root).replace(os.sep, "/")
    try:
        target, configure_flags, source_subdir = pro_to_cmake_target(pro_rel)
    except KeyError:
        log_fail(name, f"no cmake target mapping for {pro_rel}")
        return False
    # Per "one binary per CMakeLists.txt" refactor: cmake -S points at the
    # binary's own subdir (which has its own project()), not the repo root.
    cmake_source = os.path.join(root, source_subdir)
    # CMake's OUTPUT_NAME property can rename the produced binary file
    # (qtcpu800x600 sets OUTPUT_NAME=catchchallenger so callers can keep
    # using the qmake-era TARGET=catchchallenger binary path). The
    # post-build copy below must look for the actual filename on disk,
    # not the cmake target name. Map known overrides explicitly here —
    # cheaper than parsing CMakeFiles/<target>.dir/build.make.
    _OUTPUT_NAME_OVERRIDES = {
        "catchchallenger800x600": "catchchallenger",
    }
    bin_filename = _OUTPUT_NAME_OVERRIDES.get(target, target)

    decision, drop_options = prepare_cmake_build_dir(
        build_dir, comp_short, cxx_version, extra_defines, root)
    # Only wipe on a real ABI change (compiler or C++ standard). CMake
    # re-configures idempotently when -D flags change, so a "clean_first"
    # request that only flips defines (NOXML on/off, EXTRA_DEFINES, etc.)
    # used to trigger a full from-scratch rebuild for nothing — that was
    # the qmake-era cleanliness pattern. Keep clean_first as a hint and
    # honour it only when the dir doesn't exist yet (or via cmake's own
    # --clean-first below); never rmtree just because a define changed.
    if decision == "full":
        log_info(f"compiler/std changed → wipe {label}")
        shutil.rmtree(build_dir, ignore_errors=True)
        ensure_dir(build_dir)
        # State file is gone after rmtree; nothing to drop on a fresh
        # build dir, CMake reads its defaults from the option() calls.
        drop_options = []

    # -fno-lto on every test build: link-time optimisation slows compile
    # by 5-10x and the lto-wrapper invocations that fan out from a single
    # `ld` call confuse our build-load sampling + nice-prefix bookkeeping.
    # Tests don't need LTO's runtime-perf gains, so disable it everywhere
    # via CMAKE_C/CXX_FLAGS_INIT (preserves any per-target additions) and
    # CMAKE_*_LINKER_FLAGS_INIT. Same convention is mirrored in
    # remote_build.py and every cross-compile testing*.py.
    cmake_args = ["cmake"] + _ninja_or_default_generator_args() + [
        "-S", cmake_source, "-B", build_dir,
        f"-DCMAKE_C_COMPILER={cc_bin}",
        f"-DCMAKE_CXX_COMPILER={cxx_bin}",
        "-DCMAKE_BUILD_TYPE=Debug",
        "-DCMAKE_EXE_LINKER_FLAGS=-fuse-ld=mold -fno-lto",
        "-DCMAKE_SHARED_LINKER_FLAGS=-fno-lto",
        "-DCMAKE_MODULE_LINKER_FLAGS=-fno-lto",
        "-DCMAKE_C_FLAGS_INIT=-fno-lto",
        "-DCMAKE_CXX_FLAGS_INIT=-fno-lto",
    ]
    cmake_args.extend(configure_flags)
    cmake_args.extend(cmake_extra_args(extra_defines))
    # Force-OFF any option macro that was ON in the previous configure
    # but isn't requested now. See prepare_cmake_build_dir's docstring
    # for the rationale (CMake cache retention across re-configures).
    di = 0
    while di < len(drop_options):
        cmake_args.append(f"-D{drop_options[di]}=OFF")
        di += 1
    if cxx_version:
        m = re.match(r"(?:c\+\+|gnu\+\+)?(\d+)", cxx_version)
        if m:
            cmake_args.append(f"-DCMAKE_CXX_STANDARD={m.group(1)}")
    if extra_qmake_args:
        for a in extra_qmake_args:
            if a == "CONFIG+=catchchallenger_select":
                cmake_args.append("-DCATCHCHALLENGER_SELECT=ON")
                cmake_args.append("-DCATCHCHALLENGER_IO_URING=OFF")
            elif a == "CONFIG+=catchchallenger_poll":
                cmake_args.append("-DCATCHCHALLENGER_POLL=ON")
                cmake_args.append("-DCATCHCHALLENGER_IO_URING=OFF")
            elif a == "CONFIG+=catchchallenger_io_uring":
                cmake_args.append("-DCATCHCHALLENGER_IO_URING=ON")
            elif a == "CONFIG+=catchchallenger_force_epoll":
                # IO_URING is the default-ON Linux backend; force-pick
                # plain epoll for the matrix's "epoll" entry by
                # disabling all the alternative backends.
                cmake_args.append("-DCATCHCHALLENGER_SELECT=OFF")
                cmake_args.append("-DCATCHCHALLENGER_POLL=OFF")
                cmake_args.append("-DCATCHCHALLENGER_IO_URING=OFF")
            # other qmake-only CONFIG+= flags are intentionally dropped

    # Bound each cmake invocation so a hung configure / runaway link
    # can't stall the test suite indefinitely. Defaults match the
    # per-remote-node fields in remote_nodes.json so local and remote
    # share the same numbers:
    #   cmake_configure_timeout = 180s (find_package + try_compile etc.)
    #   cmake_compile_timeout   = 600s (full target build)
    # Diagnostic builds (--sanitize / --valgrind) scale via
    # diag_module.scale_timeout() so valgrind's ~10x slowdown still fits.
    base_cfg_timeout = 180
    base_build_timeout = 600
    if diag_module is not None and hasattr(diag_module, "scale_timeout"):
        cfg_timeout = diag_module.scale_timeout(diag, base_cfg_timeout)
        build_timeout = diag_module.scale_timeout(diag, base_build_timeout)
    else:
        cfg_timeout = base_cfg_timeout
        build_timeout = base_build_timeout

    log_info(f"cmake configure {label} (timeout {cfg_timeout}s)")
    rc, out = run_cmd(cmake_args, build_dir, timeout=cfg_timeout)
    # CMake's CheckCSourceCompiles (used by FindThreads etc.) writes a
    # scratch src.c then runs a child cmake on it; with concurrent rsync /
    # parallel remote builds touching the host, the scratch file
    # occasionally gets a transient "Cannot find source file: src.c" race.
    # The configure is fully reproducible on retry, so wipe the scratch
    # tree and try one more time before declaring failure. Only retry on
    # the exact symptom — other errors (missing dep, syntax error) stay
    # fatal so we don't paper over real bugs.
    retry_marker = ("Cannot find source file" in (out or ""))
    if rc != 0 and retry_marker:
        scratch = os.path.join(build_dir, "CMakeFiles", "CMakeScratch")
        if os.path.isdir(scratch):
            shutil.rmtree(scratch, ignore_errors=True)
        log_info(f"cmake configure {label}: transient TryCompile race, "
                 f"retrying once after wiping CMakeScratch/")
        rc, out = run_cmd(cmake_args, build_dir, timeout=cfg_timeout)
    if rc != 0:
        log_fail(name, f"cmake configure failed (rc={rc})")
        if out.strip():
            print(out[-2000:])
        return False
    log_info(f"cmake --build -j{nproc} {label} (timeout {build_timeout}s)")
    build_cmd = ["cmake", "--build", build_dir,
                 "--target", target, "-j", str(nproc)]
    if clean_first:
        # Force re-link of <target> via cmake's own --clean-first instead
        # of an rmtree. Only the affected target's objects get cleaned;
        # the configure cache and unrelated targets stay warm.
        build_cmd.append("--clean-first")
    rc, out = run_cmd(build_cmd, build_dir, timeout=build_timeout)
    if rc != 0:
        log_fail(name, f"cmake build failed (rc={rc})")
        if out.strip():
            print(out[-3000:])
        return False

    # With self-contained per-binary CMakeLists.txt, the executable lands
    # at build_dir/<target> directly (each binary's CMakeLists.txt is the
    # project root, so add_executable puts the binary in CMAKE_BINARY_DIR
    # which == build_dir). bin_filename respects OUTPUT_NAME (e.g.
    # qtcpu800x600's target is "catchchallenger800x600" but the produced
    # binary is "catchchallenger").
    log_pass(name)
    return True
