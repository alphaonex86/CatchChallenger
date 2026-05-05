"""Single loader for test-side machine-local paths.

The test harness runs on a personal workstation where the operator stores
big build artefacts on a tmpfs, MXE under /mnt/data, etc. Hard-coding any
of those paths in the repo would leak the operator's filesystem layout
into a public git history, so every machine-local path lives in
~/.config/catchchallenger-testing/config.json instead. This module is
the only place the JSON keys are translated into Python constants.

Required `paths.*` keys:
  tmpfs_root         scratch dir for build outputs, screenshots,
                     failed.json, ccache, datapack stage cache.
  mxe_prefix         dedicated MXE install (testingcompilationwindows).
  msi_dir            WiX 3.11 + signing cert (testingcompilationwindows).
  android_workspace  SDK / AVD / APK staging (testingcompilationandroid).
  remote_nodes_json  full path to the JSON describing remote compile/exec
                     nodes; lives outside the repo so personal SSH hosts
                     and rootfs paths don't leak into git history.

Derived constants exported here:
  TMPFS_ROOT         <paths.tmpfs_root>
  TMPFS_BUILD_ROOT   <tmpfs_root>/cc-build
  LOCAL_CACHE_ROOT   <tmpfs_root>/cc-datapack
  CCACHE_ROOT        <tmpfs_root>/ccache
  FAILED_JSON        <tmpfs_root>/failed.json
  PYCACHE_DIR        <tmpfs_root>/cc-build/__pycache__
  MAP2PNG_OUTPUT     <tmpfs_root>/catchchallenger-map2png.png
  MAP2PNG_DIFF       <tmpfs_root>/fail.png
  SCREENSHOT_OUTPUT  <tmpfs_root>/screenshot.png
  SCREENSHOT_DIFF    <tmpfs_root>/fail-map4client.png
  MXE_PREFIX         <paths.mxe_prefix>
  MSI_DIR            <paths.msi_dir>
  ANDROID_WORKSPACE  <paths.android_workspace>

The config is read lazily on first attribute access so importing the
module on a node where the file is missing (e.g. an exec_node where we
only run a binary) doesn't crash. Anything that actually NEEDS a path
will trip a clear error then.
"""
import json, os

CONFIG_PATH = os.path.expanduser("~/.config/catchchallenger-testing/config.json")

# Dummy template written when the operator runs the test harness for
# the first time and the config file does not yet exist. Every value is
# a placeholder the operator MUST replace — paths point at /tmp scratch
# dirs and obviously-fake checkout locations so a typo'd run fails
# loudly instead of silently writing tons of files into a wrong tree.
_DUMMY_CONFIG = {
    "qmake": "qmake6",
    "httpDatapackMirror": "http://example.invalid/mirror/",
    "server_host": "localhost",
    "server_port": 61917,
    "paths": {
        "datapacks": [
            "/path/to/CatchChallenger-datapack",
            "/path/to/datapack-pkmn"
        ],
        "www_root": "/path/to/www-root",
        "savegame_cpu": "~/.local/share/CatchChallenger/client-qtcpu800x600/savegames",
        "savegame_gl": "~/.local/share/CatchChallenger/client/solo",
        "client_datapack_cache": "~/.local/share/CatchChallenger/client-qtcpu800x600/datapack",
        "tmpfs_root": "/tmp/cc-test",
        "mxe_prefix": "/path/to/mxe",
        "msi_dir": "/path/to/msi-tooling",
        "android_workspace": "/path/to/android-workspace",
        "remote_nodes_json": "/path/to/remote_nodes.json"
    },
    "postgresql": {
        "database": "catchchallenger",
        "user": "postgres"
    }
}


def _ensure_config_file():
    """Create CONFIG_PATH with dummy values when missing so the harness
    has something to read. The dummy values WILL fail at first use —
    that is the point: a fresh checkout shouldn't silently inherit some
    other operator's real paths. The operator edits this file once."""
    if os.path.isfile(CONFIG_PATH):
        return
    os.makedirs(os.path.dirname(CONFIG_PATH), exist_ok=True)
    with open(CONFIG_PATH, "w") as f:
        json.dump(_DUMMY_CONFIG, f, indent=4)
        f.write("\n")
    print(f"test_config: wrote dummy {CONFIG_PATH} — edit it before running tests",
          file=__import__("sys").stderr)


