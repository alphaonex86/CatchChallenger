r"""qmake build-helper utilities for the test/ scripts.

Public entry points
-------------------

``resolvCtoO(build_dir)``
    Parse the qmake-generated Makefile in *build_dir* and return a
    ``{dep_path: [obj_path, …]}`` dict mapping every dependency the
    Makefile lists (sources AND headers) to the absolute paths of the
    .o files it contributes to.  Sources map to a single-entry list;
    headers map to every .o whose translation unit ``#include``\s them.

``find_files_referencing(macros, source_root)``
    Walk *source_root* and return the absolute paths of every C/C++
    source/header that references any of the given preprocessor *macros*
    via ``#ifdef`` / ``#ifndef`` / ``#if defined(…)`` / ``#elif`` lines.

``invalidate_objects_for_macros(build_dir, macros, source_root)``
    Combine the two above: when one of *macros* changes value across two
    qmake invocations against the same *build_dir*, run this to remove
    only the .o files whose translation unit (source OR any included
    header) references those macros.  A follow-up ``make -j$(nproc)``
    then rebuilds just the affected files instead of doing a costly
    ``make distclean`` + full rebuild.

Why this exists
---------------

When you re-run ``qmake`` with a different ``DEFINES+=FOO`` value, qmake
regenerates the Makefile but ``make`` does NOT rebuild stale .o files,
because the source / header mtimes have not changed.  Manually wiping
the affected .o files is the standard workaround; this module does it
automatically based on the actual qmake dependency graph.

Caveats
-------

* False positives are possible: a macro mentioned only inside a comment
  or string literal will still trigger invalidation.  That just causes
  one extra rebuild — never a wrong build.
* The Makefile dependency lines list the *direct* deps qmake/moc emit;
  transitive header chains are followed by qmake-generated ``depend``
  rules in modern qmake.  Older qmake outputs may miss some indirect
  headers — re-run a full ``make distclean`` if results look stale.
"""

import os
import re


# qmake compile rules look like:
#   <obj>.o: <dep> <dep> <dep>
# possibly continued across lines via trailing backslashes.  We prefilter
# on the ``*.o:`` form so linker / virtual / phony rules do not pollute
# the dep graph.
_OBJ_RULE_RE = re.compile(r"^\s*([^\s:]+\.o)\s*:\s*(.*)$")

_SOURCE_EXT = (".cpp", ".cc", ".cxx", ".c++", ".c")
_HEADER_EXT = (".h", ".hpp", ".hh", ".hxx", ".h++")


def _read_logical_lines(path):
    """Read *path* and return a list of logical lines (backslash-continued
    physical lines collapsed into one)."""
    out = []
    with open(path, "r", errors="replace") as f:
        cur = ""
        for raw in f:
            line = raw.rstrip("\n")
            if line.endswith("\\"):
                cur += line[:-1] + " "
            else:
                cur += line
                out.append(cur)
                cur = ""
        if cur:
            out.append(cur)
    return out


def resolvCtoO(build_dir):
    """Map every dependency listed in *build_dir*'s Makefile to the .o
    files that depend on it.

    Returns a ``dict[str, list[str]]`` whose keys are dependency paths
    verbatim from the Makefile (i.e. *relative to build_dir* — qmake
    typically emits e.g. ``../../general/base/CommonDatapack.cpp``) and
    whose values are absolute .o paths under *build_dir*.

    Returns an empty dict if *build_dir* has no ``Makefile`` or the file
    cannot be parsed."""
    mk = os.path.join(build_dir, "Makefile")
    if not os.path.isfile(mk):
        return {}
    try:
        lines = _read_logical_lines(mk)
    except OSError:
        return {}
    out = {}
    idx = 0
    while idx < len(lines):
        m = _OBJ_RULE_RE.match(lines[idx])
        idx += 1
        if not m:
            continue
        obj = m.group(1)
        obj_path = os.path.join(build_dir, obj)
        deps_str = m.group(2).strip()
        if not deps_str:
            continue
        parts = deps_str.split()
        pi = 0
        while pi < len(parts):
            dep = parts[pi]
            pi += 1
            if dep.endswith(_SOURCE_EXT) or dep.endswith(_HEADER_EXT):
                out.setdefault(dep, []).append(obj_path)
    return out


def find_files_referencing(macros, source_root, ext=None):
    """Return the absolute paths of every C/C++ source or header under
    *source_root* that mentions any of *macros* in a preprocessor
    directive (``#ifdef`` / ``#ifndef`` / ``#if`` / ``#elif``).

    *macros* is an iterable of macro names; *ext* defaults to all C/C++
    file extensions known to this module.

    ``build/`` and ``.git/`` directories are skipped to keep the walk
    cheap on a typical source tree."""
    if not macros:
        return set()
    if ext is None:
        ext = _SOURCE_EXT + _HEADER_EXT
    macro_alt = "|".join(re.escape(m) for m in macros)
    # Match any preprocessor directive that mentions the macro as a word.
    # `#if defined(MACRO)`, `#if !defined(MACRO)`, `#if MACRO == 1` etc.
    # all match because we just look for the macro as a \b-bounded token
    # inside an `#if/#ifdef/#ifndef/#elif` line.
    rx = re.compile(
        r"^\s*#\s*(?:if|ifdef|ifndef|elif)\b.*?\b(?:" + macro_alt + r")\b",
        re.MULTILINE,
    )
    found = set()
    for dirpath, _dirs, files in os.walk(source_root):
        if "/build/" in dirpath or "/.git" in dirpath:
            continue
        fi = 0
        while fi < len(files):
            fname = files[fi]
            fi += 1
            if not fname.endswith(ext):
                continue
            full = os.path.join(dirpath, fname)
            try:
                with open(full, "r", errors="replace") as f:
                    text = f.read()
            except OSError:
                continue
            if rx.search(text):
                found.add(os.path.abspath(full))
    return found


