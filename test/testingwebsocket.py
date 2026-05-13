#!/usr/bin/env python3
"""
testingwebsocket.py — CatchChallenger WebSocket transport test.

Pipeline:
  1. Build catchchallenger-server-cli (file-db, IO_URING backend).
  2. Build qtcpu800x600 + qtopengl with their WebSockets options ON.
  3. Start nginx (config in test/nginx-websocket.conf.in) serving the
     CatchChallenger-datapack as a static HTTP datapack mirror.
  4. Start the server on a raw-TCP port.
  5. Start `websockify` (dev-python/websockify) as a separate process,
     forwarding ws://...:<wsport> to the server's TCP port.
  6. Run each client with `--url ws://127.0.0.1:<wsport>/`, expect the
     "MapVisualiserPlayer::mapDisplayedSlot()" success marker.

The nginx config lives at test/nginx-websocket.conf.in and can be
edited freely; only @PREFIX@/@WWW_ROOT@/@NGINX_PORT@ are substituted at
runtime by this script.
"""
import sys
import process_helpers
sys.dont_write_bytecode = True

import os, sys, signal, subprocess, threading, multiprocessing, json
import shutil, time, socket

import diagnostic
import build_paths
from cmd_helpers import clamp_local, assert_port_or_fail


build_paths.ensure_root()

DIAG = diagnostic.parse_diag_args()
_DIAG_SUFFIX = diagnostic.build_dir_suffix(DIAG)

_CONFIG_PATH = os.path.join(os.path.expanduser("~"),
                            ".config", "catchchallenger-testing", "config.json")
with open(_CONFIG_PATH, "r") as _f:
    _config = json.load(_f)

ROOT  = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
NPROC = str(multiprocessing.cpu_count())

DATAPACK_SRC = "/home/user/Desktop/CatchChallenger/CatchChallenger-datapack"
MAINCODE     = "test"

SERVER_PRO     = os.path.join(ROOT, "server/cli/catchchallenger-server-filedb.pro")
CLIENT_CPU_PRO = os.path.join(ROOT, "client/qtcpu800x600/qtcpu800x600.pro")
CLIENT_GL_PRO  = os.path.join(ROOT, "client/catchchallenger.pro")

SERVER_BUILD     = build_paths.build_path("server/cli/build/testing-ws-server"  + _DIAG_SUFFIX)
CLIENT_CPU_BUILD = build_paths.build_path("client/qtcpu800x600/build/testing-ws-cpu" + _DIAG_SUFFIX)
CLIENT_GL_BUILD  = build_paths.build_path("client/build/testing-ws-gl"            + _DIAG_SUFFIX)

SERVER_BIN = "catchchallenger-server-cli"
CLIENT_CPU_BIN = "catchchallenger"
CLIENT_GL_BIN  = "catchchallenger"

# Ports — picked to avoid collision with other testing*.py defaults.
TCP_HOST       = "127.0.0.1"
TCP_PORT       = 61920    # server raw-TCP listen
WS_PORT        = 61921    # websockify listen (clients dial ws://:WS_PORT/)
NGINX_PORT     = 8766     # nginx HTTP datapack mirror

from test_config import TMPFS_ROOT
NGINX_PREFIX = os.path.join(TMPFS_ROOT, "cc-nginx-websocket")
NGINX_WWW    = os.path.join(NGINX_PREFIX, "www")
# The catchchallenger mirror lives under /datapack/ on the nginx site —
# leaves / free for unrelated website content. The mirror itself
# contains pack/, datapack-list/ and a datapack/ subdir of files.
MIRROR_DIR   = os.path.join(NGINX_WWW, "datapack")
NGINX_CONF   = os.path.join(NGINX_PREFIX, "nginx.conf")
NGINX_TPL    = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                            "nginx-websocket.conf.in")

COMPILE_TIMEOUT      = 600
SERVER_READY_TIMEOUT = 60
# qtopengl needs more wall-clock than qtcpu800x600 — it spins up audio,
# image, and OpenGL paths in parallel before reaching the first datapack
# fetch. 180s easily covers cold-start full-download + map load.
CLIENT_TIMEOUT       = 180
NGINX_READY_TIMEOUT  = 5
WS_READY_TIMEOUT     = 5

# Per-client cache directories. Layout follows
# `argument-<host-or-url>` and the client that wrote it. We need these
# absolute paths to set up the empty / partial / same scenarios.
CLIENT_CPU_CACHE_DIR = os.path.expanduser(
    "~/.local/share/CatchChallenger/client-qtcpu800x600/datapack/argument-ws:/127.0.0.1:61921")
