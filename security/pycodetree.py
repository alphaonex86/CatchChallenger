#!/usr/bin/env python3
"""pycodetree.py — Python AST-based code tree (the codetree.py twin for Python).

codetree.py builds a function index + caller/callee graph from C++ LLVM IR.
This is the same idea for PYTHON, built with the stdlib `ast` module (no clang):
a function/method index + forward+reverse call graph, rendered as 5-depth
caller/callee trees in plain text. All work is IN CODE — the IA only receives the
pre-built tree text + the function body, slashing context per turn.

Used by:
  codecheck.py : general quality review (bugs/logic/duplication/name-vs-purpose),
                 walks each leaf function with its caller+callee view.

The whole point (mirrors codetree.py): give the IA, per function, exactly the
small slice it needs — the function source PLUS who calls it and what it calls —
instead of whole 2000-line files. Functions are SORTED leaves-first so a reviewer
sees callees before their callers.

Resolution note: Python calls are by NAME and dynamic (`self.f()`, `m.f()`,
`f()`), so edges are a best-effort name match (same-class > same-module > global),
exactly as codetree.py's demangled-name match is best-effort for C++ overloads.

Imports: stdlib `ast`/`os`/`sys` only; `from common import _ts` when available.
"""

import ast
import os
import sys

try:
    from common import _ts            # optional: shared timestamp helper
except Exception:                     # standalone (no security/common.py on path)
    def _ts():
        import time
        return time.strftime("%H:%M %d/%m/%Y")

REPO_ROOT = "/home/user/Desktop/CatchChallenger/git"
# Default scope: the test/ harness (override with --scope or scope_dirs=).
DEFAULT_SCOPE = (os.path.join(REPO_ROOT, "test"),)
# Directories never worth indexing (caches, throwaway scratch).
SKIP_DIR_NAMES = ("__pycache__", ".git", ".claude")
# Names that are almost always stdlib/builtins or noise — never index edges to
# them even if a local function happens to share the name.
_NOISE_CALLEES = frozenset((
    "print", "len", "str", "int", "float", "bool", "list", "dict", "set",
    "tuple", "range", "open", "isinstance", "getattr", "setattr", "hasattr",
    "format", "sorted", "enumerate", "zip", "map", "filter", "min", "max",
    "sum", "abs", "repr", "super", "type", "id", "bytes", "bytearray",
    "join", "append", "split", "strip", "get", "items", "keys", "values",
    "write", "read", "format", "startswith", "endswith", "replace", "decode",
    "encode", "add", "pop", "extend", "update", "sort", "lower", "upper",
))


class FuncInfo:
    """One function/method extracted from a Python source file."""
    __slots__ = ("name", "qual_name", "file", "line", "end_line",
                 "kind", "class_name")

    def __init__(self, name, qual_name, file, line, end_line,
                 kind="function", class_name=""):
        self.name = name              # bare def name, e.g. "run_scan"
        self.qual_name = qual_name    # "<module>:<Class>.<name>" or "<module>:<name>"
        self.file = file
        self.line = line
        self.end_line = end_line
        self.kind = kind              # "function" | "method"
        self.class_name = class_name

    def __repr__(self):
        return "Func(%s @ %s:%d)" % (self.qual_name,
                                     os.path.basename(self.file), self.line)


def _module_name(path):
    return os.path.splitext(os.path.basename(path))[0]


class _CallCollector(ast.NodeVisitor):
    """Collect the simple callee names + call lines inside ONE function body."""
    def __init__(self):
        self.calls = []   # (callee_simple_name, line)

    def visit_Call(self, node):
        callee = None
        f = node.func
        if isinstance(f, ast.Name):
            callee = f.id
        elif isinstance(f, ast.Attribute):
            callee = f.attr
        if callee:
            self.calls.append((callee, getattr(node, "lineno", 0)))
        # recurse into args/nested calls
        self.generic_visit(node)


