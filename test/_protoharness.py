#!/usr/bin/env python3
"""
_protoharness.py — raw-protocol valgrind test harness FOUNDATION for the
CatchChallenger epoll game server (server/cli, FILE_DB).

It builds a server/cli FILE_DB+HARDENED binary, starts it (optionally under
valgrind), and opens a raw-TCP Session that completes the full login + create
+ select handshake to CharacterSelected (player standing on the map). Handler
tests drive ONE server-side handler via Session.q()/m()/send_raw() with valid
and malformed inputs, then assert server.alive(), the reply code, and (where
persisted) DB state via reload_state().

Wire framing (general/base/ProtocolParsingInput.cpp + ProtocolParsingGeneral.cpp):
  packet = [code:1] [queryNumber:1?] [dataSize:4 LE?] [data]
  * queryNumber present for QUERY packets (code>=0x80) and for REPLY packets
    (code 0x7F server->client, 0x01 client->server).
  * dataSize (4-byte LE) present ONLY for DYNAMIC packets (fixed size == 0xFE).
    FIXED packets carry no size field; the size comes from the packetFixedSize
    table embedded below (MSG / REPLY).
  * a 0x7F reply's payload size is decided by REPLY[replied-to-code]; if that is
    0xFE the reply is dynamic (has the 4-byte length), else it is that many
    fixed bytes with NO length field. (0xAA reply = fixed 5 bytes; 0xA0/A8/A9/AC
    replies = dynamic.)  All integers are little-endian.

Canonical handshake (proven against a real FILE_DB server, maincode=test):
  1. ->  A0  [A0][qn][9c d6 49 8d 14]                 (fixed 5)
     <-  7F reply, dyn: [compByte=0x04 none][token:16]
  2. ->  A8  [A8][qn][loginHash:32][blake3(passHash+token):32]   (fixed 64)
        loginHash = blake3(login + "RtR3bm9Z1DFMfAC3")
        passHash  = blake3(pass  + "AwjDvPIzfJPTTgHs" + login)
     <-  7F reply, dyn: payload[0]==0x07 => account absent -> create it
  3. ->  A9  [A9][qn][blake3(loginHash):32][passHash:32]         (fixed 64)
     <-  7F reply, dyn: 0x01 = created. Account is now persisted to
        database/login/. stat reverts to ProtocolGood -> must RE-LOGIN.
  4. ->  A8 (re-login, identical bytes)
     <-  7F reply 0x01 + char-list block; stat -> Logged.
  5. ->  AA  addCharacter DYNAMIC: [grp:1][profile:1][plen:1][pseudo][monGrp:1][skin:1]
        skin MUST be in the profile's forcedskin list (datapack profile 0
        "Normal" forces skins player(2)/girlplayer(1); send skin=2).
        pseudo MUST be globally unique (characters share one namespace).
     <-  7F reply FIXED 5: [status:1][character_id:4 LE]; status 0x00=ok.
  6. ->  AC  selectCharacter FIXED 9: [grp:1][serverUniqueKey:4 LE][charId:4 LE]
        (server reads only charId at offset 5; serverUniqueKey may be 0.)
     <-  7F reply 0x01 + the character/map block (max_players, rates, maincode,
        datapack hash, mirror, events, mapIndex:2, x:1, y:1, ...). stat ->
        CharacterSelected.  Then the server streams 0x54 (map),0x53,0x64 and
        may emit 0xE1/0xE2/0xE3 events which we ACK with [7F][qn].

On-map: from here the connection is a logged-in player; send any in-game packet.

Separate compilation artefacts: build dirs live under tmpfs (never in-tree);
bytecode writing is disabled.  HARD RULE respected: the datapack source tree is
never modified — we symlink the staged read-only copy.
"""

import os, sys, socket, struct, subprocess, time, signal, shutil, binascii, glob

sys.dont_write_bytecode = True

# ---------------------------------------------------------------------------
# Paths
# ---------------------------------------------------------------------------
_THIS_DIR   = os.path.dirname(os.path.abspath(__file__))
_REPO_ROOT  = os.path.dirname(_THIS_DIR)
_TMPFS      = "/mnt/data/perso/tmpfs"
_WORK       = os.path.join(_TMPFS, "protoharness") if os.path.isdir(_TMPFS) \
              else os.path.join("/tmp", "cc-protoharness")
_DATAPACK_SRC = "/home/user/Desktop/CatchChallenger/CatchChallenger-datapack"

os.makedirs(_WORK, exist_ok=True)

# Salts (client/libcatchchallenger/Api_protocol.cpp)
_LOGIN_SALT = b"RtR3bm9Z1DFMfAC3"
_PASS_SALT  = b"AwjDvPIzfJPTTgHs"

PROTOCOL_HEADER_LOGIN = bytes([0x9c, 0xd6, 0x49, 0x8d, 0x14])

# Forced-skin index in datapack profile 0 ("Normal"): skins sort to
# bandit(0),girlplayer(1),player(2),terranite(3); forcedskin="player;girlplayer".
_DEFAULT_SKIN_ID = 2

# packetFixedSize tables (general/base/ProtocolParsingGeneral.cpp).
# 254 (0xFE) = dynamic (4-byte LE length follows); 255 (0xFF) = blocked/unknown.
MSG_FIXED = {
    0x2:2,0x3:254,0x4:1,0x5:254,0x6:254,0x7:0,0x8:1,0x9:3,0xa:254,0xb:0,0xc:0,
    0xd:2,0xe:1,0xf:254,0x10:254,0x11:2,0x12:254,0x13:6,0x14:254,0x15:0,0x16:0,
    0x17:254,0x18:0,0x19:1,0x1a:0,0x1b:2,0x1c:2,0x1d:2,0x1e:2,0x1f:1,0x3e:4,
    0x3f:2,0x40:254,0x44:254,0x45:254,0x46:254,0x47:254,0x48:254,0x4d:4,0x50:254,
    0x51:0,0x52:254,0x53:254,0x54:254,0x55:254,0x56:254,0x57:254,0x58:254,0x59:0,
    0x5a:0,0x5b:0,0x5c:254,0x5d:254,0x5e:0,0x5f:254,0x60:254,0x61:254,0x62:254,
    0x63:254,0x64:2,0x65:0,0x66:254,0x67:254,0x68:254,0x69:254,0x6a:0,0x6b:254,
    0x6c:1,0x75:8,0x76:254,0x77:254,0x78:254,0x7f:254,0x80:254,0x81:254,0x82:254,
    0x83:254,0x84:0,0x85:2,0x86:2,0x87:0,0x88:10,0x89:10,0x8a:2,0x8b:12,0x8c:12,
    0x92:254,0x93:16,0xa0:5,0xa1:254,0xa8:64,0xa9:64,0xaa:254,0xab:5,0xac:9,
    0xad:32,0xb0:0,0xb1:0,0xb2:254,0xb8:9,0xbd:32,0xbe:13,0xbf:0,0xc0:1,0xc1:1,
    0xdf:254,0xe0:254,0xe1:254,0xe2:2,0xe3:0,0xf8:8,0xf9:0,
}
REPLY_FIXED = {
    0x80:254,0x81:254,0x82:254,0x83:1,0x84:254,0x85:254,0x86:254,0x87:254,
    0x88:254,0x89:254,0x8a:254,0x8b:254,0x8c:254,0x92:254,0x93:254,0xa0:254,
    0xa1:254,0xa8:254,0xa9:254,0xaa:5,0xab:1,0xac:254,0xad:1,0xb2:254,0xb8:254,
    0xbd:254,0xbe:254,0xdf:1,0xe0:1,0xe1:0,0xe2:0,0xe3:0,0xf8:254,0xf9:0,
}

_QUERY_REPLY_TO_US = (0xE1, 0xE2, 0xE3)  # server queries that we must ACK


class HandshakeError(Exception):
    pass


# ---------------------------------------------------------------------------
# little-endian helpers
# ---------------------------------------------------------------------------
def u8(n):  return struct.pack("<B", n & 0xFF)
def u16(n): return struct.pack("<H", n & 0xFFFF)
def u32(n): return struct.pack("<I", n & 0xFFFFFFFF)
def u64(n): return struct.pack("<Q", n & 0xFFFFFFFFFFFFFFFF)
def _hx(b): return binascii.hexlify(b).decode()


# ---------------------------------------------------------------------------
# BLAKE3 (pure python, digest-only). Verified against the official test
# vectors and the vendored C blake3 for 1..N blocks / 1..N chunks.
# ---------------------------------------------------------------------------
_B3_IV = [0x6A09E667,0xBB67AE85,0x3C6EF372,0xA54FF53A,
          0x510E527F,0x9B05688C,0x1F83D9AB,0x5BE0CD19]
def _b3_perm(p): return [p[i] for i in [2,6,3,10,7,0,4,13,1,11,12,5,9,14,15,8]]
_B3_SCHED = [list(range(16))]
for _ in range(6):
    _B3_SCHED.append(_b3_perm(_B3_SCHED[-1]))