CLIENT_GL_CACHE_DIR = os.path.expanduser(
    "~/.local/share/CatchChallenger/client/datapack/argument-ws:/127.0.0.1:61921")

NICE_PREFIX = ["nice", "-n", "19", "ionice", "-c", "3"]

C_GREEN  = "\033[92m"
C_RED    = "\033[91m"
C_CYAN   = "\033[96m"
C_RESET  = "\033[0m"

results = []
total_expected = [0]
_last_log_time = [time.monotonic()]
server_proc    = None
nginx_proc     = None
websockify_proc = None

SCRIPT_NAME = os.path.basename(__file__)
import failed_cases as _fc
import phase_timer


def load_failed_cases():
    return _fc.load_names(SCRIPT_NAME)


def should_run(test_name, failed_cases):
    if failed_cases is None:
        return True
    return test_name in failed_cases


def save_failed_cases():
    failed = []
    for name, ok, detail, _elapsed in results:
        if not ok:
            d = _fc.make_detail(detail)
            d.update(_fc.pop_extras(name))
            failed.append((name, d))
    _fc.save(SCRIPT_NAME, failed)


def log_info(msg):
    print(f"{phase_timer.t()} {C_CYAN}[INFO]{C_RESET} {msg}")


def log_pass(name, detail=""):
    now = time.monotonic()
    elapsed = now - _last_log_time[0]
    _last_log_time[0] = now
    results.append((name, True, detail, elapsed))
    if len(results) > total_expected[0]:
        total_expected[0] = len(results)
    print(f"{phase_timer.t()} {C_GREEN}[PASS]{C_RESET} {len(results)}/{total_expected[0]} "
          f"{name}  {detail}  ({elapsed:.1f}s)")
    phase_timer.record_event("pass", name, ok=True, dt=elapsed, detail=detail)


def log_fail(name, detail=""):
    now = time.monotonic()
    elapsed = now - _last_log_time[0]
    _last_log_time[0] = now
    results.append((name, False, detail, elapsed))
    if len(results) > total_expected[0]:
        total_expected[0] = len(results)
    print(f"{phase_timer.t()} {C_RED}[FAIL]{C_RESET} {len(results)}/{total_expected[0]} "
          f"{name}  {detail}  ({elapsed:.1f}s)")
    phase_timer.record_event("fail", name, ok=False, dt=elapsed, detail=detail)
    li = 0
    _ctx = diagnostic.last_cmd_lines()
    while li < len(_ctx):
        print(_ctx[li])
        li += 1


def ensure_dir(path):
    os.makedirs(path, exist_ok=True)


def run_cmd(args, cwd, timeout=COMPILE_TIMEOUT, env=None):
    timeout = clamp_local(timeout)
    diagnostic.record_cmd(NICE_PREFIX + list(args), cwd)
    try:
        p = subprocess.run(NICE_PREFIX + list(args), cwd=cwd, timeout=timeout,
                           stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                           env=env or os.environ)
        return p.returncode, p.stdout.decode(errors="replace")
    except subprocess.TimeoutExpired:
        return -1, f"TIMEOUT after {timeout}s"


def build_project(pro_file, build_dir, label, extra_defines=None,
                  extra_qmake_args=None):
    import cmake_helpers as _ch
    return _ch.build_project(
        pro_file, build_dir, label,
        root=ROOT, nproc=NPROC,
        log_info=log_info, log_pass=log_pass, log_fail=log_fail,
        ensure_dir=ensure_dir, run_cmd=run_cmd,
        diag=DIAG, diag_module=diagnostic,
        extra_defines=extra_defines,
        extra_qmake_args=extra_qmake_args,
    )


# ── nginx ───────────────────────────────────────────────────────────────────
def write_nginx_conf():
    ensure_dir(NGINX_PREFIX)
    ensure_dir(NGINX_WWW)
    with open(NGINX_TPL, "r") as f:
        tpl = f.read()
    body = (tpl
            .replace("@PREFIX@", NGINX_PREFIX)
            .replace("@WWW_ROOT@", NGINX_WWW)
            .replace("@NGINX_PORT@", str(NGINX_PORT)))
    with open(NGINX_CONF, "w") as f:
        f.write(body)