class Index:
    """Cross-file Python function index + call graph (built from AST)."""

    def __init__(self, scope_dirs=None):
        self.scope_dirs = tuple(scope_dirs) if scope_dirs else DEFAULT_SCOPE
        self.by_name = {}            # qual_name -> FuncInfo
        self._simple = {}            # simple name -> [qual_name, ...]
        self._raw_calls = {}         # qual_name -> [(callee_simple, line), ...]
        self._forward = {}           # caller_qual -> {callee_qual: [line,...]}
        self._reverse = {}           # callee_qual -> {caller_qual: [line,...]}
        self._built = False

    # -- collection ---------------------------------------------------------
    def _collect_sources(self):
        out = []
        seen = set()
        for d in self.scope_dirs:
            if not os.path.isdir(d):
                if d.endswith(".py") and os.path.isfile(d):
                    rp = os.path.realpath(d)
                    if rp not in seen:
                        seen.add(rp); out.append(rp)
                continue
            for root, dirs, names in os.walk(d):
                dirs[:] = [x for x in dirs if x not in SKIP_DIR_NAMES]
                for n in sorted(names):
                    if n.endswith(".py"):
                        rp = os.path.realpath(os.path.join(root, n))
                        if rp not in seen:
                            seen.add(rp); out.append(rp)
        return sorted(out)

    def _index_file(self, path):
        try:
            src = open(path, "r", errors="replace").read()
        except OSError:
            return
        try:
            tree = ast.parse(src, filename=path)
        except SyntaxError as exc:
            sys.stderr.write("  [pycodetree] syntax error %s: %s\n"
                             % (os.path.basename(path), exc))
            return
        mod = _module_name(path)

        def record(node, cls):
            name = node.name
            qual = "%s:%s.%s" % (mod, cls, name) if cls else "%s:%s" % (mod, name)
            fi = FuncInfo(name, qual, path, node.lineno,
                          getattr(node, "end_lineno", node.lineno),
                          "method" if cls else "function", cls)
            # First definition wins (a re-def at module scope is rare).
            if qual not in self.by_name:
                self.by_name[qual] = fi
                self._simple.setdefault(name, []).append(qual)
                cc = _CallCollector()
                for sub in node.body:
                    cc.visit(sub)
                self._raw_calls[qual] = cc.calls

        for node in tree.body:
            if isinstance(node, (ast.FunctionDef, ast.AsyncFunctionDef)):
                record(node, "")
            elif isinstance(node, ast.ClassDef):
                for sub in node.body:
                    if isinstance(sub, (ast.FunctionDef, ast.AsyncFunctionDef)):
                        record(sub, node.name)

    # -- edge resolution ----------------------------------------------------
    def _resolve(self, caller_qual, callee_simple):
        """Best-effort: same-class > same-module > unique-global > first-global.
        Returns a callee qual_name or None (unresolved/stdlib/noise)."""
        if callee_simple in _NOISE_CALLEES:
            return None
        cands = self._simple.get(callee_simple)
        if not cands:
            return None
        if len(cands) == 1:
            c = cands[0]
            return c if c != caller_qual else None
        caller = self.by_name.get(caller_qual)
        caller_mod = caller_qual.split(":", 1)[0]
        # 1) same class
        if caller and caller.class_name:
            for c in cands:
                fi = self.by_name[c]
                if fi.class_name == caller.class_name and c != caller_qual:
                    return c
        # 2) same module
        for c in sorted(cands):
            if c.split(":", 1)[0] == caller_mod and c != caller_qual:
                return c
        # 3) deterministic first global (cross-module call)
        for c in sorted(cands):
            if c != caller_qual:
                return c
        return None

    def build(self):
        if self._built:
            return
        sources = self._collect_sources()
        sys.stderr.write("[pycodetree] indexing %d files\n" % len(sources))
        for path in sources:
            self._index_file(path)
        # resolve every raw call to a callee qual + build both directions
        for caller_qual, calls in self._raw_calls.items():
            for callee_simple, line in calls:
                callee_qual = self._resolve(caller_qual, callee_simple)
                if callee_qual is None:
                    continue
                self._forward.setdefault(caller_qual, {}).setdefault(
                    callee_qual, []).append(line)
                self._reverse.setdefault(callee_qual, {}).setdefault(
                    caller_qual, []).append(line)
        self._built = True
        sys.stderr.write("[pycodetree] %d functions, %d call edges\n"
                         % (len(self.by_name),
                            sum(len(v) for v in self._forward.values())))

    # -- queries ------------------------------------------------------------
    def functions_in(self, path):
        self.build()
        rp = os.path.realpath(path)
        return sorted((f for f in self.by_name.values() if f.file == rp),
                      key=lambda f: f.line)

    def callees_of(self, qual, depth=5):
        self.build()
        seen = set()
        result = {}

        def walk(q, d):
            if d <= 0 or q in seen:
                return
            seen.add(q)
            for c, lines in self._forward.get(q, {}).items():
                result.setdefault(c, []).extend(lines)
                walk(c, d - 1)
        walk(qual, depth)
        return result

    def callers_of(self, qual, depth=5):
        self.build()
        seen = set()
        result = {}

        def walk(q, d):
            if d <= 0 or q in seen:
                return
            seen.add(q)
            for c, lines in self._reverse.get(q, {}).items():
                result.setdefault(c, []).extend(lines)
                walk(c, d - 1)
        walk(qual, depth)
        return result

    def all_functions(self):
        """Every function, SORTED by qual_name (deterministic)."""
        self.build()
        return [self.by_name[q] for q in sorted(self.by_name)]

    def leaves_first(self):
        """Functions in leaves-first order: a function appears AFTER every
        indexed function it calls (callees before callers). Cycles are broken
        deterministically by qual_name. This is the order to review in."""
        self.build()
        order = []
        state = {}   # qual -> 0 unvisited / 1 in-progress / 2 done

        def visit(q):
            st = state.get(q, 0)
            if st == 2:
                return
            if st == 1:                      # cycle edge — break it
                return
            state[q] = 1
            for c in sorted(self._forward.get(q, {})):
                visit(c)
            state[q] = 2
            order.append(q)

        for q in sorted(self.by_name):
            visit(q)
        return [self.by_name[q] for q in order]

    def count(self):
        self.build()
        return len(self.by_name)