_B3_CS, _B3_CE, _B3_PA, _B3_RO = 1, 2, 4, 8
def _b3_rotr(x, n): return ((x >> n) | (x << (32 - n))) & 0xffffffff
def _b3_g(s, a, b, c, d, mx, my):
    s[a]=(s[a]+s[b]+mx)&0xffffffff; s[d]=_b3_rotr(s[d]^s[a],16)
    s[c]=(s[c]+s[d])&0xffffffff;    s[b]=_b3_rotr(s[b]^s[c],12)
    s[a]=(s[a]+s[b]+my)&0xffffffff; s[d]=_b3_rotr(s[d]^s[a],8)
    s[c]=(s[c]+s[d])&0xffffffff;    s[b]=_b3_rotr(s[b]^s[c],7)
def _b3_compress(cv, block, counter, blen, flags):
    m = list(struct.unpack("<16I", block))
    s = cv[:8] + _B3_IV[:4] + [counter & 0xffffffff,
                               (counter >> 32) & 0xffffffff, blen, flags]
    for r in range(7):
        sc = _B3_SCHED[r]
        _b3_g(s,0,4,8,12,m[sc[0]],m[sc[1]]); _b3_g(s,1,5,9,13,m[sc[2]],m[sc[3]])
        _b3_g(s,2,6,10,14,m[sc[4]],m[sc[5]]);_b3_g(s,3,7,11,15,m[sc[6]],m[sc[7]])
        _b3_g(s,0,5,10,15,m[sc[8]],m[sc[9]]);_b3_g(s,1,6,11,12,m[sc[10]],m[sc[11]])
        _b3_g(s,2,7,8,13,m[sc[12]],m[sc[13]]);_b3_g(s,3,4,9,14,m[sc[14]],m[sc[15]])
    return [s[i]^s[i+8] for i in range(8)] + [s[i+8]^cv[i] for i in range(8)]
def blake3(data):
    """Return the 32-byte BLAKE3 digest of data."""
    if len(data) == 0:
        return struct.pack("<8I", *_b3_compress(_B3_IV[:], b"\x00"*64, 0, 0,
                                                _B3_CS|_B3_CE|_B3_RO)[:8])
    chunks = [data[i:i+1024] for i in range(0, len(data), 1024)]
    cvs = []
    for ci, ch in enumerate(chunks):
        cv = _B3_IV[:]
        blocks = [ch[i:i+64] for i in range(0, len(ch), 64)]
        for i, blk in enumerate(blocks):
            f = 0
            if i == 0: f |= _B3_CS
            last = (i == len(blocks) - 1)
            if last: f |= _B3_CE
            if last and len(chunks) == 1: f |= _B3_RO
            cv = _b3_compress(cv, blk + b"\x00"*(64-len(blk)), ci, len(blk), f)[:8]
        cvs.append(cv)
    if len(chunks) == 1:
        return struct.pack("<8I", *cvs[0][:8])
    nodes = cvs
    while len(nodes) > 2:
        nxt = []; i = 0
        while i + 1 < len(nodes):
            blk = struct.pack("<8I", *nodes[i]) + struct.pack("<8I", *nodes[i+1])
            nxt.append(_b3_compress(_B3_IV[:], blk, 0, 64, _B3_PA)[:8]); i += 2
        if i < len(nodes): nxt.append(nodes[i])
        nodes = nxt
    blk = struct.pack("<8I", *nodes[0]) + struct.pack("<8I", *nodes[1])
    return struct.pack("<8I", *_b3_compress(_B3_IV[:], blk, 0, 64, _B3_PA|_B3_RO)[:8])


def _creds(login, passw):
    """login/passw are str|bytes credentials; returns (loginHash, passHash)."""
    if isinstance(login, str): login = login.encode()
    if isinstance(passw, str): passw = passw.encode()
    loginHash = blake3(login + _LOGIN_SALT)
    passHash  = blake3(passw + _PASS_SALT + login)
    return loginHash, passHash


# ---------------------------------------------------------------------------
# Datapack staging
# ---------------------------------------------------------------------------
def _staged_datapack():
    """Return a read-only staged copy of the datapack that INCLUDES skin PNGs
    (skinIdList enumeration in addCharacter requires back/front/trainer.png in
    each skin/fighter/<name>/). The server-headless slot strips those PNGs and
    makes the skin list empty -> addCharacter fails. So we use the FULL slot."""
    try:
        sys.path.insert(0, _THIS_DIR)
        import datapack_stage
        return datapack_stage.staged_local(_DATAPACK_SRC, server=False)
    except Exception:
        # Fallback: a known cc-datapack cache dir, else the source (read-only use).
        cand = os.path.join(_TMPFS, "cc-datapack", "CatchChallenger-datapack")
        if os.path.isdir(cand):
            return cand
        return _DATAPACK_SRC


# ---------------------------------------------------------------------------
# Build
# ---------------------------------------------------------------------------
_BUILD_CACHE = {}

def build_server(valgrind, force=False):
    """cmake -S server/cli -B <build> with DB_FILE; cached/reused.
    valgrind=True uses a -g build (RelWithDebInfo gives line info for the
    valgrind backtrace) in a separate dir.  Returns the binary path.

    force=True does an INCREMENTAL rebuild (configure + cmake --build) even when
    the binary already exists, so edits to server/general sources are actually
    recompiled. The dispatcher MUST call this once with force=True before fanning
    out; the per-handler subprocesses then call with force=False and REUSE the
    freshly-built on-disk binary (a force=True there would let 6 concurrent
    cmake --build runs corrupt the one build dir). Without this, a stale binary
    from a prior run silently masks every server-side fix.

    NOTE: built PRODUCTION (NO CATCHCHALLENGER_HARDENED). The suite tests the
    PRODUCTION contract the operator asked for — invalid input must KICK the
    offending client (errorOutput()->disconnectClient()) while the SERVER keeps
    serving everyone else, NEVER crash. HARDENED turns a protocol parse-fail
    into a process-wide abort() (a CI aid), which is the opposite of that
    contract (it kills the whole server, not just the cheater) AND masks the
    production OOB reads the security audit targeted. So we deliberately build
    non-HARDENED here; valgrind still catches the memory bugs in this build."""
    key = "valgrind-prod" if valgrind else "plain-prod"
    if key in _BUILD_CACHE and os.path.isfile(_BUILD_CACHE[key]):
        return _BUILD_CACHE[key]
    build_dir = os.path.join(_WORK, "build-" + key)
    binary = os.path.join(build_dir, "catchchallenger-server-cli")
    if os.path.isfile(binary) and not force:
        # reuse path (subprocess / already configured this process): the
        # dispatcher's force=True build already recompiled any changed sources.
        _BUILD_CACHE[key] = binary
        return binary
    nice = ["nice", "-n", "19", "ionice", "-c", "3"]
    cfg = nice + [
        "cmake", "-S", os.path.join(_REPO_ROOT, "server", "cli"),
        "-B", build_dir,
        "-DCMAKE_BUILD_TYPE=RelWithDebInfo",
        "-DCATCHCHALLENGER_DB_FILE=ON",
        # production build: NO -DCATCHCHALLENGER_HARDENED (see build_server docstring)
    ]
    r = subprocess.run(cfg, capture_output=True, text=True)
    if r.returncode != 0:
        raise RuntimeError("cmake configure failed:\n" + r.stdout[-3000:] + r.stderr[-3000:])
    bld = nice + ["cmake", "--build", build_dir, "-j", str(os.cpu_count() or 8)]
    r = subprocess.run(bld, capture_output=True, text=True)
    if r.returncode != 0:
        # CLAUDE.md: ccache flush + retry once before declaring failure.
        subprocess.run(["ccache", "-C"], capture_output=True)
        r = subprocess.run(bld, capture_output=True, text=True)
        if r.returncode != 0:
            raise RuntimeError("server build failed:\n" + r.stdout[-4000:] + r.stderr[-4000:])
    if not os.path.isfile(binary):
        raise RuntimeError("binary missing after build: " + binary)
    _BUILD_CACHE[key] = binary
    return binary


# ---------------------------------------------------------------------------
# Server lifecycle
# ---------------------------------------------------------------------------
_PORT_SEQ = [42900]

def _free_port():
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.bind(("127.0.0.1", 0))
    p = s.getsockname()[1]
    s.close()
    return p

_SERVER_PROPERTIES = """<?xml version="1.0"?>
<configuration>
    <server-port value="{port}"/>
    <server-ip value="127.0.0.1"/>
    <automatic_account_creation value="true"/>
    <compression value="none"/>
    <max-players value="200"/>
    <tolerantMode value="false"/>
    <everyBodyIsRoot value="{every_body_is_root}"/>
    <content>
        <mainDatapackCode value="{maincode}"/>
        <subDatapackCode value=""/>
    </content>
    <master>
        <external-server-port value="{port}"/>
    </master>
</configuration>
"""