_cfg_cache = None

def _load():
    global _cfg_cache
    if _cfg_cache is None:
        _ensure_config_file()
        with open(CONFIG_PATH, "r") as f:
            _cfg_cache = json.load(f)
    return _cfg_cache

def _path(key):
    cfg = _load()
    paths = cfg.get("paths", {})
    if key not in paths:
        raise KeyError(
            f"missing paths.{key} in {CONFIG_PATH} — see test_config.py docstring")
    return os.path.expanduser(paths[key])


def __getattr__(name):
    """Lazy module-level attribute lookup (PEP 562). Avoids reading the
    JSON at import time, which keeps `import test_config` cheap even
    when the file is missing."""
    if name == "TMPFS_ROOT":
        return _path("tmpfs_root")
    if name == "TMPFS_BUILD_ROOT":
        return os.path.join(_path("tmpfs_root"), "cc-build")
    if name == "LOCAL_CACHE_ROOT":
        return os.path.join(_path("tmpfs_root"), "cc-datapack")
    if name == "CCACHE_ROOT":
        return os.path.join(_path("tmpfs_root"), "ccache")
    if name == "FAILED_JSON":
        return os.path.join(_path("tmpfs_root"), "failed.json")
    if name == "PYCACHE_DIR":
        return os.path.join(_path("tmpfs_root"), "cc-build", "__pycache__")
    if name == "MAP2PNG_OUTPUT":
        return os.path.join(_path("tmpfs_root"), "catchchallenger-map2png.png")
    if name == "MAP2PNG_DIFF":
        return os.path.join(_path("tmpfs_root"), "fail.png")
    if name == "SCREENSHOT_OUTPUT":
        return os.path.join(_path("tmpfs_root"), "screenshot.png")
    if name == "SCREENSHOT_DIFF":
        return os.path.join(_path("tmpfs_root"), "fail-map4client.png")
    if name == "MXE_PREFIX":
        return _path("mxe_prefix")
    if name == "MSI_DIR":
        return _path("msi_dir")
    if name == "ANDROID_WORKSPACE":
        return _path("android_workspace")
    if name == "REMOTE_NODES_JSON":
        return _path("remote_nodes_json")
    raise AttributeError(f"module 'test_config' has no attribute {name!r}")


# ── datapack checksum cache (used by datapack_stage.py) ────────────────────
# Stored in config.json under `paths.datapack_checksums` as a {src_path:
# checksum} dict. On `stage_all()` we compare the freshly-computed source
# checksum against the stored one; on a match every rsync (local + each
# remote LXC) is SKIPPED, which on a no-change all.sh start saves the
# multi-hundred-MB host→LXC SSH transfer entirely. Persisted in config so
# the cache survives across sessions.

_CHECKSUM_LOCK = __import__("threading").Lock()


def get_datapack_checksum(src_path):
    """Return the cached checksum for `src_path`, or "" if none recorded."""
    cfg = _load()
    table = cfg.get("paths", {}).get("datapack_checksums") or {}
    return table.get(src_path, "")


def set_datapack_checksum(src_path, checksum):
    """Persist `checksum` for `src_path` to config.json. Read-modify-write
    serialised across stage_all worker threads via _CHECKSUM_LOCK."""
    global _cfg_cache
    with _CHECKSUM_LOCK:
        # Re-read from disk so two parallel writers don't lose each
        # other's update — the threaded stage_all calls us once per
        # successfully-staged source.
        with open(CONFIG_PATH, "r") as f:
            cfg = json.load(f)
        paths = cfg.setdefault("paths", {})
        table = paths.setdefault("datapack_checksums", {})
        table[src_path] = checksum
        with open(CONFIG_PATH, "w") as f:
            json.dump(cfg, f, indent=4)
            f.write("\n")
        # Refresh the in-process cache so a subsequent get_ in the same
        # process sees the new value.
        _cfg_cache = cfg