# ---------------------------------------------------------------------------
# Tree renderer (deterministic, in code) — mirrors codetree.py.TreeRender
# ---------------------------------------------------------------------------
class TreeRender:
    @staticmethod
    def _rel(path):
        return os.path.relpath(path, REPO_ROOT)

    @staticmethod
    def callee_tree(idx, qual, depth=5):
        idx.build()
        lines = ["=== Callee tree for %s (depth %d) ===" % (qual, depth)]
        seen = set()

        def render(q, d, indent):
            if d <= 0 or q in seen:
                return
            seen.add(q)
            for c, cl in idx._forward.get(q, {}).items():
                fi = idx.by_name.get(c)
                loc = " (%s:%d)" % (TreeRender._rel(fi.file), fi.line) if fi else ""
                lines.append("%s> %s%s  [call lines %s]"
                             % (indent, c, loc,
                                ",".join(str(x) for x in sorted(set(cl))[:5])))
                render(c, d - 1, indent + "  ")
        render(qual, depth, "")
        if len(lines) == 1:
            lines.append("  (no callees within %d depth)" % depth)
        return "\n".join(lines)

    @staticmethod
    def caller_tree(idx, qual, depth=5):
        idx.build()
        lines = ["=== Caller tree for %s (depth %d) ===" % (qual, depth)]
        seen = set()

        def render(q, d, indent):
            if d <= 0 or q in seen:
                return
            seen.add(q)
            for c, cl in idx._reverse.get(q, {}).items():
                fi = idx.by_name.get(c)
                loc = " (%s:%d)" % (TreeRender._rel(fi.file), fi.line) if fi else ""
                lines.append("%s< %s%s  [call lines %s]"
                             % (indent, c, loc,
                                ",".join(str(x) for x in sorted(set(cl))[:5])))
                render(c, d - 1, indent + "  ")
        render(qual, depth, "")
        if len(lines) == 1:
            lines.append("  (no callers within %d depth — entry point/dead code?)" % depth)
        return "\n".join(lines)

    @staticmethod
    def source_body(fi):
        try:
            lines = open(fi.file, "r", errors="replace").readlines()
        except OSError:
            return ""
        return "".join(lines[fi.line - 1:fi.end_line])

    @staticmethod
    def func_summary(idx, qual, body=True, body_cap=12000):
        idx.build()
        fi = idx.by_name.get(qual)
        if not fi:
            return "[not in index: %s]" % qual
        parts = [
            "=== %s (%s:%d-%d) ===" % (qual, TreeRender._rel(fi.file),
                                       fi.line, fi.end_line),
            TreeRender.callee_tree(idx, qual),
            "",
            TreeRender.caller_tree(idx, qual),
        ]
        if body:
            src = TreeRender.source_body(fi)
            clipped = src[:body_cap]
            parts.append("\nBODY:\n" + clipped
                         + ("\n...[body truncated]..." if len(src) > body_cap else ""))
        return "\n".join(parts)


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------
def _main(argv):
    import json
    scope = None
    mode = "stats"
    arg = None
    i = 1
    while i < len(argv):
        a = argv[i]
        if a.startswith("--scope="):
            scope = a.split("=", 1)[1].split(",")
        elif a == "--list":
            mode = "list"
        elif a == "--leaves":
            mode = "leaves"
        elif a.startswith("--func="):
            mode, arg = "func", a.split("=", 1)[1]
        elif a.startswith("--file="):
            mode, arg = "file", a.split("=", 1)[1]
        elif a == "--json":
            mode = "json"
        elif a in ("-h", "--help"):
            sys.stdout.write(
                "pycodetree.py — Python caller/callee code tree\n"
                "  --scope=DIR[,DIR]   dirs/files to index (default: test/)\n"
                "  --list              list every function (sorted)\n"
                "  --leaves            list functions leaves-first\n"
                "  --func=NAME         full summary (body + caller/callee tree) for a qual\n"
                "  --file=PATH         summaries for every function in a file\n"
                "  --json              dump the whole graph as JSON\n")
            return 0
        i += 1
    idx = Index(scope_dirs=scope)
    idx.build()
    if mode == "stats":
        sys.stdout.write("functions: %d\n" % idx.count())
    elif mode == "list":
        for fi in idx.all_functions():
            sys.stdout.write("%s  (%s:%d)\n"
                             % (fi.qual_name, TreeRender._rel(fi.file), fi.line))
    elif mode == "leaves":
        for fi in idx.leaves_first():
            sys.stdout.write("%s\n" % fi.qual_name)
    elif mode == "func":
        sys.stdout.write(TreeRender.func_summary(idx, arg) + "\n")
    elif mode == "file":
        for fi in idx.functions_in(arg):
            sys.stdout.write(TreeRender.func_summary(idx, fi.qual_name, body=False) + "\n\n")
    elif mode == "json":
        graph = {
            "functions": {q: {"file": TreeRender._rel(f.file), "line": f.line,
                              "end_line": f.end_line, "kind": f.kind,
                              "class": f.class_name}
                          for q, f in idx.by_name.items()},
            "forward": {q: {c: sorted(set(l)) for c, l in d.items()}
                        for q, d in idx._forward.items()},
        }
        sys.stdout.write(json.dumps(graph, indent=1, sort_keys=True) + "\n")
    return 0


if __name__ == "__main__":
    sys.exit(_main(sys.argv))