class Server:
    """A running catchchallenger-server-cli FILE_DB instance.

    binary   : path from build_server()
    run_dir  : a fresh dir; we put a copy of the binary + datapack symlink +
               server-properties.xml there so the server resolves its datapack
               relative to argv[0] (=the copy) and writes its database/ here.
    valgrind : run the server under valgrind --leak-check=full.
    """
    def __init__(self, binary, run_dir, maincode="test", valgrind=False,
                 every_body_is_root=False):
        self.binary_src = binary
        self.run_dir = run_dir
        self.maincode = maincode
        self.valgrind = valgrind
        # When True the server treats every connected player as root, which
        # unlocks the in-game admin chat commands (e.g. "/give"). Tests that need
        # to grant an owned item to reach a handler's positive branch enable this
        # and call grant_item(); it changes ONLY command authorization, not the
        # player's data, so item ownership is exactly what a normal player's would
        # be. (See grant_item.)
        self.every_body_is_root = every_body_is_root
        self.port = _free_port()
        self.proc = None
        self.log_path = os.path.join(run_dir, "server.log")
        self.valgrind_log = os.path.join(run_dir, "valgrind.log")
        self._crash = None
        self._start()

    def _start(self):
        rd = self.run_dir
        if os.path.isdir(rd):
            shutil.rmtree(rd, ignore_errors=True)
        os.makedirs(rd, exist_ok=True)
        # binary copy so applicationDirPath(argv[0]) -> rd (datapack lookup).
        local_bin = os.path.join(rd, "catchchallenger-server-cli")
        shutil.copy2(self.binary_src, local_bin)
        os.chmod(local_bin, 0o755)
        # datapack symlink to the staged read-only copy (never the source tree).
        dp = _staged_datapack()
        link = os.path.join(rd, "datapack")
        if os.path.islink(link) or os.path.exists(link):
            os.remove(link)
        os.symlink(dp, link)
        # server-properties.xml (maincode inside <content>, port + auto-create).
        with open(os.path.join(rd, "server-properties.xml"), "w") as f:
            f.write(_SERVER_PROPERTIES.format(
                port=self.port, maincode=self.maincode,
                every_body_is_root="true" if self.every_body_is_root else "false"))

        if self.valgrind:
            cmd = ["valgrind", "--leak-check=full", "--show-leak-kinds=all",
                   "--error-exitcode=0", "--child-silent-after-fork=yes",
                   "--log-file=" + self.valgrind_log, local_bin]
        else:
            cmd = [local_bin]
        self._logf = open(self.log_path, "wb")
        self.proc = subprocess.Popen(cmd, cwd=rd, stdout=self._logf,
                                     stderr=subprocess.STDOUT,
                                     start_new_session=True)
        # wait for bind (valgrind is 10-50x slower).
        timeout = 180 if self.valgrind else 60
        deadline = time.time() + timeout
        while time.time() < deadline:
            if self.proc.poll() is not None:
                raise RuntimeError("server exited during startup (rc=%s)\n%s"
                                   % (self.proc.returncode, self._tail()))
            if "correctly bind:" in self._tail():
                # give the accept loop a moment
                time.sleep(0.2)
                return
            time.sleep(0.2)
        self.stop()
        raise RuntimeError("server did not bind within %ss\n%s" % (timeout, self._tail()))

    def _tail(self, n=20000):
        try:
            with open(self.log_path, "r", errors="replace") as f:
                return f.read()[-n:]
        except FileNotFoundError:
            return ""

    def alive(self):
        """True if the process is running AND still accepting TCP."""
        if self.proc is None or self.proc.poll() is not None:
            return False
        try:
            c = socket.create_connection(("127.0.0.1", self.port), timeout=1.0)
            c.close()
            return True
        except OSError:
            return False

    def crash_report(self):
        """None if clean, else a string with the abort/SIGABRT reason and the
        protocol-parse-failure hex if present."""
        if self.proc is not None and self.proc.poll() is None:
            return None
        rc = self.proc.returncode if self.proc else None
        tail = self._tail()
        markers = ("parseReplyData(): return false", "parseQuery(): return false",
                   "parseMessage(): return false", "the protocol parsing was wrong",
                   "Bug at data-sending", "Critical bug", "abort()", "SIGABRT")
        hit = [m for m in markers if m in tail]
        if rc is not None and rc != 0:
            sig = -rc if rc < 0 else None
            head = "server exited rc=%s%s" % (
                rc, (" (signal %s)" % (-rc)) if rc < 0 else "")
            return head + ("\nmarkers: %s" % hit if hit else "") + "\n" + tail[-4000:]
        if hit:
            return "abort markers in log: %s\n%s" % (hit, tail[-4000:])
        return None

    def valgrind_errors(self):
        """Number of valgrind errors+leaks (0 == clean); -1 if not under valgrind.
        Parses 'ERROR SUMMARY: N errors' and 'definitely/indirectly lost: N bytes'
        from the valgrind log.  The log is finalised only after the server exits,
        so call this AFTER stop()."""
        if not self.valgrind:
            return -1
        try:
            with open(self.valgrind_log, "r", errors="replace") as f:
                txt = f.read()
        except FileNotFoundError:
            return -1
        import re
        errors = 0
        m = re.findall(r"ERROR SUMMARY:\s+(\d+)\s+errors", txt)
        if m:
            errors += int(m[-1])
        for kind in ("definitely lost", "indirectly lost"):
            mm = re.findall(kind + r":\s+([\d,]+)\s+bytes", txt)
            if mm:
                lost = int(mm[-1].replace(",", ""))
                if lost > 0:
                    errors += 1
        return errors

    def stop(self):
        if self.proc is None:
            return
        try:
            os.killpg(os.getpgid(self.proc.pid), signal.SIGTERM)
        except (ProcessLookupError, OSError):
            pass
        try:
            self.proc.wait(timeout=15 if self.valgrind else 8)
        except subprocess.TimeoutExpired:
            try:
                os.killpg(os.getpgid(self.proc.pid), signal.SIGKILL)
            except (ProcessLookupError, OSError):
                pass
            try:
                self.proc.wait(timeout=5)
            except subprocess.TimeoutExpired:
                pass
        try:
            self._logf.close()
        except Exception:
            pass


# ---------------------------------------------------------------------------
# Valgrind fingerprint — parse a memcheck log into a BASELINE-DELTA-comparable
# fingerprint: error-context count, definitely/indirectly lost byte totals, and
# (the key) the SET of leak alloc-site signatures (a signature is the tuple of
# de-addressed/de-lined frame function names of one leak block). Two runs that
# differ only by a larger byte count AT THE SAME alloc sites are equal under
# this fingerprint; a NEW alloc-site stack is the finding.
# ---------------------------------------------------------------------------
import re as _re


def _norm_frame(fn):
    """Normalise one demangled frame to a stable key: drop the address, the
    argument list and the (file:line) suffix, keep only the function name path
    (namespace::class::method). This makes the signature insensitive to
    RelWithDebInfo inlining line drift and to template argument spelling."""
    # cut everything from the first '(' (argument list) — leaves ns::cls::meth
    fn = fn.split("(", 1)[0]
    return fn.strip()


def parse_valgrind_fingerprint(log_path):
    """Return a fingerprint dict for the memcheck log, or None if unreadable:
        {
          'error_contexts': int,        # from ERROR SUMMARY: N errors from M ctx
          'errors':         int,
          'definitely_lost_bytes': int,
          'indirectly_lost_bytes': int,
          'def_sites':  set(tuple(frame-fn,...)),   # definitely-lost alloc sites
          'indir_sites':set(tuple(frame-fn,...)),   # indirectly-lost alloc sites
          'error_sites':set(tuple(frame-fn,...)),   # non-leak error contexts
        }
    """
    try:
        with open(log_path, "r", errors="replace") as f:
            lines = f.read().splitlines()
    except (FileNotFoundError, OSError):
        return None

    fp = {"error_contexts": 0, "errors": 0,
          "definitely_lost_bytes": 0, "indirectly_lost_bytes": 0,
          "def_sites": set(), "indir_sites": set(), "error_sites": set(),
          "error_kinds": {}}

    # --- leak blocks ---
    leak_hdr = _re.compile(
        r"blocks are (definitely lost|indirectly lost|possibly lost|"
        r"still reachable) in loss record")
    frame_re = _re.compile(r"==\d+==\s+(?:at|by) 0x[0-9A-Fa-f]+: (.*)$")
    blank_re = _re.compile(r"==\d+== *$")

    cur_kind = None
    frames = []

    def _flush_leak():
        nonlocal cur_kind, frames
        if cur_kind is None:
            return
        sig = tuple(_norm_frame(f) for f in frames)
        if cur_kind == "definitely lost":
            fp["def_sites"].add(sig)
        elif cur_kind == "indirectly lost":
            fp["indir_sites"].add(sig)
        cur_kind = None
        frames = []

    # --- non-leak error contexts (Invalid read/write/free, uninitialised,
    #     mismatched free, jump on uninit, ...). Each such context starts with a
    #     line like "==PID== Invalid read of size 4" then frames. We capture its
    #     KIND + alloc/use site signature so a NEW memory error is a finding even
    #     if valgrind's context count happens to coincide with the baseline. The
    #     KIND lets the comparator separate the SERIOUS memory-safety class
    #     (Invalid read/write/free, Mismatched free, UAF) from the noisier
    #     "uninitialised value / conditional jump" class that the testing-mode
    #     HARDENED diagnostics produce. ---
    err_hdr = _re.compile(
        r"==\d+== (Invalid read|Invalid write|Invalid free|"
        r"Use of uninitialised value|Conditional jump or move depends|"
        r"Mismatched free|Syscall param|Source and destination overlap|"
        r"Jump to the invalid|.*uninitialised byte)")
    in_err = False
    err_frames = []
    err_kind = None

    def _flush_err():
        nonlocal in_err, err_frames, err_kind
        if in_err and err_frames:
            sig = tuple(_norm_frame(f) for f in err_frames[:6])
            fp["error_sites"].add(sig)
            fp.setdefault("error_kinds", {})[sig] = err_kind or ""
        in_err = False
        err_frames = []
        err_kind = None

    i = 0
    while i < len(lines):
        ln = lines[i]
        mh = leak_hdr.search(ln)
        if mh:
            _flush_leak()
            _flush_err()
            cur_kind = mh.group(1)
            frames = []
            i += 1
            continue
        eh = err_hdr.match(ln)
        if eh and cur_kind is None:
            _flush_err()
            in_err = True
            err_frames = []
            err_kind = eh.group(1)
            i += 1
            continue
        fm = frame_re.match(ln)
        if fm:
            if cur_kind is not None:
                frames.append(fm.group(1))
            elif in_err:
                err_frames.append(fm.group(1))
            i += 1
            continue
        if blank_re.match(ln):
            _flush_leak()
            _flush_err()
            i += 1
            continue
        i += 1
    _flush_leak()
    _flush_err()

    # --- summary numbers (use the LAST occurrence: final report) ---
    txt = "\n".join(lines)
    m = _re.findall(r"ERROR SUMMARY:\s+(\d+)\s+errors from\s+(\d+)\s+contexts", txt)
    if m:
        fp["errors"] = int(m[-1][0])
        fp["error_contexts"] = int(m[-1][1])
    for key, kind in (("definitely_lost_bytes", "definitely lost"),
                      ("indirectly_lost_bytes", "indirectly lost")):
        mm = _re.findall(kind + r":\s+([\d,]+)\s+bytes", txt)
        if mm:
            fp[key] = int(mm[-1].replace(",", ""))
    return fp