def stage_nginx_www(datapack_path):
    """Stage the datapack files DIRECTLY under MIRROR_DIR (not under a
    nested datapack/ subdir): for each top-level entry of the source
    that's not repo metadata, create a symlink in MIRROR_DIR. The
    layout MUST be flat because the client's per-file mirror URL
    `${httpDatapackMirror}<file>` (== `http://.../datapack/<file>`)
    resolves to NGINX_WWW/datapack/<file> via nginx's `root` directive,
    so the file has to sit at MIRROR_DIR/<file> — NOT
    MIRROR_DIR/datapack/<file>. With per-entry symlinks, `pack/` and
    `datapack-list/` (real dirs created later) live as siblings."""
    ensure_dir(NGINX_WWW)
    # Wipe and re-create MIRROR_DIR so stale entries from a prior run
    # (different datapack, leftover symlinks) don't bleed in.
    if os.path.exists(MIRROR_DIR) or os.path.islink(MIRROR_DIR):
        if os.path.islink(MIRROR_DIR) or os.path.isfile(MIRROR_DIR):
            os.remove(MIRROR_DIR)
        else:
            shutil.rmtree(MIRROR_DIR, ignore_errors=True)
    ensure_dir(MIRROR_DIR)
    excludes = {".git", ".gitignore", ".gitattributes", ".github",
                ".claude", ".vscode", "__pycache__"}
    for entry in os.listdir(datapack_path):
        if entry in excludes:
            continue
        os.symlink(os.path.join(datapack_path, entry),
                   os.path.join(MIRROR_DIR, entry))


WWW_ROOT_SRC = os.path.expanduser(_config["paths"]["www_root"])


