"""
cmake_helpers.py — Phase 5 of the qmake -> CMake migration.

Shared helpers for the testing*.py orchestrators. Public surface:

  pro_to_cmake_target(pro_rel)
      -> (cmake_target_name, configure_flags, output_subdir) given a
         .pro path. The configure_flags are the per-target -D switches
         that turn the relevant component on; output_subdir is where
         CMake puts the binary inside the build tree.

  prepare_cmake_build_dir(build_dir, compiler, cxx_version, extra_defines, root)
      -> str: "full" | "incremental"
         Decide whether the build_dir needs to be wiped (compiler or
         C++ standard changed) or whether incremental rebuild is OK.

  cmake_extra_args(extra_defines)
      -> list of cmake -D args derived from EXTRA_DEFINES style macros.

  build_project(pro_file, build_dir, label, ...)
      -> bool. Runs `cmake -S ROOT -B build_dir ...` then
         `cmake --build build_dir --target <name>`. Replaces every
         testing*.py's per-file `build_project` qmake driver. The
         binary is copied into build_dir/<bin_name> so callers can
         locate it via os.path.join(build_dir, bin_name) like before.
"""
import os
import re
import subprocess
import shutil

# Keep this in sync with cmake_make_helper._OPTION_MACROS.
_OPTION_MACROS = {
    "CATCHCHALLENGER_NOXML",
    "CATCHCHALLENGER_HARDENED",
    "CATCHCHALLENGER_DUMP_DATATREE_CACHE_DATAPACK",
    "CATCHCHALLENGER_SERVER_DATAPACK_ONLYBYMIRROR",
    "CATCHCHALLENGER_IO_URING",
    "CATCHCHALLENGER_POLL",
    "CATCHCHALLENGER_EXTRA_CHECK",
    "CATCHCHALLENGER_CACHE_HPS",
    "CATCHCHALLENGER_DB_FILE",
}

_PRO_TO_CMAKE = {
    "server/epoll/catchchallenger-server-filedb.pro": (
        "catchchallenger-server-cli-epoll",
        ["-DCATCHCHALLENGER_DB_FILE=ON",
         "-DCATCHCHALLENGER_BUILD_EPOLL_FILEDB=ON"],
        "server/epoll",
    ),
    "server/epoll/catchchallenger-server-cli-epoll.pro": (
        "catchchallenger-server-cli-epoll",
        ["-DCATCHCHALLENGER_DB_FILE=ON",
         "-DCATCHCHALLENGER_BUILD_EPOLL_FILEDB=ON"],
        "server/epoll",
    ),
    "server/epoll/catchchallenger-server-test.pro": (
        "catchchallenger-server-cli-epoll",
        ["-DCATCHCHALLENGER_DB_FILE=ON",
         "-DCATCHCHALLENGER_BUILD_EPOLL_FILEDB=ON"],
        "server/epoll",
    ),
    "server/login/login.pro": (
        "catchchallenger-server-login",
        ["-DCATCHCHALLENGER_BUILD_LOGIN=ON"],
        "server/login",
    ),
    "server/master/master.pro": (
        "catchchallenger-server-master",
        ["-DCATCHCHALLENGER_BUILD_MASTER=ON"],
        "server/master",
    ),
    "server/gateway/gateway.pro": (
        "catchchallenger-gateway",
        ["-DCATCHCHALLENGER_BUILD_GATEWAY=ON"],
        "server/gateway",
    ),
    "server/game-server-alone/game-server-alone.pro": (
        "catchchallenger-game-server-alone",
        ["-DCATCHCHALLENGER_BUILD_GAME_SERVER_ALONE=ON"],
        "server/game-server-alone",
    ),
    "server/epoll/filedb-converter/filedb-converter.pro": (
        "filedb-converter",
        ["-DCATCHCHALLENGER_BUILD_TOOL_FILEDB_CONVERTER=ON"],
        "server/epoll/filedb-converter",
    ),
    "tools/stats/stats.pro": (
        "stats",
        ["-DCATCHCHALLENGER_BUILD_TOOL_STATS=ON"],
        "tools/stats",
    ),
    "tools/map2png/map2png.pro": (
        "map2png",
        ["-DCATCHCHALLENGER_BUILD_TOOL_MAP2PNG=ON"],
        "tools/map2png",
    ),
    "tools/tmxTileLayerConverterToZstd/tmxTileLayerConverterToZstd.pro": (
        "tmxTileLayerConverterToZstd",
        ["-DCATCHCHALLENGER_BUILD_TOOL_TMX_LAYER_CONVERTER=ON"],
        "tools/tmxTileLayerConverterToZstd",
    ),
    "tools/bot-test-connect-to-gameserver-cli/bot-test-connect-to-gameserver.pro": (
        "bot-test-connect-to-gameserver",
        ["-DCATCHCHALLENGER_BUILD_TOOL_BOT_TEST_CONNECT=ON"],
        "tools/bot-test-connect-to-gameserver-cli",
    ),
    "tools/datapack-explorer-generator-cli/datapack-explorer-generator.pro": (
        "datapack-explorer-generator",
        ["-DCATCHCHALLENGER_BUILD_TOOL_DATAPACK_EXPLORER=ON"],
        "tools/datapack-explorer-generator-cli",
    ),
    "tools/map-procedural-generation-terrain/map-procedural-generation-terrain.pro": (
        "map-procedural-generation-terrain",
        ["-DCATCHCHALLENGER_BUILD_TOOL_MAP_PROCGEN_TERRAIN=ON"],
        "tools/map-procedural-generation-terrain",
    ),
    "tools/map-procedural-generation/map-procedural-generation.pro": (
        "map-procedural-generation",
        ["-DCATCHCHALLENGER_BUILD_TOOL_MAP_PROCGEN=ON"],
        "tools/map-procedural-generation",
    ),
    "tools/datapack-downloader-cli/datapack-downloader.pro": (
        "datapack-downloader",
        ["-DCATCHCHALLENGER_BUILD_TOOL_DATAPACK_DOWNLOADER=ON"],
        "tools/datapack-downloader-cli",
    ),
    "tools/bot-actions/bot-actions.pro": (
        "bot-actions",
        ["-DCATCHCHALLENGER_BUILD_TOOL_BOT_ACTIONS=ON"],
        "tools/bot-actions",
    ),
    "client/qtcpu800x600/qtcpu800x600.pro": (
        "catchchallenger800x600",
        ["-DCATCHCHALLENGER_BUILD_CLIENT_QTCPU800X600=ON"],
        "client/qtcpu800x600",
    ),
    "client/qtopengl/catchchallenger-qtopengl.pro": (
        "catchchallenger",
        ["-DCATCHCHALLENGER_BUILD_CLIENT_QTOPENGL=ON"],
        "client/qtopengl",
    ),
    "client/catchchallenger.pro": (
        "catchchallenger",
        ["-DCATCHCHALLENGER_BUILD_CLIENT_QTOPENGL=ON"],
        "client/qtopengl",
    ),
}