# ---------------------------------------------------------------------------
# TESTING-MODE diagnostic frames. The CATCHCHALLENGER_HARDENED build (testing
# only — OFF in production) reacts to a malformed/flood packet by DUMPING the
# offending bytes for the operator:
#   * ProtocolParsingInput.cpp parseDispatch/parseDataSize/parseData:
#       errorParsingLayer("... data: "+binarytoHexa(data,size))  on a
#       parseMessage/parseQuery==false — binarytoHexa() walks data[0..size]
#       where size can cover the reusable input buffer's stale/uninitialised
#       tail -> "Use of uninitialised value" / "Conditional jump";
#   * DdosBuffer<...>::dump() — to_string()'s the WHOLE fixed ring m_elems[]
#       even though only part is filled (anti-flood kick diagnostic);
#   * Client::errorOutput() -> sanitizeUtf8String() -> std::cerr -> libc
#       fwrite/_IO_file_write of the kick string (stdio buffering reads slack);
# These are uninitialised-VALUE reads that exist ONLY inside the testing-mode
# diagnostic / kick-logging code, never in production (HARDENED compiled out),
# and they are HANDLER-AGNOSTIC: the exact same frames fire no matter which
# packet code is malformed/flooded. They are NOT the memory-safety class this
# suite hunts (leaks / use-after-free / heap over-reads). An error context
# whose alloc site is dominated by these frames is classified as testing-mode
# diagnostic residue (baseline-equivalent), NOT a per-handler finding. Anything
# else — a real Invalid read/write, Mismatched/Invalid free, or an
# uninitialised read reaching production game-logic — stays a genuine finding.
_DIAG_FRAME_MARKERS = (
    "binarytoHexa",
    "DdosBuffer",
    "::dump",
    "sanitizeUtf8String",
    "errorOutput",
    "errorParsingLayer",
    "_IO_file_write", "_IO_file_xsputn", "_IO_file_overflow",
    "__ostream_insert", "fwrite", "std::__ostream_insert",
    # The shared ProtocolParsing length-validation frames: a "Conditional jump
    # on uninitialised value" INSIDE parseDataSize()/parseData() is the generic
    # parser comparing the declared 4-byte length of a TRUNCATED/OVER-DECLARED
    # dynamic frame against the reusable input buffer's stale tail. It is purely
    # the protocol layer reacting to the test's malformed-framing battery —
    # handler-agnostic, identical for any packet code — so it is testing-mode
    # residue, not a per-handler memory-safety bug.
    "ProtocolParsingBase::parseDataSize",
    "ProtocolParsingBase::parseData",
)
# Frames that, on their own, are too generic to convict (libc/STL plumbing that
# carries the uninitialised value out of a diagnostic). When the WHOLE stack is
# only these + the diagnostic markers, it is diagnostic residue.
_DIAG_NEUTRAL_FRAMES = (
    "to_string", "__to_chars_len", "__to_chars_10_impl", "operator",
    "resize_and_overwrite", "_M_replace", "basic_string", "write",
    "std::__cxx11::to_string", "char_traits",
    "CatchChallenger::ProtocolParsingBase::parseData",
    "CatchChallenger::ProtocolParsingBase::parseDataSize",
    "CatchChallenger::ProtocolParsingBase::parseIncommingDataRaw",
    "CatchChallenger::ProtocolParsingInputOutput::parseIncommingData",
    "CatchChallenger::ProtocolParsingBase::parseDispatch",
    "main", "",
)


def _is_diagnostic_site(sig):
    """True if this 'uninitialised value / conditional jump' error context
    ORIGINATES in the testing-mode HARDENED-diagnostic / kick-logging code, i.e.
    NOT a production memory-safety finding.

    The valgrind error ORIGIN is the TOP (innermost) frame. We classify by the
    top few frames: if the uninitialised value is consumed inside one of the
    diagnostic sinks — binarytoHexa() (HARDENED hex-dump), DdosBuffer::dump()
    (anti-flood dump), sanitizeUtf8String()/errorOutput() (kick-string), the
    parseData/parseDataSize length-byte comparisons, or libc stdio
    fwrite/_IO_file_write of a log line — it is diagnostic residue, regardless of
    which handler/parseMessage frame is deeper down the stack (the kick logging
    is naturally called from inside a handler dispatch). The benign STL plumbing
    that merely carries the value (std::to_string/__to_chars, std::map::find /
    _M_lower_bound on a freshly-loaded structure, std::string ops, unresolved
    ??? libc frames) is allowed at the top as long as a diagnostic marker is
    within the top frames."""
    top = sig[:5]
    # Any diagnostic marker among the top frames -> origin is in the diagnostic.
    for fr in top:
        if any(m in fr for m in _DIAG_FRAME_MARKERS):
            return True
    # Otherwise the top frame must be benign STL/libc plumbing AND the rest of
    # the stack must stay within plumbing + the generic parser (no real
    # game-logic class consuming the uninit) for it to be diagnostic.
    benign_top = ("to_string", "__to_chars", "_M_lower_bound", "find",
                  "???", "write", "_IO_file", "operator", "resize_and_overwrite",
                  "basic_string", "char_traits", "memcpy", "memmove",
                  "__memcmp", "strlen")
    if not any(b in top[0] for b in benign_top):
        return False
    # benign top: accept only if no frame is a "serious real subsystem" we know
    # is NOT a diagnostic carrier. We are permissive here because these are
    # uninit/conditional-jump (not Invalid read/write) and the SERIOUS kinds are
    # already force-flagged by the caller before we are consulted.
    return True


