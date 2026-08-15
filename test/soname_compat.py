"""soname_compat.py -- system-lib compatibility between a COMPILE node and
the EXECUTION nodes its binaries are pushed to.

CCCommon.cmake defaults every EXTERNALLIB* to ON ("system lib first": the
compile node's sysroot is meant to be a copy of the exec hardware, so its
.so is the one the device actually runs). That holds for a container/board
pair of the same vintage, but NOT when a newer board is paired with an older
compile node -- the geode container ships tinyxml2 soname libtinyxml2.so.10
while the p1mmx board ships .so.11, so the binary built there died on start
with "error while loading shared libraries: libtinyxml2.so.10".

Detected BEFORE the build, that lib (and only that lib, for that node only)
is built from the vendored in-tree copy instead. Everything else keeps
linking the system .so, so the "prefer the target's own library" policy is
unchanged wherever it actually holds.

Shared by benchmark/benchmark_remote.py and test/remote_build.py: each
passes its own ssh runner (a callable(cmd, timeout) -> (rc, stdout)), so
this module depends on neither harness and needs no ssh layer of its own.
"""
import threading


# Printed as the LAST line of every probe. Presence is the only proof the
# script ran to completion on the far side: callers' ssh wrappers differ
# (one separates stderr, the other merges it into stdout), and the probe's
# own exit status is that of its last `[ -n "$f" ]` test, so neither the
# emptiness of the output nor its rc can distinguish "this box has none of
# these libs" from "ssh printed 'No route to host'". Without the sentinel an
# offline board reported every lib missing and would have forced the whole
# fleet onto vendored copies.
_OK = "__soname_probe_ok__"

# (lib base name as `-l<name>` sees it, CMake option that forces vendored)
VENDORABLE_LIBS = (("tinyxml2", "EXTERNALLIBTINYXML2"),
                   ("blake3",   "EXTERNALLIBBLAKE3"),
                   ("xxhash",   "EXTERNALLIBXXHASH"),
                   ("zstd",     "EXTERNALLIBZSTD"),
                   ("z",        "EXTERNALLIBZLIB"))

_CACHE = {}
_LOCK = threading.Lock()


def _cached(key, produce):
    """Memoise `produce()` under `key` -- one ssh probe per node per run,
    even when a dozen build targets ask for the same answer."""
    with _LOCK:
        got = _CACHE.get(key)
    if got is None:
        got = produce()
        with _LOCK:
            _CACHE[key] = got
    return got


def compile_node_sonames(run_ssh, label):
    """{lib name -> the soname this compile node's linker would record}.

    `gcc -print-file-name=libX.so` resolves exactly what `-lX` picks up, and
    readelf/objdump read DT_SONAME out of it. The FILE name is not a
    substitute: blake3 1.8.5 still carries soname libblake3.so.0, so
    comparing version numbers would force a needless vendored build. A lib
    the node doesn't have -- or a probe that fails -- is left out of the
    dict: absent means "no opinion", never "force vendored"."""
    def _probe():
        names = " ".join(n for n, _opt in VENDORABLE_LIBS)
        cmd = ('for n in %s; do '
               'p=$(gcc -print-file-name=lib$n.so 2>/dev/null); '
               'case "$p" in /*) ;; *) continue;; esac; '
               's=$(readelf -d "$p" 2>/dev/null | '
               'sed -n "s/.*SONAME.*\\[\\(.*\\)\\].*/\\1/p" | head -1); '
               'if [ -z "$s" ]; then s=$(objdump -p "$p" 2>/dev/null | '
               'sed -n "s/^[ \\t]*SONAME[ \\t]*//p" | head -1); fi; '
               '[ -n "$s" ] && echo "$n=$s"; done; echo %s') % (names, _OK)
        rc, out = run_ssh(cmd, 30)
        found = {}
        if _OK not in (out or ""):
            return found        # probe never ran -> no opinion
        for line in (out or "").splitlines():
            if "=" in line:
                n, _sep, s = line.partition("=")
                n, s = n.strip(), s.strip()
                if n and s:
                    found[n] = s
        return found
    return _cached(("compile", label), _probe)


def present_sonames(run_ssh, label, sonames):
    """Subset of `sonames` the exec node's dynamic loader can actually find.

    Globs the standard lib dirs, falling back to the ldconfig cache for
    boxes with unusual paths. Fail-open: a probe that did not complete (the
    box is powered off, ssh timed out) reports every soname as present, so
    an unreachable board never triggers a needless vendored rebuild."""
    if not sonames:
        return set()
    wanted = sorted(sonames)

    def _probe():
        cmd = ('for s in %s; do '
               'f=$(ls /usr/lib/$s /usr/lib32/$s /usr/lib64/$s /lib/$s '
               '/lib32/$s /lib64/$s /usr/local/lib/$s /usr/lib/*-linux-gnu*/$s '
               '2>/dev/null | head -1); '
               'if [ -z "$f" ]; then '
               'f=$(ldconfig -p 2>/dev/null | grep -F "$s" | head -1); fi; '
               '[ -n "$f" ] && echo "$s"; done; echo %s') % (" ".join(wanted),
                                                             _OK)
        rc, out = run_ssh(cmd, 30)
        if _OK not in (out or ""):
            return set(wanted)      # probe never ran -> fail-open
        return set(t for t in out.split() if t != _OK)
    got = _cached(("exec", label, tuple(wanted)), _probe)
    return set(got)


def vendored_defs(compile_ssh, compile_label, exec_ssh, exec_label, log=None):
    """{CMake option -> "OFF"} for every optional lib whose compile-node
    soname is missing on the exec node. {} in the normal case (paired
    container/board of the same vintage), so an unaffected fleet builds
    exactly as before. `log` is called once per forced fallback -- the
    substitution is never silent."""
    def _probe():
        wanted = compile_node_sonames(compile_ssh, compile_label)
        present = present_sonames(exec_ssh, exec_label, set(wanted.values()))
        defs = {}
        for name, opt in VENDORABLE_LIBS:
            soname = wanted.get(name)
            if soname is not None and soname not in present:
                defs[opt] = "OFF"
                if log is not None:
                    log("%r: %s (linked by compile node %r) is absent on the "
                        "exec node -> building %s from the vendored copy "
                        "(-D%s=OFF)" % (exec_label, soname, compile_label,
                                        name, opt))
        return defs
    return dict(_cached(("pair", compile_label, exec_label), _probe))


def vendored_defs_for_node(compile_ssh, compile_label, exec_specs, log=None):
    """Same, for a compile node whose binary is SHARED by several exec nodes
    (the correctness harness builds once per node, not once per board): the
    union of what each exec node needs, so the one binary loads on all of
    them. `exec_specs` is a list of (exec_label, exec_ssh) pairs."""
    out = {}
    for exec_label, exec_ssh in exec_specs:
        out.update(vendored_defs(compile_ssh, compile_label,
                                 exec_ssh, exec_label, log=log))
    return out