def pro_to_cmake_target(pro_rel):
    """Map a .pro path (relative to repo root, forward slashes) to
    (cmake_target, configure_flags, output_subdir). Raises KeyError if
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
    incrementally. Returns "full" or "incremental".

    Caller should clear the build_dir (or at least re-run cmake) when
    "full" is returned. CMake itself handles incremental rebuild fine
    once configured — this hook exists so EXTRA_DEFINES changes still
    invalidate the right .o files (CMake re-configures, and target
    compile flags pick up the new defines)."""
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

    os.makedirs(build_dir, exist_ok=True)
    try:
        with open(state_path, "w") as f:
            f.write(f"compiler={cur_compiler}\n")
            f.write(f"std={cur_std}\n")
            f.write(f"defines={cur_defines}\n")
    except IOError:
        pass

    if abi_changed:
        return "full"
    return "incremental"


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
        target, configure_flags, output_subdir = pro_to_cmake_target(pro_rel)
    except KeyError:
        log_fail(name, f"no cmake target mapping for {pro_rel}")
        return False

    decision = prepare_cmake_build_dir(build_dir, comp_short,
                                       cxx_version, extra_defines, root)
    if decision == "full" or clean_first:
        log_info(f"compiler/std changed → wipe {label}")
        shutil.rmtree(build_dir, ignore_errors=True)
        ensure_dir(build_dir)

    cmake_args = [
        "cmake", "-S", root, "-B", build_dir,
        f"-DCMAKE_C_COMPILER={cc_bin}",
        f"-DCMAKE_CXX_COMPILER={cxx_bin}",
        "-DCMAKE_BUILD_TYPE=Debug",
        "-DCMAKE_EXE_LINKER_FLAGS=-fuse-ld=mold",
    ]
    cmake_args.extend(configure_flags)
    cmake_args.extend(cmake_extra_args(extra_defines))
    if cxx_version:
        m = re.match(r"(?:c\+\+|gnu\+\+)?(\d+)", cxx_version)
        if m:
            cmake_args.append(f"-DCMAKE_CXX_STANDARD={m.group(1)}")
    if extra_qmake_args:
        for a in extra_qmake_args:
            if a == "CONFIG+=catchchallenger_poll":
                cmake_args.append("-DCATCHCHALLENGER_POLL=ON")
            elif a == "CONFIG+=catchchallenger_io_uring":
                cmake_args.append("-DCATCHCHALLENGER_IO_URING=ON")
            # other qmake-only CONFIG+= flags are intentionally dropped

    log_info(f"cmake configure {label}")
    rc, out = run_cmd(cmake_args, build_dir)
    if rc != 0:
        log_fail(name, f"cmake configure failed (rc={rc})")
        if out.strip():
            print(out[-2000:])
        return False
    log_info(f"cmake --build -j{nproc} {label}")
    rc, out = run_cmd(["cmake", "--build", build_dir,
                       "--target", target, "-j", str(nproc)], build_dir)
    if rc != 0:
        log_fail(name, f"cmake build failed (rc={rc})")
        if out.strip():
            print(out[-3000:])
        return False

    # Copy the binary to build_dir/<bin> so downstream code that does
    # os.path.join(build_dir, bin_name) finds it (matches qmake era).
    src_bin = os.path.join(build_dir, output_subdir, target)
    dst_bin = os.path.join(build_dir, target)
    if os.path.isfile(src_bin) and src_bin != dst_bin:
        try:
            shutil.copy2(src_bin, dst_bin)
        except OSError as e:
            log_fail(name, f"binary copy failed: {e}")
            return False
    log_pass(name)
    return True