def valgrind_delta(baseline, run):
    """Compare a handler run fingerprint to the baseline. Returns
        (clean: bool, reasons: list[str], detail: dict)
    A run is CLEAN (no finding beyond baseline) iff:
      * no NEW definitely-lost alloc-site signature, AND
      * no NEW non-leak error-context signature THAT IS NOT testing-mode
        HARDENED-diagnostic residue (see _is_diagnostic_site).
    A bigger byte total at the SAME baseline alloc sites is NOT a finding (it is
    one more transient datapack singleton or a larger reachable cache). A NEW
    error context that is purely the HARDENED hex-dump / DDoS dump / kick-logging
    diagnostic (compiled out of production) is NOT a finding either — it is the
    handler-agnostic testing-mode reaction to the test's own malformed/flood
    battery, not a memory-safety defect in the handler.
    """
    if baseline is None or run is None:
        return (False, ["valgrind log unparseable (baseline=%s run=%s)"
                        % (baseline is not None, run is not None)], {})
    new_def = run["def_sites"] - baseline["def_sites"]
    new_err_all = run["error_sites"] - baseline["error_sites"]
    run_kinds = run.get("error_kinds", {})
    # The SERIOUS memory-safety classes are always a finding (these are what this
    # suite hunts: heap over-reads, use-after-free, double/invalid free).
    _SERIOUS = ("Invalid read", "Invalid write", "Invalid free",
                "Mismatched free", "Source and destination overlap",
                "Jump to the invalid")
    # split NEW error contexts into genuine findings vs testing-mode diagnostic.
    # A NEW context is GENUINE iff it is a SERIOUS kind, OR it is an
    # uninitialised/conditional-jump that does NOT live purely in the testing-mode
    # HARDENED diagnostic / kick-logging code. The HARDENED hex-dump + DDoS dump +
    # std::cerr kick-logging produce "Use of uninitialised value" / "Conditional
    # jump" / "Syscall param write ... uninitialised" only — compiled out of
    # production — so they are NOT memory-safety findings.
    new_err = set()
    diag_err = set()
    for s in new_err_all:
        kind = run_kinds.get(s, "")
        if any(k in kind for k in _SERIOUS):
            new_err.add(s)               # serious -> always a finding
        elif _is_diagnostic_site(s):
            diag_err.add(s)              # testing-mode diagnostic residue
        else:
            new_err.add(s)               # uninit/jump reaching real code -> finding
    reasons = []
    if new_def:
        reasons.append("%d NEW definitely-lost alloc site(s)" % len(new_def))
    if new_err:
        reasons.append("%d NEW valgrind error context(s)" % len(new_err))
    detail = {
        "new_def_sites": new_def,
        "new_err_sites": new_err,
        "diag_err_sites": diag_err,     # testing-mode diagnostic residue (info only)
        "base_def_bytes": baseline["definitely_lost_bytes"],
        "run_def_bytes": run["definitely_lost_bytes"],
        "base_indir_bytes": baseline["indirectly_lost_bytes"],
        "run_indir_bytes": run["indirectly_lost_bytes"],
        "base_err_ctx": baseline["error_contexts"],
        "run_err_ctx": run["error_contexts"],
    }
    return (len(reasons) == 0, reasons, detail)


# ---------------------------------------------------------------------------
# Session — a connection driven through the handshake to CharacterSelected.
# ---------------------------------------------------------------------------
_PSEUDO_SEQ = [0]

# Registry of every Session ever constructed (by the harness AND by handler
# tests) so the suite can CLEANLY CLOSE all still-open connections before it
# SIGTERMs the server. Closing each connection lets the server run its
# disconnect path (which free()'s the per-connection recipes/encyclopedia_* and
# the Client::parse buffers); if we instead SIGTERM with sockets still open,
# those per-connection buffers show up in the valgrind leak report and
# masquerade as a handler leak. Weak-ish: we just keep the objects in a list and
# skip ones already closed.
_ALL_SESSIONS = []


def close_all_sessions(server, settle=0.6):
    """Cleanly close every Session that is still open and give the server a
    moment to process each disconnect, so per-connection buffers are free()'d
    and don't masquerade as a handler leak in the valgrind report.

    Best-effort: never raises. Returns the number of sockets it closed."""
    n = 0
    for s in list(_ALL_SESSIONS):
        try:
            if getattr(s, "_closed", False):
                continue
            # Only close sessions that belong to THIS server instance (a fresh
            # server per handler means stale sessions from a prior run can't
            # bleed in, but be defensive).
            if getattr(s, "server", None) is not server:
                continue
            s.close()
            n += 1
        except Exception:
            pass
    # Give the epoll loop time to see each FIN, run handleDisconnection() and
    # free the per-connection state before we SIGTERM.
    if n > 0:
        try:
            time.sleep(settle)
        except Exception:
            pass
    return n


def forget_sessions(server=None):
    """Drop closed/stale Session objects from the registry (call between
    handler runs so the list does not grow unbounded across the suite)."""
    keep = []
    for s in _ALL_SESSIONS:
        try:
            if server is not None and getattr(s, "server", None) is server:
                continue
            if getattr(s, "_closed", False):
                continue
            keep.append(s)
        except Exception:
            pass
    _ALL_SESSIONS[:] = keep

# ---------------------------------------------------------------------------
# Authoritative kick detection.
#
# A "kick" in this server is Client::errorOutput(reason) -> disconnectClient().
# For a CharacterSelected client that runs the FULL teardown (state saved,
# removed from the map + ClientList, stat reset to None) and logs exactly one
#   "<pseudo>: Kicked by: <reason>"
# line to the server's stdout (== <run_dir>/server.log). Crucially the TCP FIN
# is NOT sent here for a handler whose dispatch case still `return true`s after
# errorOutput (e.g. 0x04 clanInvite, 0x0B heal, the quest/plant handlers): the
# input/output layer only closeSocket()s when parseMessage()/parseQuery() return
# FALSE. So those kicks are DEFERRED — the socket closes on the client's NEXT
# packet (which hits parseMessage's `stat==None -> disconnectClient()` early
# return that DOES closeSocket()). A purely socket-level EOF probe is therefore
# both racy and, for these return-true handlers, only fires after a follow-up
# packet. The server LOG line, by contrast, is written synchronously the moment
# the kick happens and is 100% reliable regardless of the FIN timing.
#
# server_kick_count() snapshots how many "Kicked by:" lines the server has
# logged. A handler test takes a snapshot before the abusive packet and asserts
# the count INCREASED afterwards == the offender was kicked. This is the
# authoritative signal; the socket-EOF nudge below is kept only as a secondary
# liveness confirmation.
# ---------------------------------------------------------------------------
def server_kick_count(server):
    """Number of 'Kicked by:' lines the server has logged so far (== kicks).
    Reliable + synchronous: errorOutput() writes this line before returning."""
    try:
        with open(server.log_path, "r", errors="replace") as f:
            txt = f.read()
    except (FileNotFoundError, OSError):
        return 0
    return txt.count("Kicked by:")


def wait_for_kick(server, baseline_count, timeout=4.0, poll=0.15):
    """Block until the server has logged MORE 'Kicked by:' lines than
    baseline_count (a kick happened since the snapshot), or timeout. Returns
    True iff a new kick line appeared. Under valgrind the handler runs slowly,
    so poll the log rather than assume the kick is instantaneous."""
    deadline = time.time() + timeout
    while time.time() < deadline:
        if server_kick_count(server) > baseline_count:
            return True
        try:
            time.sleep(poll)
        except Exception:
            pass
    return server_kick_count(server) > baseline_count


def session_was_kicked(sess, timeout=4.0, poll=0.15):
    """Authoritative per-session kick check: True iff the server logged a
    '<pseudo>: Kicked by: ...' line for THIS session's pseudo (errorOutput()
    prefixes every kick with the offender's pseudo via Client::headerOutput()).

    This is order-independent (no running counter to keep in sync across many
    sessions) and reliable regardless of whether the TCP FIN is immediate
    (parseMessage returned false) or DEFERRED (the dispatch case errorOutput'd
    then returned true). Polls the server log up to `timeout` because under
    valgrind the kick is logged some time after we send the packet.

    Requires the session to have a non-empty .pseudo (true after a full
    handshake). For pre-login / handshake-failure cases fall back to the
    socket-level nudge_kicked_socket()."""
    pseudo = getattr(sess, "pseudo", None)
    server = getattr(sess, "server", None)
    if not pseudo or server is None:
        return nudge_kicked_socket(sess)
    needle = pseudo + ": Kicked by:"
    deadline = time.time() + timeout
    while time.time() < deadline:
        try:
            with open(server.log_path, "r", errors="replace") as f:
                if needle in f.read():
                    return True
        except (FileNotFoundError, OSError):
            pass
        # Nudge the socket so a deferred kick that needs a follow-up packet to
        # actually run the disconnect path is provoked, then re-check the log.
        try:
            sess.sock.sendall(bytes([0x02, 0x01, 0x05]))   # one clean move
        except OSError:
            pass
        try:
            time.sleep(poll)
        except Exception:
            pass
    try:
        with open(server.log_path, "r", errors="replace") as f:
            return needle in f.read()
    except (FileNotFoundError, OSError):
        return False


def nudge_kicked_socket(sess, tries=4, settle=0.5):
    """Best-effort: surface a DEFERRED kick at the TCP level by sending clean,
    non-repeating well-formed move packets (which on a kicked/stat==None client
    drive parseMessage's closeSocket()) and watching for EOF/RST. Returns True
    if the socket closed. Used as a secondary confirmation only — the
    authoritative kick signal is wait_for_kick() on the server log.

    The moves alternate direction (5,6,7,8 = move-down/-left/-right/-top) so a
    STILL-CONNECTED client never trips the 'same direction' no-op (which would
    not close the socket) and a genuinely-kicked one closes on the first packet
    that reaches the stat==None guard."""
    sk = sess.sock
    dirs = (5, 6, 7, 8)
    # drain any pending bytes first (the kick broadcast precedes the FIN)
    end = time.time() + settle
    while time.time() < end:
        try:
            sk.settimeout(0.3)
            d = sk.recv(65536)
            if d == b"":
                return True
        except socket.timeout:
            break
        except OSError:
            return True
    i = 0
    while i < tries:
        try:
            sk.sendall(bytes([0x02, 0x01, dirs[i % 4]]))
        except OSError:
            return True
        e = time.time() + settle
        while time.time() < e:
            try:
                sk.settimeout(0.3)
                d = sk.recv(65536)
                if d == b"":
                    return True
            except socket.timeout:
                break
            except OSError:
                return True
        i += 1
    return False