def invalidate_objects_for_macros(build_dir, macros, source_root):
    """Remove every .o under *build_dir* whose translation unit (source
    or any header it depends on per the Makefile) references one of
    *macros*.  Returns the list of removed absolute .o paths.

    Use after re-running ``qmake`` with a different ``DEFINES+=…`` value
    against the same *build_dir*: a follow-up ``make -j$(nproc)`` then
    rebuilds only the affected files instead of the whole tree.

    No-op when *macros* is empty, when the Makefile is missing, or when
    no .o actually depends on a referencing file."""
    if not macros:
        return []
    deps = resolvCtoO(build_dir)
    if not deps:
        return []
    referencing = find_files_referencing(macros, source_root)
    if not referencing:
        return []
    targets = set()
    items = list(deps.items())
    ii = 0
    while ii < len(items):
        dep, obj_list = items[ii]
        ii += 1
        ap = os.path.abspath(os.path.normpath(os.path.join(build_dir, dep)))
        if ap not in referencing:
            continue
        oj = 0
        while oj < len(obj_list):
            targets.add(obj_list[oj])
            oj += 1
    removed = []
    sorted_targets = sorted(targets)
    ti = 0
    while ti < len(sorted_targets):
        o = sorted_targets[ti]
        ti += 1
        try:
            os.unlink(o)
            removed.append(o)
        except FileNotFoundError:
            pass
        except OSError:
            # best-effort: an unremovable .o just gets rebuilt by make.
            pass
    return removed


def prepare_qmake_build_dir(build_dir, compiler_spec, cxx_version,
                             extra_defines, source_root):
    """State-aware cleanup hook for `build_project()`-style helpers in the
    testing*.py scripts.

    Compares (compiler_spec, cxx_version, extra_defines) against the values
    saved by the previous run in *build_dir*/.cc_qmake_state.txt and acts:

      * compiler_spec OR cxx_version differs ⇒ wipe every .o under
        *build_dir* and return ``"full"`` so the caller knows to also run
        ``make distclean`` + qmake fresh.
      * Only extra_defines differs ⇒ remove just the .o files affected
        by the macros that were added/removed (via
        ``invalidate_objects_for_macros``); return ``"incremental"``.
      * Nothing differs ⇒ return ``"unchanged"`` (caller can skip
        distclean entirely).
      * No previous state (first run) ⇒ return ``"first"``.

    State is rewritten in every case so the next run sees the current
    values."""
    state_file = os.path.join(build_dir, ".cc_qmake_state.txt")
    last_compiler = ""
    last_std = ""
    last_defines = ""
    if os.path.isfile(state_file):
        try:
            with open(state_file, "r") as f:
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
        except OSError:
            pass

    cur_compiler = compiler_spec or ""
    cur_std = cxx_version or ""
    cur_defines_set = set(extra_defines or [])
    cur_defines = " ".join(sorted(cur_defines_set))

    abi_changed = ((last_compiler and last_compiler != cur_compiler) or
                   (last_std and last_std != cur_std))

    decision = "first"
    if abi_changed:
        decision = "full"
        # Wipe every .o under build_dir.  Conservative: also remove the
        # qmake-generated Makefile so the caller's next qmake re-runs
        # cleanly even if `make distclean` is skipped.
        if os.path.isdir(build_dir):
            for dirpath, _dirs, files in os.walk(build_dir):
                fi = 0
                while fi < len(files):
                    fname = files[fi]
                    fi += 1
                    if fname.endswith(".o"):
                        try:
                            os.unlink(os.path.join(dirpath, fname))
                        except OSError:
                            pass
    elif last_compiler == "" and last_std == "" and last_defines == "":
        decision = "first"
    elif last_defines != cur_defines:
        decision = "incremental"
        last_set = set(last_defines.split()) if last_defines else set()
        changed = last_set.symmetric_difference(cur_defines_set)
        if changed:
            invalidate_objects_for_macros(build_dir, changed, source_root)
    else:
        decision = "unchanged"

    try:
        os.makedirs(build_dir, exist_ok=True)
        with open(state_file, "w") as f:
            f.write("compiler=" + cur_compiler + "\n")
            f.write("std=" + cur_std + "\n")
            f.write("defines=" + cur_defines + "\n")
    except OSError:
        pass
    return decision


def _cli():
    """`python3 qmake_helpers.py invalidate <build_dir> <macro> [<macro>...]`

    Prints removed .o paths one per line on stdout, then a one-line
    summary on stderr.  Exit code 0 on success, 2 on bad arguments."""
    import sys
    if len(sys.argv) < 4 or sys.argv[1] != "invalidate":
        print("usage: qmake_helpers.py invalidate <build_dir> <macro> [<macro>...]",
              file=sys.stderr)
        sys.exit(2)
    build_dir = sys.argv[2]
    macros = sys.argv[3:]
    source_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    removed = invalidate_objects_for_macros(build_dir, macros, source_root)
    ri = 0
    while ri < len(removed):
        print(removed[ri])
        ri += 1
    print(f"removed {len(removed)} .o file(s) referencing: {', '.join(macros)}",
          file=sys.stderr)


if __name__ == "__main__":
    _cli()