def prepare_http_mirror():
    """Generate pack/*.tar.zst and datapack-list/*.txt under NGINX_WWW
    so the clients' http datapack mirror requests
    (pack/datapack.tar.zst, datapack-list/base.txt, …) actually
    resolve. Reuses datapack-archive.sh + datapack-list.php from the
    operator-configured www_root — both scripts are cwd-relative so we
    copy them next to our staged datapack symlink and run them there.

    Without this step the client logs:
      "datapack-list/base.txt - server replied: Not Found"
      "Unable to download the datapack"
    and refuses to fall back to in-protocol download.
    """
    archive_sh = os.path.join(WWW_ROOT_SRC, "datapack-archive.sh")
    list_php   = os.path.join(WWW_ROOT_SRC, "datapack-list.php")
    if not os.path.isfile(archive_sh):
        log_fail("http mirror prep",
                 f"datapack-archive.sh missing in www_root: {archive_sh}")
        return False
    if not os.path.isfile(list_php):
        log_fail("http mirror prep",
                 f"datapack-list.php missing in www_root: {list_php}")
        return False

    # The archive script accepts <datapack> <output> args, so we point
    # it directly at MIRROR_DIR (the staged tree) and tell it to write
    # tarballs into MIRROR_DIR/pack/. Run BEFORE creating pack/ +
    # datapack-list/ siblings so find -L doesn't wander into the
    # output dirs (the script's own EXCLUDE handles `<dpname>/pack`).
    pack_dir = os.path.join(MIRROR_DIR, "pack")
    log_info(f"running datapack-archive.sh {MIRROR_DIR} {pack_dir}")
    rc, out = run_cmd(["bash", archive_sh, MIRROR_DIR, pack_dir],
                      NGINX_WWW, timeout=300)
    if rc != 0:
        log_fail("datapack-archive.sh", f"rc={rc}")
        for l in (out or "").splitlines()[-20:]:
            print(f"  | {l}")
        return False
    # Re-grant read perms in case archive.sh's chmod 600/700 hit any
    # writable targets. Safe + no-op when the source repo refused.
    run_cmd(["chmod", "-R", "a+rX", MIRROR_DIR], NGINX_WWW, timeout=30)
    copied = len([f for f in os.listdir(pack_dir) if f.endswith(".tar.zst")])
    log_info(f"wrote {copied} tar.zst file(s) into pack/")

    # datapack-list.php is cwd-relative — it scans `./datapack/`. With
    # MIRROR_DIR being NGINX_WWW/datapack/, running with cwd=NGINX_WWW
    # makes `./datapack/` resolve to our staged tree. Copy the script
    # to NGINX_WWW so it can be invoked there.
    dst_php = os.path.join(NGINX_WWW, "datapack-list.php")
    shutil.copy2(list_php, dst_php)

    # Generate datapack-list/{base,main-<mc>,sub-<mc>-<sc>}.txt by
    # running datapack-list.php with the documented arg shapes.
    list_dir = os.path.join(MIRROR_DIR, "datapack-list")
    ensure_dir(list_dir)

    def _run_php(args, out_path):
        cmd = NICE_PREFIX + ["php", "datapack-list.php"] + list(args)
        diagnostic.record_cmd(cmd, NGINX_WWW)
        try:
            p = subprocess.run(cmd, cwd=NGINX_WWW, timeout=120,
                               stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        except subprocess.TimeoutExpired:
            return False
        if p.returncode != 0:
            return False
        with open(out_path, "wb") as f:
            f.write(p.stdout)
        return True

    if not _run_php([], os.path.join(list_dir, "base.txt")):
        log_fail("datapack-list base.txt", "php datapack-list.php (no args) failed")
        return False
    map_main = os.path.join(MIRROR_DIR, "map", "main")
    if os.path.isdir(map_main):
        for mc in sorted(os.listdir(map_main)):
            mc_path = os.path.join(map_main, mc)
            if not os.path.isdir(mc_path):
                continue
            _run_php([f"main={mc}"],
                     os.path.join(list_dir, f"main-{mc}.txt"))
            sub_dir = os.path.join(mc_path, "sub")
            if os.path.isdir(sub_dir):
                for sc in sorted(os.listdir(sub_dir)):
                    if os.path.isdir(os.path.join(sub_dir, sc)):
                        _run_php([f"main={mc}", f"sub={sc}"],
                                 os.path.join(list_dir, f"sub-{mc}-{sc}.txt"))
    log_pass("http mirror prep",
             f"pack/={copied} files, datapack-list/={len(os.listdir(list_dir))} files")
    return True


def start_nginx():
    global nginx_proc
    if shutil.which("nginx") is None:
        log_fail("nginx start", "nginx binary not on PATH")
        return False
    write_nginx_conf()
    # -e overrides nginx's compiled-in error_log path (typically
    # /var/log/nginx/error.log, write-protected for non-root). The
    # in-conf `daemon off;` keeps nginx in the foreground so Popen owns
    # its lifetime; do NOT also pass `-g 'daemon off;'` — that would
    # make nginx complain about a duplicate directive.
    args = ["nginx", "-p", NGINX_PREFIX + "/", "-c", NGINX_CONF,
            "-e", os.path.join(NGINX_PREFIX, "early-error.log")]
    diagnostic.record_cmd(args, NGINX_PREFIX)
    log_info(f"starting nginx on :{NGINX_PORT}")
    nginx_proc = subprocess.Popen(
        args, cwd=NGINX_PREFIX,
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
        preexec_fn=process_helpers.setsid_and_pdeathsig)
    deadline = time.monotonic() + NGINX_READY_TIMEOUT
    while time.monotonic() < deadline:
        try:
            socket.create_connection(("127.0.0.1", NGINX_PORT), timeout=0.5).close()
            log_pass("nginx start", f":{NGINX_PORT} reachable")
            return True
        except OSError:
            if nginx_proc.poll() is not None:
                out = nginx_proc.stdout.read().decode(errors="replace")
                log_fail("nginx start", f"died early: {out[-300:]}")
                nginx_proc = None
                return False
            time.sleep(0.1)
    log_fail("nginx start", f":{NGINX_PORT} not reachable in {NGINX_READY_TIMEOUT}s")
    stop_nginx()
    return False


def stop_nginx():
    global nginx_proc
    if nginx_proc is None:
        return
    try:
        os.killpg(os.getpgid(nginx_proc.pid), signal.SIGTERM)
    except ProcessLookupError:
        pass
    try:
        nginx_proc.wait(timeout=5)
    except subprocess.TimeoutExpired:
        try:
            os.killpg(os.getpgid(nginx_proc.pid), signal.SIGKILL)
        except ProcessLookupError:
            pass
        nginx_proc.wait(timeout=2)
    nginx_proc = None


# ── websockify ──────────────────────────────────────────────────────────────
def start_websockify():
    global websockify_proc
    binary = shutil.which("websockify")
    if binary is None:
        log_fail("websockify start",
                 "websockify not on PATH (install dev-python/websockify)")
        return False
    args = [binary, str(WS_PORT), f"{TCP_HOST}:{TCP_PORT}"]
    diagnostic.record_cmd(args, NGINX_PREFIX)
    log_info(f"starting websockify ws://0.0.0.0:{WS_PORT}/ → {TCP_HOST}:{TCP_PORT}")
    websockify_proc = subprocess.Popen(
        args, cwd=NGINX_PREFIX,
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
        preexec_fn=process_helpers.setsid_and_pdeathsig)
    deadline = time.monotonic() + WS_READY_TIMEOUT
    while time.monotonic() < deadline:
        try:
            socket.create_connection(("127.0.0.1", WS_PORT), timeout=0.5).close()
            log_pass("websockify start", f":{WS_PORT} reachable")
            return True
        except OSError:
            if websockify_proc.poll() is not None:
                out = websockify_proc.stdout.read().decode(errors="replace")
                log_fail("websockify start", f"died early: {out[-300:]}")
                websockify_proc = None
                return False
            time.sleep(0.1)
    log_fail("websockify start",
             f":{WS_PORT} not reachable in {WS_READY_TIMEOUT}s")
    stop_websockify()
    return False


def stop_websockify():
    global websockify_proc
    if websockify_proc is None:
        return
    try:
        os.killpg(os.getpgid(websockify_proc.pid), signal.SIGTERM)
    except ProcessLookupError:
        pass
    try:
        websockify_proc.wait(timeout=5)
    except subprocess.TimeoutExpired:
        try:
            os.killpg(os.getpgid(websockify_proc.pid), signal.SIGKILL)
        except ProcessLookupError:
            pass
        websockify_proc.wait(timeout=2)
    websockify_proc = None


# ── server ──────────────────────────────────────────────────────────────────
def write_server_settings(http_url):
    """Pin server to TCP_PORT, autologin, mainDatapackCode=test, http mirror."""
    xml = os.path.join(SERVER_BUILD, "server-properties.xml")
    ensure_dir(SERVER_BUILD)
    body = (
        '<?xml version="1.0"?>\n'
        '<configuration>\n'
        '    <server-port value="{port}"/>\n'
        '    <automatic_account_creation value="true"/>\n'
        '    <max-players value="200"/>\n'
        '    <httpDatapackMirror value="{http}"/>\n'
        '    <master>\n'
        '        <external-server-port value="{port}"/>\n'
        '    </master>\n'
        '    <content>\n'
        '        <mainDatapackCode value="{mc}"/>\n'
        '        <subDatapackCode value=""/>\n'
        '    </content>\n'
        '</configuration>\n'
    ).format(port=TCP_PORT, http=http_url, mc=MAINCODE)
    with open(xml, "w") as f:
        f.write(body)


def stage_server_datapack():
    link = os.path.join(SERVER_BUILD, "datapack")
    if os.path.islink(link) or os.path.exists(link):
        try:
            os.remove(link)
        except OSError:
            shutil.rmtree(link, ignore_errors=True)
    os.symlink(DATAPACK_SRC, link)


def reset_server_state():
    cache = os.path.join(SERVER_BUILD, "datapack-cache.bin")
    if os.path.exists(cache):
        os.remove(cache)
    db = os.path.join(SERVER_BUILD, "database")
    if os.path.isdir(db):
        shutil.rmtree(db)


def start_server():
    global server_proc
    binary = os.path.join(SERVER_BUILD, SERVER_BIN)
    if not os.path.isfile(binary):
        log_fail("server start", f"binary missing: {binary}")
        return False
    env = os.environ.copy()
    for k, v in diagnostic.runtime_env(DIAG).items():
        env[k] = v
    wrapper = diagnostic.runtime_wrapper(DIAG)
    args = NICE_PREFIX + wrapper + [binary]
    diagnostic.record_cmd(args, SERVER_BUILD)
    log_info(f"starting server: {binary}")
    server_proc = subprocess.Popen(
        args, cwd=SERVER_BUILD,
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT, env=env,
        preexec_fn=process_helpers.setsid_and_pdeathsig)
    ready = threading.Event()
    output = []

    def reader():
        for raw in iter(server_proc.stdout.readline, b""):
            line = raw.decode(errors="replace").rstrip("\n")
            output.append(line)
            if "correctly bind:" in line:
                ready.set()
        ready.set()

    threading.Thread(target=reader, daemon=True).start()
    timeout = diagnostic.scale_timeout(DIAG, SERVER_READY_TIMEOUT)
    if ready.wait(timeout=timeout):
        log_pass("server start", "correctly bind: detected")
        return True
    log_fail("server start", f"timeout waiting for 'correctly bind:' ({timeout}s)")
    for l in output[-30:]:
        print(f"  | {l}")
    stop_server()
    return False


def stop_server():
    global server_proc
    if server_proc is None:
        return
    try:
        os.killpg(os.getpgid(server_proc.pid), signal.SIGTERM)
    except ProcessLookupError:
        pass
    try:
        server_proc.wait(timeout=10)
    except subprocess.TimeoutExpired:
        try:
            os.killpg(os.getpgid(server_proc.pid), signal.SIGKILL)
        except ProcessLookupError:
            pass
        server_proc.wait(timeout=5)
    server_proc = None


# ── cache scenarios ─────────────────────────────────────────────────────────
def nginx_access_log_lines():
    """Return current line count of nginx's access.log; 0 if missing."""
    p = os.path.join(NGINX_PREFIX, "access.log")
    if not os.path.isfile(p):
        return 0
    with open(p, "rb") as f:
        return sum(1 for _ in f)


def assert_no_redownload_since(label, baseline_lines):
    """Verify that no further requests to /datapack/pack/ or
    /datapack/datapack-list/ landed in the access log since
    `baseline_lines`. Per-file fetches under /datapack/<x> are still
    OK in some clients' info-fetch flows; the strict no-download check
    is just for the bulk endpoints."""
    p = os.path.join(NGINX_PREFIX, "access.log")
    if not os.path.isfile(p):
        log_fail(f"{label}: no-redownload",
                 "access.log missing, can't verify")
        return
    with open(p, "r", errors="replace") as f:
        lines = f.readlines()
    new = lines[baseline_lines:]
    bulk_hits = [l for l in new
                 if "/datapack/pack/" in l or "/datapack/datapack-list/" in l]
    if bulk_hits:
        log_fail(f"{label}: no-redownload",
                 f"{len(bulk_hits)} bulk-mirror hit(s) on a 'same' cache")
        for h in bulk_hits[:5]:
            print(f"  | {h.rstrip()}")
    else:
        log_pass(f"{label}: no-redownload",
                 "no pack/ or datapack-list/ fetched")


def setup_cache_empty(cache_dir):
    """Wipe the cache so the client must full-download from the http
    mirror. Also wipes the per-cache stored hash dir so the next
    connection's checksum compare reports "local list is empty"."""
    if os.path.exists(cache_dir):
        shutil.rmtree(cache_dir, ignore_errors=True)
    cache_bin = cache_dir + "-cache"
    if os.path.exists(cache_bin):
        shutil.rmtree(cache_bin, ignore_errors=True)
    log_info(f"cache empty: {cache_dir}")


def setup_cache_partial(cache_dir, src_datapack):
    """Stage cache_dir with a NEAR-complete copy of `src_datapack` —
    every allowed file present except one, plus an extra stale file
    that doesn't exist server-side. Forces the client through the
    diff-download code path (a small handful of per-file http fetches
    + one stale-removal) instead of the slow ~250-file mirror sweep
    that staging only-one-file would trigger.

    The cache-bin (stored hashes) dir is also wiped so the client
    treats the local tree as untrusted and re-checksums everything,
    which is what triggers diff detection."""
    if os.path.exists(cache_dir):
        shutil.rmtree(cache_dir, ignore_errors=True)
    os.makedirs(cache_dir, exist_ok=True)
    allowed = {"tmx","xml","tsx","js","png","jpg","gif","ogg","opus"}
    excludes = {".git", ".gitignore", ".gitattributes", ".github",
                ".claude", ".vscode", "__pycache__"}
    files_copied = []
    skipped_one = False
    for dirpath, dirs, files in os.walk(src_datapack):
        dirs[:] = [d for d in dirs if d not in excludes]
        for fn in sorted(files):
            ext = fn.rsplit(".", 1)[-1] if "." in fn else ""
            if ext not in allowed:
                continue
            src = os.path.join(dirpath, fn)
            rel = os.path.relpath(src, src_datapack)
            # Skip exactly one file (the first .xml encountered) so the
            # client has to diff-download it.
            if not skipped_one and ext == "xml":
                skipped_one = True
                log_info(f"cache partial: skipped {rel}")
                continue
            dst = os.path.join(cache_dir, rel)
            os.makedirs(os.path.dirname(dst), exist_ok=True)
            shutil.copy2(src, dst)
            files_copied.append(rel)
    # Add a stale file that doesn't exist server-side — exercises the
    # client's "remove stale local file" branch.
    stale = os.path.join(cache_dir, "_stale_unknown.xml")
    with open(stale, "w") as f:
        f.write("<?xml version=\"1.0\"?>\n<stale/>\n")
    cache_bin = cache_dir + "-cache"
    if os.path.exists(cache_bin):
        shutil.rmtree(cache_bin, ignore_errors=True)
    log_info(f"cache partial: copied {len(files_copied)} file(s) + stale marker")


# ── client ──────────────────────────────────────────────────────────────────
def run_client(build_dir, bin_name, label, timeout_override=None):
    binary = os.path.join(build_dir, bin_name)
    if not os.path.isfile(binary):
        log_fail(label, f"binary missing: {binary}")
        return False
    if not assert_port_or_fail("127.0.0.1", WS_PORT, log_fail, label):
        return False
    args = ["--url", f"ws://127.0.0.1:{WS_PORT}/",
            "--autologin", "--character", "Player",
            "--closewhenonmap"]
    log_info(f"client: {bin_name} {' '.join(args)}")
    env = os.environ.copy()
    env["QT_QPA_PLATFORM"] = "offscreen"
    base_timeout = timeout_override if timeout_override else CLIENT_TIMEOUT
    timeout = diagnostic.scale_timeout(DIAG, base_timeout)
    cmd = NICE_PREFIX + [binary] + args
    diagnostic.record_cmd(cmd, build_dir)
    proc = subprocess.Popen(
        cmd, cwd=build_dir,
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT, env=env,
        preexec_fn=process_helpers.setsid_and_pdeathsig)
    output = []
    done = threading.Event()
    outcome = [None]
    seen_state1 = [False]

    def reader():
        for raw in iter(proc.stdout.readline, b""):
            line = raw.decode(errors="replace").rstrip("\n")
            output.append(line)
            if "MapVisualiserPlayer::mapDisplayedSlot()" in line:
                outcome[0] = ("pass", "reached map")
                done.set()
                return
            if "Connection refused by the server" in line:
                outcome[0] = ("fail", "Connection refused")
                done.set()
                return
            if "stateChanged(1)" in line:
                seen_state1[0] = True
            elif seen_state1[0] and "stateChanged(0)" in line:
                outcome[0] = ("fail",
                              "stateChanged(0) after stateChanged(1) — "
                              "client connected then dropped before map")
                done.set()
                return
        done.set()

    threading.Thread(target=reader, daemon=True).start()
    triggered = done.wait(timeout=clamp_local(timeout))

    def _kill():
        try:
            os.killpg(os.getpgid(proc.pid), signal.SIGTERM)
        except ProcessLookupError:
            return
        try:
            proc.wait(timeout=5)
        except subprocess.TimeoutExpired:
            try:
                os.killpg(os.getpgid(proc.pid), signal.SIGKILL)
            except ProcessLookupError:
                pass
            try:
                proc.wait(timeout=5)
            except subprocess.TimeoutExpired:
                pass

    if not triggered:
        _kill()
        log_fail(label, f"timeout after {timeout}s")
        for l in output[-100:]:
            print(f"  | {l}")
        return False
    _kill()
    if outcome[0] is not None:
        kind, detail = outcome[0]
        if kind == "pass":
            log_pass(label, detail)
            return True
        log_fail(label, detail)
        for l in output[-100:]:
            print(f"  | {l}")
        return False
    log_fail(label, "client exited without reaching map")
    for l in output[-100:]:
        print(f"  | {l}")
    return False


# ── cleanup ─────────────────────────────────────────────────────────────────
def cleanup(*_args):
    stop_websockify()
    stop_server()
    stop_nginx()
    sys.exit(1)


signal.signal(signal.SIGINT, cleanup)
signal.signal(signal.SIGTERM, cleanup)


# ── main ────────────────────────────────────────────────────────────────────
def main():
    print(f"\n{C_CYAN}{'='*60}")
    print("  CatchChallenger — WebSocket Transport Testing")
    print(f"{'='*60}{C_RESET}\n")

    if shutil.which("websockify") is None:
        log_fail("setup", "websockify not on PATH (install dev-python/websockify)")
        summary()
        return
    if shutil.which("nginx") is None:
        log_fail("setup", "nginx not on PATH")
        summary()
        return
    if not os.path.isdir(DATAPACK_SRC):
        log_fail("setup", f"datapack missing: {DATAPACK_SRC}")
        summary()
        return
    main_dir = os.path.join(DATAPACK_SRC, "map", "main", MAINCODE)
    if not os.path.isdir(main_dir):
        log_fail("setup",
                 f"datapack maincode '{MAINCODE}' missing: {main_dir}")
        summary()
        return

    failed_cases = load_failed_cases()
    if failed_cases is not None and len(failed_cases) == 0:
        log_info("all previously passed (delete failed.json to re-run)")
        return

    # 1. Build server (IO_URING is the default-ON Linux backend; force it
    #    explicitly via the qmake-era CONFIG flag the cmake helper translates).
    if not build_project(SERVER_PRO, SERVER_BUILD, "server-filedb",
                         extra_qmake_args=["CONFIG+=catchchallenger_io_uring"]):
        summary()
        return
    # 2. Build qtcpu800x600 with WebSockets ON.
    if not build_project(CLIENT_CPU_PRO, CLIENT_CPU_BUILD, "qtcpu800x600",
                         extra_defines=["CATCHCHALLENGER_BUILD_QTCPU800X600_WEBSOCKETS"]):
        summary()
        return
    # 3. Build qtopengl with WebSockets ON.
    if not build_project(CLIENT_GL_PRO, CLIENT_GL_BUILD, "qtopengl",
                         extra_defines=["CATCHCHALLENGER_BUILD_QTOPENGL_WEBSOCKETS"]):
        summary()
        return

    total_expected[0] = 3 + 5  # 3 builds + nginx + server + websockify + 2 clients

    # 4. Stage backend, configure, start nginx + server + websockify.
    stage_server_datapack()
    reset_server_state()
    # Point nginx at the source datapack directly — it's read-only from
    # nginx's POV, and skipping the server's build/datapack symlink-of-
    # symlink avoids a chicken-and-egg ordering with start_nginx.
    stage_nginx_www(DATAPACK_SRC)
    if not prepare_http_mirror():
        summary()
        return
    # The mirror lives under /datapack/ on the nginx site so / stays
    # free for unrelated website content. MIRROR_DIR (==NGINX_WWW/datapack)
    # holds: pack/, datapack-list/, datapack/<files>. Client appends
    # "pack/datapack.tar.zst", "datapack-list/<x>.txt" and
    # "datapack/<file>" to this base URL, all of which resolve under
    # MIRROR_DIR.
    nginx_url = f"http://127.0.0.1:{NGINX_PORT}/datapack/"
    write_server_settings(nginx_url)

    if not start_nginx():
        summary()
        return
    if not start_server():
        stop_nginx()
        summary()
        return
    if not start_websockify():
        stop_server()
        stop_nginx()
        summary()
        return

    # 5. Run each client through three cache states. Server state
    #    (file-db database/, datapack-cache.bin) is reset between every
    #    client run because the auto-account-creation flow leaves
    #    per-pseudo records that conflict on a second connect — same
    #    pattern as testinghttp.py's per-case reset.
    #
    #    Order chosen so each state's prerequisites are met by the
    #    previous one:
    #      1. empty   — wipe cache; full HTTP download.
    #      2. same    — cache from #1 matches server; NO download.
    #      3. partial — cache pruned to one file + bad hash; diff DL.
    # qtopengl exposes a separate, deeper bug under empty cache + WS:
    # the server's datapackHashServerMain short-circuits as zero (or
    # the QthaveDatapackMainSubCode signal isn't propagating in this
    # build), so sendDatapackContentMain() never fires and the maincode
    # tarball is never requested. Partial cache works because the
    # files are pre-staged. Test order: partial first to prime the
    # cache, then `same` reuses it. `empty` is exercised on
    # qtcpu800x600 only — it covers the same http-mirror bootstrap
    # path that qtopengl currently can't drive on its own.
    for build_dir, bin_name, cache_dir, cli_label, cases in (
            (CLIENT_CPU_BUILD, CLIENT_CPU_BIN, CLIENT_CPU_CACHE_DIR,
             "qtcpu800x600", ("empty", "same", "partial")),
            (CLIENT_GL_BUILD,  CLIENT_GL_BIN,  CLIENT_GL_CACHE_DIR,
             "qtopengl",      ("empty", "same", "partial")),
    ):
        for case in cases:
            # Restart server between cases so per-pseudo file-db state
            # from the previous connect doesn't break this one.
            stop_server()
            reset_server_state()
            if not start_server():
                stop_websockify()
                stop_nginx()
                summary()
                return
            if case == "empty":
                setup_cache_empty(cache_dir)
            elif case == "partial":
                setup_cache_partial(cache_dir, DATAPACK_SRC)
            # else "same": leave cache from previous successful run.
            baseline = nginx_access_log_lines()
            # The partial-cache path triggers per-file http downloads
            # (~250 files for CatchChallenger-datapack), each adding
            # latency — bump the timeout so this case has room to
            # actually finish rather than tripping the 180s default.
            ct = 600 if case == "partial" else None
            run_client(build_dir, bin_name,
                       f"{cli_label} ws→map ({case} cache)",
                       timeout_override=ct)
            if case == "same":
                # Verify nothing was re-downloaded — files unchanged
                # since the previous run's cache populate.
                assert_no_redownload_since(
                    f"{cli_label} ({case})", baseline)

    stop_websockify()
    stop_server()
    stop_nginx()

    summary()


def summary():
    print(f"\n{C_CYAN}{'='*60}")
    print("  Summary")
    print(f"{'='*60}{C_RESET}")
    passed = sum(1 for r in results if r[1])
    failed = sum(1 for r in results if not r[1])
    total_elapsed = sum(r[3] for r in results)
    for name, ok, detail, elapsed in results:
        tag = f"{C_GREEN}PASS{C_RESET}" if ok else f"{C_RED}FAIL{C_RESET}"
        print(f"  [{tag}] {name}  {detail}  ({elapsed:.1f}s)")
    print(f"  total elapsed: {total_elapsed:.1f}s")
    print()
    print(f"  {C_GREEN}{passed} passed{C_RESET}, {C_RED}{failed} failed{C_RESET}")
    save_failed_cases()
    if failed:
        sys.exit(1)


if __name__ == "__main__":
    main()