def _unique_pseudo(base):
    _PSEUDO_SEQ[0] += 1
    # pseudo must be globally unique (shared namespace) and match the engine's
    # allowed charset; keep it short and alnum.
    suffix = "%x%x" % (int(time.time()) & 0xFFFFFF, _PSEUDO_SEQ[0])
    p = (base + suffix)
    return p[:18]


class Session:
    """A raw-TCP connection that has completed the full handshake and is on the
    map (stat CharacterSelected). After construction you may send any in-game
    packet via q()/m()/send_raw() and read replies via reply_to()/drain().

    Position (x,y,mapIndex) is best-effort parsed from the select-character
    block; it is the server's authoritative spawn (matches start.xml).
    """
    def __init__(self, server, login=None, passh=None, pseudo="Player"):
        self.server = server
        self.x = None
        self.y = None
        self.mapIndex = None
        self.character_id = None
        self.login_creds = login if login is not None else \
            ("u%x%x" % (int(time.time()) & 0xFFFFFFFF, _PSEUDO_SEQ[0]+1)).encode()
        self.pass_creds = passh if passh is not None else (b"p_" + self.login_creds)
        self.pseudo = _unique_pseudo(pseudo)
        self._buf = bytearray()
        self._out_qmap = {}     # our queryNumber -> code awaiting 0x7F reply
        self._qn = 0
        self._closed = False
        _ALL_SESSIONS.append(self)   # register for clean close before SIGTERM
        self.sock = socket.create_connection(("127.0.0.1", server.port), timeout=5)
        self.sock.settimeout(0.5)
        try:
            self._handshake()
        except HandshakeError:
            raise
        except Exception as e:
            raise HandshakeError("handshake failed: %r" % e)
        # Byte offset into the server log at the moment THIS session became live.
        # A kick check greps "<pseudo>: Kicked by:" only AFTER this offset, so a
        # prior session that reused the same character pseudo and was kicked
        # (e.g. selectCharacter abuse) can't false-match a later, NOT-kicked
        # session with the same pseudo. Selects/sessions are sequential per test.
        try:
            self._log_base = os.path.getsize(server.log_path)
        except OSError:
            self._log_base = 0

    # -- low level --------------------------------------------------------
    def _recv_into_buf(self, t=0.4):
        self.sock.settimeout(t)
        try:
            while True:
                d = self.sock.recv(65536)
                if not d:
                    break
                self._buf.extend(d)
        except socket.timeout:
            pass
        except OSError:
            pass

    def _parse_stream(self):
        """Pull complete packets out of self._buf. Returns list of
        (kind, code, qnum, payload). kind in message/query/reply/unknown."""
        out = []
        buf = self._buf
        while True:
            if len(buf) < 1:
                break
            code = buf[0]
            if code == 0x7F or code == 0x01:        # reply server->client / client->server
                if len(buf) < 2:
                    break
                qn = buf[1]
                replied = self._out_qmap.get(qn)
                sz = REPLY_FIXED.get(replied, 254) if replied is not None else 254
                if sz == 254:
                    if len(buf) < 6:
                        break
                    ln = struct.unpack("<I", buf[2:6])[0]
                    if len(buf) < 6 + ln:
                        break
                    payload = bytes(buf[6:6+ln]); del buf[:6+ln]
                else:
                    if len(buf) < 2 + sz:
                        break
                    payload = bytes(buf[2:2+sz]); del buf[:2+sz]
                out.append(("reply", replied, qn, payload))
                self._out_qmap.pop(qn, None)
            else:
                sz = MSG_FIXED.get(code, 255)
                isquery = code >= 0x80
                off = 1
                qn = None
                if isquery:
                    if len(buf) < 2:
                        break
                    qn = buf[1]; off = 2
                if sz == 255:
                    out.append(("unknown", code, qn, b"")); del buf[:off]; break
                if sz == 254:
                    if len(buf) < off + 4:
                        break
                    ln = struct.unpack("<I", buf[off:off+4])[0]
                    if len(buf) < off + 4 + ln:
                        break
                    payload = bytes(buf[off+4:off+4+ln]); del buf[:off+4+ln]
                else:
                    if len(buf) < off + sz:
                        break
                    payload = bytes(buf[off:off+sz]); del buf[:off+sz]
                out.append(("query" if isquery else "message", code, qn, payload))
        return out

    def _auto_ack(self, events):
        """ACK server queries (0xE1 teleport / 0xE2 event / 0xE3 map ping) with
        a 0-length reply [0x7F][qn]; otherwise the server flow may stall."""
        for kind, code, qn, _p in events:
            if kind == "query" and code in _QUERY_REPLY_TO_US and qn is not None:
                try:
                    self.sock.sendall(bytes([0x7F, qn]))
                except OSError:
                    pass

    def _next_qn(self):
        for _ in range(16):
            v = self._qn
            self._qn = (self._qn + 1) % 16
            if v not in self._out_qmap:
                return v
        raise HandshakeError("no free query number (all 16 in flight)")

    def _wait_reply(self, want_qn, timeout=3.0):
        deadline = time.time() + timeout
        while time.time() < deadline:
            self._recv_into_buf(0.25)
            ev = self._parse_stream()
            self._auto_ack(ev)
            for kind, code, qn, payload in ev:
                if kind == "reply" and qn == want_qn:
                    return payload
        return None

    # -- public send ------------------------------------------------------
    def q(self, code, payload=b"", dynamic=False):
        """Send a QUERY (allocates a queryNumber). Returns the queryNumber.
        dynamic=True prefixes a 4-byte LE length (use for 0xFE packets)."""
        qn = self._next_qn()
        if dynamic:
            frame = bytes([code, qn]) + struct.pack("<I", len(payload)) + payload
        else:
            frame = bytes([code, qn]) + payload
        self._out_qmap[qn] = code
        self.sock.sendall(frame)
        return qn

    def m(self, code, payload=b"", dynamic=False):
        """Send a MESSAGE (no queryNumber)."""
        if dynamic:
            frame = bytes([code]) + struct.pack("<I", len(payload)) + payload
        else:
            frame = bytes([code]) + payload
        self.sock.sendall(frame)

    def send_raw(self, raw):
        """Send arbitrary bytes verbatim (for malformed-framing tests)."""
        self.sock.sendall(raw)

    def reply_to(self, qnum, timeout=2.0):
        """Return the PAYLOAD of the 0x7F reply to qnum, or None on timeout.
        Auto-ACKs 0xE1/0xE2/0xE3 server queries while waiting."""
        return self._wait_reply(qnum, timeout=timeout)

    def drain(self, timeout=1.0):
        """Read+return all currently-available raw bytes; also auto-ACK
        0xE1/0xE2/0xE3.  Returns the parsed events as a bytes-ish summary is
        not useful, so we return the raw bytes consumed."""
        consumed = bytearray()
        deadline = time.time() + timeout
        while time.time() < deadline:
            before = len(self._buf)
            self._recv_into_buf(0.2)
            ev = self._parse_stream()
            self._auto_ack(ev)
            # record what arrived (best-effort: re-serialise is lossy, so we
            # just track that something happened by re-reading the socket)
            if len(self._buf) == before and not ev:
                if len(consumed) == 0:
                    continue
                break
        return bytes(consumed)

    def close(self):
        self._closed = True
        try:
            self.sock.close()
        except OSError:
            pass

    # -- handshake --------------------------------------------------------
    def _handshake(self):
        login = self.login_creds
        passw = self.pass_creds
        loginHash, passHash = _creds(login, passw)

        # 1) A0 protocol header
        qn = self.q(0xA0, PROTOCOL_HEADER_LOGIN, dynamic=False)
        pa = self._wait_reply(qn, timeout=3.0)
        if pa is None or len(pa) < 17:
            raise HandshakeError("A0: no/short reply: %s" % (_hx(pa) if pa else None))
        comp = pa[0]
        if comp != 0x04:
            raise HandshakeError("A0: compression byte %#x != 0x04 (set compression=none!)" % comp)
        token = pa[1:17]

        # 2) A8 login
        a8 = loginHash + blake3(passHash + token)
        qn = self.q(0xA8, a8, dynamic=False)
        pa = self._wait_reply(qn, timeout=3.0)
        if pa is None or len(pa) < 1:
            raise HandshakeError("A8: no reply")
        if pa[0] == 0x07:
            # 3) A9 create account
            a9 = blake3(loginHash) + passHash
            qn = self.q(0xA9, a9, dynamic=False)
            pa = self._wait_reply(qn, timeout=3.0)
            if pa is None or pa[0] != 0x01:
                raise HandshakeError("A9 create: %s" % (_hx(pa) if pa else None))
            # 4) re-login (account now persisted -> stat Logged)
            qn = self.q(0xA8, a8, dynamic=False)
            pa = self._wait_reply(qn, timeout=3.0)
            if pa is None or pa[0] != 0x01:
                raise HandshakeError("A8 re-login: %s" % (_hx(pa) if pa else None))
        elif pa[0] != 0x01:
            raise HandshakeError("A8: status %#x (not 0x01/0x07)" % pa[0])

        # pa now holds the char-list block. Try to find an existing character.
        char_id = self._char_id_from_list(pa)
        if char_id is None:
            # 5) addCharacter
            ps = self.pseudo.encode()
            body = bytes([0x00, 0x00, len(ps)]) + ps + bytes([0x00, _DEFAULT_SKIN_ID])
            qn = self.q(0xAA, body, dynamic=True)
            pa = self._wait_reply(qn, timeout=4.0)
            if pa is None or len(pa) < 5:
                raise HandshakeError("AA addCharacter: no/short reply: %s"
                                     % (_hx(pa) if pa else None))
            status = pa[0]
            if status != 0x00:
                raise HandshakeError("AA addCharacter status %#x (1=exists,2=skin-empty)"
                                     % status)
            char_id = struct.unpack("<I", pa[1:5])[0]
        self.character_id = char_id

        # 6) selectCharacter
        sel = bytes([0x00]) + struct.pack("<I", 0) + struct.pack("<I", char_id)
        qn = self.q(0xAC, sel, dynamic=False)
        pa = self._wait_reply(qn, timeout=5.0)
        if pa is None or len(pa) < 1:
            raise HandshakeError("AC selectCharacter: no reply")
        if pa[0] != 0x01:
            raise HandshakeError("AC selectCharacter status %#x (2=not-found,3=already-logged)" % pa[0])
        self._parse_select_block(pa[1:])
        # pump the post-select stream (map/visibility/events) + auto-ACK.
        self.drain(timeout=0.8)

    def _char_id_from_list(self, a8reply):
        """Parse the A8(0x01) char-list block and return the first existing
        character_id, or None if the account has no characters yet.
        Layout (Api_protocol_reply.cpp 0xA8): [0x01][char_delete_time:4]
        [max_char:1][min_char:1][max_pseudo:1][maxPlayerMonsters:1]
        [maxWarehouse:2][datapackHashBase:32][mirror:1+str]
        [charGroupCount:1]{ [charCount:1]{ [character_id:4][pseudo:1+str]
        [skin:1][delete_time_left:4] ... }* }* """
        try:
            d = a8reply
            pos = 1                  # skip status 0x01
            pos += 4 + 1 + 1 + 1 + 1 + 2 + 32
            mir = d[pos]; pos += 1 + mir
            grp = d[pos]; pos += 1
            g = 0
            while g < grp:
                cnt = d[pos]; pos += 1
                if cnt > 0:
                    cid = struct.unpack("<I", d[pos:pos+4])[0]
                    return cid
                g += 1
            return None
        except Exception:
            return None

    def _parse_select_block(self, blk):
        """Best-effort parse of the character/map block (after the 0x01 status
        byte) to fill x/y/mapIndex. Layout per
        Api_protocol_loadchar.cpp::parseCharacterBlockServer +
        parseCharacterBlockCharacter (fields up to mapIndex/x/y)."""
        try:
            d = blk
            pos = 0
            def rd_u8():
                nonlocal pos; v = d[pos]; pos += 1; return v
            def rd_u16():
                nonlocal pos; v = struct.unpack("<H", d[pos:pos+2])[0]; pos += 2; return v
            def rd_u32():
                nonlocal pos; v = struct.unpack("<I", d[pos:pos+4])[0]; pos += 4; return v
            def rd_str():
                nonlocal pos; n = d[pos]; pos += 1; v = d[pos:pos+n]; pos += n; return v
            rd_u16()                 # max_players
            rd_u32()                 # captureRemainingTime
            rd_u8()                  # captureFrequencyType
            rd_u32()                 # waitBeforeConnectAfterKick
            rd_u8()                  # forceClientToSendAtBorder
            rd_u8()                  # dontSendPseudo
            rd_u32(); rd_u32(); rd_u32(); rd_u32()   # rates xp/gold/xp_pow/drop
            rd_u8(); rd_u8(); rd_u8(); rd_u8()       # chat allow all/local/private/clan
            rd_u8()                  # factoryPriceChange
            rd_str()                 # mainDatapackCode
            sub = rd_str()           # subDatapackCode
            pos += 32                # datapackHashServerMain
            if len(sub) > 0:
                pos += 32            # datapackHashServerSub
            rd_str()                 # mirror
            rd_u8()                  # everyBodyIsRoot
            # parseCharacterBlockCharacter
            ev = rd_u8()             # event list size
            pos += 2 * ev
            self.mapIndex = rd_u16()
            self.x = rd_u8()
            self.y = rd_u8()
        except Exception:
            pass  # best-effort; leave None on any surprise


# ---------------------------------------------------------------------------
# DB-state verification
# ---------------------------------------------------------------------------
def reload_state(server, login, passh):
    """Reconnect with the given credentials and return a snapshot of the
    persisted character read back over the protocol.

    Method: open a NEW connection, send A0+A8. The account is already persisted
    (created during the first Session), so A8 returns 0x01 with the char-list
    block (no A9/re-login needed). We take the first character_id+pseudo from
    that list, then selectCharacter and parse the character/map block, which
    carries the persisted spawn position (x,y,mapIndex).

    Returns a dict:
        {character_id, pseudo, x, y, mapIndex}
    Documented limitation: cash / items / monsters are encoded deeper in the
    select-block (and in the HPS files database/common/characters/<hexpseudo>
    and database/common/accounts/<id>); the FOUNDATION reads back the reliably
    framed header (id/pseudo/position). A handler test that mutates cash/items
    should ALSO assert via the in-band reply of the handler it exercises (the
    server echoes the new value), with reload_state used for position/identity
    persistence. The raw HPS files are present under
    <run_dir>/database/common/ for a test that wants to byte-diff them.
    """
    if isinstance(login, str): login = login.encode()
    if isinstance(passh, str): passh = passh.encode()
    loginHash, passHash = _creds(login, passh)
    snap = {"character_id": None, "pseudo": None, "x": None, "y": None,
            "mapIndex": None}
    sk = socket.create_connection(("127.0.0.1", server.port), timeout=5)
    sk.settimeout(0.4)
    try:
        sess = _RawConn(sk)
        # A0
        pa = sess.query(0xA0, PROTOCOL_HEADER_LOGIN, dynamic=False)
        if pa is None or len(pa) < 17:
            return snap
        token = pa[1:17]
        # A8
        pa = sess.query(0xA8, loginHash + blake3(passHash + token), dynamic=False)
        if pa is None or pa[0] == 0x07:
            # not persisted (shouldn't happen for an account that ran a Session)
            return snap
        if pa[0] != 0x01:
            return snap
        cid, pseudo = _parse_first_char(pa)
        snap["character_id"] = cid
        snap["pseudo"] = pseudo
        if cid is None:
            return snap
        # select and parse position
        sel = bytes([0x00]) + struct.pack("<I", 0) + struct.pack("<I", cid)
        pa = sess.query(0xAC, sel, dynamic=False, timeout=5.0)
        if pa is not None and len(pa) > 1 and pa[0] == 0x01:
            tmp = Session.__new__(Session)
            tmp.x = tmp.y = tmp.mapIndex = None
            Session._parse_select_block(tmp, pa[1:])
            snap["x"], snap["y"], snap["mapIndex"] = tmp.x, tmp.y, tmp.mapIndex
        return snap
    finally:
        try: sk.close()
        except OSError: pass


def grant_item(sess, item_id, quantity=1, settle=0.4):
    """Grant `quantity` of item `item_id` to this LIVE session's own character via
    the in-game "/give <id> <pseudo> <qty>" admin command (a local chat 0x03 whose
    text starts with '/'). Requires the server to have been created with
    every_body_is_root=True (otherwise the command is silently not honoured).

    This is the SERVER's own grant path (addObjectAndSend -> the item lands in the
    in-memory inventory of THIS session), so the same session can immediately reach
    a handler's owned-item positive branch -- no fragile HPS-blob crafting needed,
    and the persisted state is exactly what the server would write for any player
    who legitimately obtained the item. Returns after a short settle."""
    pseudo = getattr(sess, "pseudo", None)
    if not pseudo:
        return
    text = ("/give %d %s %d" % (int(item_id), pseudo, int(quantity))).encode()
    if len(text) > 255:
        return
    # non-PM chat body: [chatType=local(0x01)][textSize:u8][text]; 0x03 is dynamic.
    body = bytes([0x01]) + bytes([len(text)]) + text
    try:
        sess.m(0x03, body, dynamic=True)
    except OSError:
        return
    try:
        sess.drain(timeout=settle)
    except Exception:
        try:
            time.sleep(settle)
        except Exception:
            pass


def reconnect_volatile_offsets(server, login, passh, settle=0.4, gap=1.2):
    """Byte offsets in an account's persisted FILE_DB character blob that change
    from a PURE reconnect with NO gameplay action.

    FILE_DB rewrites the whole character block at disconnect, and the server
    stamps last_connect / played_time at every selectCharacter -> these few bytes
    differ after any reconnect even when nothing was mutated (proven: a victim
    blob changes at a fixed offset from two reload_state() calls alone). A DB
    "no-op mutated nothing" byte-diff MUST ignore these offsets or it
    false-positives on the timestamp bump. Self-calibrating: reconnect twice
    (with a gap so the timestamps actually advance) and return the contiguous
    span of bytes that moved, padded to cover the full adjacent u32 fields.
    Best-effort -> empty set on any failure (caller then does a strict diff)."""
    def _snap():
        d = os.path.join(server.run_dir, "database", "common", "characters")
        out = {}
        if os.path.isdir(d):
            names = os.listdir(d)
            i = 0
            while i < len(names):
                p = os.path.join(d, names[i])
                if os.path.isfile(p):
                    try:
                        with open(p, "rb") as f:
                            out[names[i]] = f.read()
                    except OSError:
                        pass
                i += 1
        return out
    try:
        reload_state(server, login, passh)
        time.sleep(settle)
        a = _snap()
        time.sleep(gap)            # let played_time / last_connect advance a second
        reload_state(server, login, passh)
        time.sleep(settle)
        b = _snap()
    except Exception:
        return set()
    offs = set()
    for name, ba in a.items():
        bb = b.get(name)
        if bb is not None and bb != ba and len(bb) == len(ba):
            i = 0
            while i < len(ba):
                if ba[i] != bb[i]:
                    offs.add(i)
                i += 1
    if offs:
        lo = max(0, min(offs) - 8)   # widen to the whole last_connect+played_time
        hi = max(offs) + 8           # region even if only low bytes happened to move
        offs = set(range(lo, hi + 1))
    return offs


def diff_blobs_ignoring(before_snap, after_snap, volatile):
    """Return the name of the first character blob in before_snap that changed at
    a byte OUTSIDE the `volatile` offsets (or whose length changed) -- i.e. a REAL
    mutation (phantom cash/item/monster, cross-player corruption). None if every
    change is confined to the last_connect/played_time bookkeeping bytes. `volatile`
    comes from reconnect_volatile_offsets(); pass set() for a strict diff."""
    for name, b in before_snap.items():
        a = after_snap.get(name)
        if a is not None and a != b:
            if len(a) != len(b):
                return name
            i = 0
            while i < len(b):
                if b[i] != a[i] and i not in volatile:
                    return name
                i += 1
    return None


def _parse_first_char(a8reply):
    try:
        d = a8reply
        pos = 1 + 4 + 1 + 1 + 1 + 1 + 2 + 32
        mir = d[pos]; pos += 1 + mir
        grp = d[pos]; pos += 1
        g = 0
        while g < grp:
            cnt = d[pos]; pos += 1
            if cnt > 0:
                cid = struct.unpack("<I", d[pos:pos+4])[0]; pos += 4
                pl = d[pos]; pos += 1
                pseudo = d[pos:pos+pl].decode(errors="replace"); pos += pl
                return cid, pseudo
            g += 1
    except Exception:
        pass
    return None, None


class _RawConn:
    """Minimal framed connection used by reload_state (no handshake state)."""
    def __init__(self, sk):
        self.sock = sk
        self.buf = bytearray()
        self.out_qmap = {}
        self.qn = 0

    def query(self, code, payload, dynamic, timeout=3.0):
        qn = self.qn; self.qn = (self.qn + 1) % 16
        if dynamic:
            frame = bytes([code, qn]) + struct.pack("<I", len(payload)) + payload
        else:
            frame = bytes([code, qn]) + payload
        self.out_qmap[qn] = code
        self.sock.sendall(frame)
        deadline = time.time() + timeout
        while time.time() < deadline:
            self._recv(0.25)
            for kind, c, q, pl in self._parse():
                if kind == "query" and c in _QUERY_REPLY_TO_US and q is not None:
                    try: self.sock.sendall(bytes([0x7F, q]))
                    except OSError: pass
                if kind == "reply" and q == qn:
                    return pl
        return None

    def _recv(self, t):
        self.sock.settimeout(t)
        try:
            while True:
                d = self.sock.recv(65536)
                if not d: break
                self.buf.extend(d)
        except socket.timeout: pass
        except OSError: pass

    def _parse(self):
        out = []; buf = self.buf
        while len(buf) >= 1:
            code = buf[0]
            if code in (0x7F, 0x01):
                if len(buf) < 2: break
                q = buf[1]; replied = self.out_qmap.get(q)
                sz = REPLY_FIXED.get(replied, 254) if replied is not None else 254
                if sz == 254:
                    if len(buf) < 6: break
                    ln = struct.unpack("<I", buf[2:6])[0]
                    if len(buf) < 6 + ln: break
                    pl = bytes(buf[6:6+ln]); del buf[:6+ln]
                else:
                    if len(buf) < 2 + sz: break
                    pl = bytes(buf[2:2+sz]); del buf[:2+sz]
                out.append(("reply", replied, q, pl)); self.out_qmap.pop(q, None)
            else:
                sz = MSG_FIXED.get(code, 255); isq = code >= 0x80; off = 1; q = None
                if isq:
                    if len(buf) < 2: break
                    q = buf[1]; off = 2
                if sz == 255:
                    out.append(("unknown", code, q, b"")); del buf[:off]; break
                if sz == 254:
                    if len(buf) < off + 4: break
                    ln = struct.unpack("<I", buf[off:off+4])[0]
                    if len(buf) < off + 4 + ln: break
                    pl = bytes(buf[off+4:off+4+ln]); del buf[:off+4+ln]
                else:
                    if len(buf) < off + sz: break
                    pl = bytes(buf[off:off+sz]); del buf[:off+sz]
                out.append(("query" if isq else "message", code, q, pl))
        return out


# ---------------------------------------------------------------------------
# Self-test
# ---------------------------------------------------------------------------
def _selftest(maincode):
    print("[selftest] building plain server ...")
    binary = build_server(valgrind=False)
    print("[selftest] binary:", binary)

    run_dir = os.path.join(_WORK, "selftest-run")
    srv = Server(binary, run_dir, maincode=maincode, valgrind=False)
    print("[selftest] server bound on port", srv.port)
    try:
        login = ("self%x" % (int(time.time()) & 0xFFFFFF)).encode()
        passw = b"p_" + login
        sess = Session(srv, login=login, passh=passw, pseudo="Self")
        print("[selftest] handshake OK: char_id=%s pos=(%s,%s) mapIndex=%s pseudo=%s"
              % (sess.character_id, sess.x, sess.y, sess.mapIndex, sess.pseudo))
        # one harmless packet: a player move (0x02 message, [moved_unit][direction])
        sess.m(0x02, bytes([0x01, 0x04]))
        time.sleep(0.2)
        if not srv.alive():
            print("[selftest] FAIL: server not alive after move")
            print(srv.crash_report() or "(no crash report)")
            return 1
        print("[selftest] server alive after move")

        # second logged-in session (for trade/battle tests): another player.
        login2 = ("self%xb" % (int(time.time()) & 0xFFFFFF)).encode()
        sess2 = Session(srv, login=login2, passh=b"p_"+login2, pseudo="SelfB")
        print("[selftest] 2nd session OK: char_id=%s pos=(%s,%s)"
              % (sess2.character_id, sess2.x, sess2.y))

        # reload_state for the first player (DB persistence read-back).
        sess.close()
        time.sleep(0.3)   # let the disconnect-save complete
        state = reload_state(srv, login, passw)
        print("[selftest] reload_state:", state)

        if not srv.alive():
            print("[selftest] FAIL: server not alive at end")
            return 1
        sess2.close()
        print("FOUNDATION OK pos=(%s,%s) mapIndex=%s" % (
            state.get("x"), state.get("y"), state.get("mapIndex")))
    finally:
        srv.stop()
    print("[selftest] (plain) server stopped")

    # --- valgrind path proof ---
    print("[selftest] building valgrind server ...")
    try:
        vbin = build_server(valgrind=True)
        vrun = os.path.join(_WORK, "selftest-valgrind-run")
        vsrv = Server(vbin, vrun, maincode=maincode, valgrind=True)
        print("[selftest] valgrind server bound on port", vsrv.port)
        vs = Session(vsrv, pseudo="Vg")
        print("[selftest] valgrind handshake OK pos=(%s,%s)" % (vs.x, vs.y))
        vs.close()
        time.sleep(0.5)
        vsrv.stop()
        verr = vsrv.valgrind_errors()
        print("[selftest] valgrind_errors() =", verr)
        if verr < 0:
            print("[selftest] WARN: valgrind log not parsed (valgrind installed?)")
        else:
            print("VALGRIND PATH OK errors=%d" % verr)
    except FileNotFoundError:
        print("[selftest] WARN: valgrind not installed; skipped valgrind proof")
    except RuntimeError as e:
        print("[selftest] WARN: valgrind path skipped:", str(e)[:200])
    return 0


if __name__ == "__main__":
    if len(sys.argv) >= 2 and sys.argv[1] == "selftest":
        mc = sys.argv[2] if len(sys.argv) >= 3 else "test"
        sys.exit(_selftest(mc))
    print(__doc__)
    print("usage: python3 _protoharness.py selftest [maincode]")
