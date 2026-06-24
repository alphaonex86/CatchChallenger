#!/usr/bin/env python3
"""secret.py - LLM-driven secret / leak scanner for this repository.

One job: before code reaches a PUBLIC GitHub, detect anything that must NOT be
published - tokens / API keys, login+password pairs, private API URLs, public
(globally-routable) IPv4/IPv6 addresses, and INTERNAL filesystem paths that leak
this developer's environment (a user home like /home/<name>, dev/scratch areas
like /mnt/<x>/..., /root/..., C:\\Users\\...).

Unlike security/server.py (a deep, agentic code AUDITOR that pulls in
counterpart/caller/related files for context), this tool is deliberately
SHALLOW and STANDALONE: it scans each file ON ITS OWN, never loading related
files. A secret is local to the line it sits on, so no cross-file context is
needed - that keeps it fast, cheap, and obviously correct.

Only CODE/TEXT files are scanned (an allow-list of extensions: .md .txt .cpp .h
.hpp .py .cmake .js .html .css .json .xml .sh .php ... + known text basenames
like Makefile/LICENSE/Dockerfile). Binary assets (png, xcf, opus, ogg, zip,
fonts, objects, Tiled .tmx/.tsx map data, ...), the .git folder, and vendored
third-party trees are skipped. Extend the allow-list via the out-of-repo config
keys 'text_extensions' / 'text_filenames'.

It shares the IA backend (Ollama by default; Claude when CC_IA_BACKEND=claude)
with server.py via common.py - same env contract, same credentials. Three ways
to reach an Ollama model, selected by OLLAMA_HOST (see common.py for detail):
  local  : (default) http://127.0.0.1:11434, model gemma4:26b
  remote : a remote Ollama daemon, OLLAMA_HOST=http://[<ipv6>]:11434
  router : an Ollama-compatible PHP endpoint that routes to the optimized
           instance (recommended for shared use)
The remote/router URL embeds infra IP - keep it OUT of the repo in
~/.config/CatchChallenger/ia-settings.json ({"ollama_host": "..."}); never commit.
CC_SECRET_MODEL overrides the Ollama model just for this tool (default gemma4:26b).

Two detection layers, merged per file:
  1. DETERMINISTIC  - regex for the well-known internal-path shapes (high recall,
                      exact line numbers). Patterns + the allow-list come from a
                      config file OUTSIDE the repo (see below).
  2. IA / LLM       - the shared backend, asked to flag tokens, passwords,
                      private API URLs and any odd path the regex missed. This is
                      the headline detector for the fuzzy classes a regex cannot
                      catch reliably.

CONFIG LIVES OUTSIDE THE REPO (so committing the repo never itself leaks the
allow-listed internal paths it lists). Default:
  $XDG_CONFIG_HOME/CatchChallenger/secret-scan.json  (override: CC_SECRET_CONFIG)
A starter template is written there on first run. Keys (all merged onto the
built-in defaults, never replacing them):
  internal_path_patterns : extra regexes whose matches are flagged as a leaked
                           internal path (e.g. your project's /mnt/data/perso).
  path_allowlist         : path PREFIX regexes (anchored) to NOT flag - system
                           dirs and any intended path (e.g. a deploy home).
  report_allowlist       : regexes that suppress ANY finding whose text matches
                           (public domains, placeholders, example values).
  file_exclude           : glob patterns of files to skip entirely.
  extra_hints            : free text appended to the LLM prompt.

Findings are printed to STDOUT only - nothing is written to disk by default. Set
CC_SECRET_REPORT=<path> to ALSO tee them to a file (keep it OUT of the repo; it
may quote redacted secrets + internal paths).

Usage:
  python3 secret.py                 # ALL checks over everything in git (default)
  python3 secret.py all             # file scope: tracked + untracked-not-ignored
  python3 secret.py staged          # file scope: only git-staged files (pre-commit)
  python3 secret.py changed         # file scope: working-tree changes + new untracked
  python3 secret.py path/to/file …  # file scope: only the given files

Detection categories - pass one or more to run ONLY those (default: all three):
  path     internal dev paths (regex, no LLM)
  ip       public IPv4/IPv6 (regex, no LLM)
  secret   tokens / passwords / API URLs / connection strings (LLM - backend REQUIRED)
A category and a file scope combine, e.g. `secret.py ip staged`, `secret.py path`.

If 'secret' is selected (or the default, which includes it) the LLM backend is
TESTED FIRST; if it is unreachable the run ABORTS with a loud error - it never
silently does a partial (regex-only) scan that would give false confidence. Run
only the regex checks explicitly with `secret.py ip path` when no LLM is wanted.
Exit code: 0 = clean, 2 = leaks found, 1 = error/abort.
"""

import concurrent.futures
import fnmatch
import ipaddress
import json
import os
import re
import subprocess
import sys
import time

import common  # shared IA backend transport (Ollama / Claude)
from common import _ts, _Tee, _human_dur, ensure_context, claude_usage_summary, chat

# ===========================================================================
# Where things live. REPO_ROOT is discovered (git toplevel, else this file's
# parent) rather than hard-coded, so this script carries NO literal /home/...
# path that would itself be a leak.
# ===========================================================================
HERE = os.path.dirname(os.path.realpath(__file__))


def _discover_repo_root():
    """The git work-tree root containing this script, else the parent dir of
    security/. Avoids embedding an absolute dev path in the source."""
    try:
        r = subprocess.run(["git", "-C", HERE, "rev-parse", "--show-toplevel"],
                           capture_output=True, text=True, timeout=15)
        if r.returncode == 0 and r.stdout.strip():
            return os.path.realpath(r.stdout.strip())
    except (OSError, subprocess.SubprocessError):
        pass
    return os.path.dirname(HERE)


REPO_ROOT = _discover_repo_root()

_XDG_CONFIG = os.environ.get("XDG_CONFIG_HOME") or os.path.expanduser("~/.config")
CONFIG_FILE = os.environ.get(
    "CC_SECRET_CONFIG",
    os.path.join(_XDG_CONFIG, "CatchChallenger", "secret-scan.json"))
# Findings go to stdout only by default (nothing written to disk). Set
# CC_SECRET_REPORT to a path to ALSO tee them to a file (keep it out of the repo).
REPORT_FILE = os.environ.get("CC_SECRET_REPORT", "").strip()

# Optional Ollama model override (ignored on the Claude backend, which uses
# CC_CLAUDE_MODEL). Routed through common's shared MODEL_NAME so common.chat()
# picks it up.
SECRET_MODEL = os.environ.get("CC_SECRET_MODEL", "").strip()
if SECRET_MODEL and not common.USE_CLAUDE:
    common.MODEL_NAME = SECRET_MODEL
    common.IA_LABEL = SECRET_MODEL

# ===========================================================================
# Secret-detection configuration (defaults + the out-of-repo config file).
# ===========================================================================
# Built-in INTERNAL-PATH shapes. Generic on purpose (no literal project path in
# this source, so the scanner never self-flags). The config file ADDS project
# specifics (e.g. a /mnt/data/perso scratch root). A match's text is the FULL
# leaked path (the trailing (?:/...)* captures sub-directories so the report
# shows the whole path, not just the home prefix); the path_allowlist matches the
# anchored prefix, so allow-listing /home/user still clears /home/user/anything.
# A real path segment must contain at least one ALPHANUMERIC char. This rejects
# placeholder/ellipsis segments like "/home/.../" written in comments/docs (all
# dots, no alnum) while still matching real names (user, first-world.info, _x).
_SEGA = r"[A-Za-z0-9._+-]*[A-Za-z0-9][A-Za-z0-9._+-]*"
_REST = r"(?:/[A-Za-z0-9._+-]+)*"   # zero or more further segments (full path)
DEFAULT_INTERNAL_PATTERNS = [
    r"/home/" + _SEGA + _REST,                 # any user home dir (full path)
    r"/root/" + _SEGA + _REST,                 # root's home
    r"/mnt/" + _SEGA + "/" + _SEGA + _REST,    # mounted dev / scratch areas
    r"/media/" + _SEGA + "/" + _SEGA + _REST,
    r"/Users/" + _SEGA + _REST,                # macOS home
    r"[A-Za-z]:\\Users\\" + _SEGA,             # Windows user dir
]
# Anchored path PREFIXES that are fine (matched from the start of the path):
# ordinary system locations. Add intended dev/deploy paths in the config file.
DEFAULT_PATH_ALLOWLIST = [
    r"/usr/", r"/etc/", r"/var/", r"/opt/", r"/tmp/", r"/bin/", r"/sbin/",
    r"/lib", r"/proc/", r"/sys/", r"/dev/", r"/run/", r"/boot/", r"/srv/",
]
# Regexes that suppress ANY finding whose text matches (public / placeholder).
DEFAULT_REPORT_ALLOWLIST = [
    r"catchchallenger\.herman-brule\.com",     # the project's PUBLIC site
    r"\b(?:localhost|127\.0\.0\.1|0\.0\.0\.0|::1)\b",
    r"example\.(?:com|org|net)",
    r"(?i)\b(?:your[_-]?|my[_-]?)?(?:token|secret|password|api[_-]?key)\b\s*[:=]\s*(?:<[^>]+>|xxx+|changeme|placeholder|todo)",
]
# Generic / placeholder credential VALUES that are NOT real secrets. The LLM is
# told to skip them, and a finding whose value is one of these is dropped as a
# backstop. Extend via the config 'weak_credentials'.
DEFAULT_WEAK_CREDENTIALS = [
    "admin", "administrator", "root", "user", "username", "login", "guest",
    "password", "passwd", "pass", "pwd", "secret", "token", "apikey", "key",
    "test", "testing", "example", "demo", "sample", "default", "changeme",
    "change-me", "none", "null", "todo", "foo", "bar", "baz", "123456",
    "12345678", "qwerty", "letmein", "password123", "string", "value",
]
# Words that are ALSO common in finding DESCRIPTIONS ("GitHub access token"),
# so the trailing-token backstop must NOT match on them - only on clear value
# placeholders (admin, login, test, ...). They still match as a QUOTED value.
_DESC_WORDS = {"password", "passwd", "pass", "pwd", "secret", "token", "apikey",
               "key", "api_key", "user", "username", "value", "string",
               "credential", "cred", "login"}
# Files never worth scanning (binary/asset/generated). Globs vs relpath + basename.
DEFAULT_FILE_EXCLUDE = [
    "*.min.js", "*.lock", "*.map",
    # The scanner's own pattern/documentation sources: by design they contain
    # example token shapes (ghp_, sk-ant-, AKIA...), example paths and IP regexes,
    # so scanning them yields only self-references, not real leaks.
    "security/secret.py", "security/common.py",
]

# Allowlist: ONLY code/text files are scanned. A file is scanned iff its
# extension is in TEXT_EXTS or its basename is in TEXT_FILENAMES; everything else
# (png, xcf, opus, ogg, zip, fonts, objects, ...) is skipped without reading it.
# An allowlist is safer than a binary denylist: an unknown binary type is skipped
# by default. Extend via the config 'text_extensions' / 'text_filenames'. Tiled
# map data (.tmx/.tsx) is deliberately omitted - it is base64 blob data, not source.
DEFAULT_TEXT_EXTS = {
    # docs / plain text
    ".md", ".markdown", ".rst", ".adoc", ".txt", ".text", ".log", ".csv", ".tsv",
    # c / c++ / objc
    ".c", ".cc", ".cpp", ".cxx", ".c++", ".h", ".hh", ".hpp", ".hxx", ".h++",
    ".inl", ".ipp", ".m", ".mm",
    # python
    ".py", ".pyw", ".pyi", ".pyx", ".pxd",
    # web / js / ts
    ".js", ".mjs", ".cjs", ".jsx", ".ts", ".vue", ".svelte", ".html", ".htm",
    ".xhtml", ".css", ".scss", ".sass", ".less",
    # data / config (text)
    ".json", ".json5", ".jsonc", ".xml", ".yaml", ".yml", ".toml", ".ini",
    ".cfg", ".conf", ".config", ".properties", ".env", ".plist",
    # build
    ".cmake", ".mk", ".make", ".ninja", ".pro", ".pri", ".gradle", ".bzl",
    ".gn", ".gni", ".pc", ".spec",
    # shell / scripts
    ".sh", ".bash", ".zsh", ".fish", ".ksh", ".ps1", ".bat", ".cmd", ".awk", ".sed",
    # other languages
    ".php", ".phtml", ".rb", ".erb", ".go", ".rs", ".java", ".kt", ".kts",
    ".scala", ".swift", ".lua", ".pl", ".pm", ".sql", ".r", ".jl", ".dart",
    ".groovy", ".clj", ".cljs", ".ex", ".exs", ".erl", ".hrl", ".hs", ".ml",
    ".mli", ".cs", ".vb", ".nim", ".zig", ".d", ".pas", ".asm", ".s",
    # qt ui / resources / shaders
    ".ui", ".qrc", ".qml", ".qss", ".glsl", ".vert", ".frag", ".geom", ".comp",
    ".hlsl", ".metal", ".wgsl",
    # misc text
    ".svg", ".tex", ".bib", ".diff", ".patch", ".desktop", ".service",
    ".proto", ".graphql", ".gql",
}
# Extensionless / dotfile text files worth scanning (basename match, lowercased).
DEFAULT_TEXT_FILENAMES = {
    "makefile", "gnumakefile", "cmakelists.txt", "dockerfile", "containerfile",
    "license", "licence", "copying", "copyright", "authors", "contributors",
    "notice", "readme", "changelog", "changes", "news", "todo", "install",
    ".gitignore", ".gitattributes", ".gitmodules", ".editorconfig",
    ".dockerignore", ".clang-format", ".clang-tidy", ".npmrc", ".prettierrc",
    ".eslintrc", ".bashrc", ".profile", ".zshrc",
}

# Vendored third-party trees (root CLAUDE.md do-not-modify list): not OUR code,
# so our secrets aren't there, and they are huge - skip to save tokens.
VENDOR_DIRS = tuple(os.path.realpath(os.path.join(REPO_ROOT, p)) for p in (
    "general/blake3",
    "general/hps",
    "general/libxxhash",
    "general/libzlib",
    "general/libzstd",
    "general/tinyXML2",
    "client/libqtcatchchallenger/libogg",
    "client/libqtcatchchallenger/libopus",
    "client/libqtcatchchallenger/libopusfile",
    "client/libqtcatchchallenger/libtiled",
))

# Per-file / per-chunk limits (env-tunable).
MAX_FILE_BYTES = int(os.environ.get("CC_SECRET_MAXFILEBYTES", str(1536 * 1024)))
CHUNK_LINES = int(os.environ.get("CC_SECRET_CHUNK_LINES", "300"))
MAX_LINE_CHARS = 4000          # truncate giant base64/data lines before sending
SCAN_MAX = int(os.environ.get("CC_SECRET_MAX", "0"))  # 0 = no cap
# Claude API streams cheaply in parallel (8); the CLI forks a full `claude`
# process per worker, so fewer (4) avoids rate-limit thrash; local Ollama 2.
if common.CLAUDE_VIA_CLI:
    DEFAULT_WORKERS = 4
elif common.USE_CLAUDE:
    DEFAULT_WORKERS = 8
else:
    DEFAULT_WORKERS = 2
try:
    WORKERS = max(1, int(os.environ.get("CC_SECRET_WORKERS",
                                        str(DEFAULT_WORKERS)) or DEFAULT_WORKERS))
except ValueError:
    WORKERS = DEFAULT_WORKERS

# Per-file scan progress is hidden by default (it buried the findings). Set
# CC_SECRET_PROGRESS=1 to print one stderr line per file (debugging). On a TTY a
# single in-place counter is shown instead; redirected stderr stays silent.
PROGRESS = os.environ.get("CC_SECRET_PROGRESS", "0").strip().lower() in ("1", "true", "yes", "on")
# Trailing SUMMARY block (count + re-listed file paths) - off by default (it just
# duplicated the per-file LEAKS headers); CC_SECRET_SUMMARY=1 restores it.
SHOW_SUMMARY = os.environ.get("CC_SECRET_SUMMARY", "0").strip().lower() in ("1", "true", "yes", "on")

# Retries per chunk when thinking is ON and the model returns no usable answer
# (a reasoning model sometimes burns its budget thinking and replies empty). 0
# disables. Ignored on the reliable think:false path.
try:
    THINK_RETRIES = max(0, int(os.environ.get("CC_SECRET_THINK_RETRIES", "3") or "3"))
except ValueError:
    THINK_RETRIES = 3

# Active detection categories (set from CLI args by main(); default = all):
#   path   -> det_path_findings (regex, no LLM)
#   ip     -> det_ip_findings   (regex, no LLM)
#   secret -> llm_findings      (tokens/passwords/API URLs - LLM, backend required)
# 'secret' (or the default-all) makes the LLM mandatory; run() tests the backend
# first and ABORTS loudly if it is down rather than scanning partially.
ALL_CATEGORIES = ("path", "ip", "secret")
CATEGORIES = set(ALL_CATEGORIES)
# Map CLI words (and a few aliases) to a category.
CATEGORY_ALIASES = {
    "path": "path", "paths": "path",
    "ip": "ip", "ips": "ip",
    "secret": "secret", "secrets": "secret", "token": "secret",
    "tokens": "secret", "llm": "secret",
}

# Compiled config, filled by load_config().
INTERNAL_RES = []
INTERNAL_PATTERN_STRS = []
PATH_ALLOW_RES = []
REPORT_ALLOW_RES = []
FILE_EXCLUDE = []
EXTRA_HINTS = ""
WEAK_CREDENTIALS = list(DEFAULT_WEAK_CREDENTIALS)
WEAK_SET = set(w.lower() for w in DEFAULT_WEAK_CREDENTIALS)          # quoted-value check
WEAK_VALUE_SET = WEAK_SET - _DESC_WORDS                             # trailing-token check
REDACT_SECRETS = False   # default: show real values so the owner can fix/rotate
TEXT_EXTS = set(DEFAULT_TEXT_EXTS)
TEXT_FILENAMES = set(DEFAULT_TEXT_FILENAMES)


def _compile(patterns, label):
    """Compile a list of regex strings, skipping (with a warning) any that don't
    compile - a hand-edited config must never abort the run."""
    out = []
    for p in patterns:
        try:
            out.append(re.compile(p))
        except re.error as exc:
            sys.stderr.write("    [config] bad %s regex %r: %s (skipped)\n"
                             % (label, p, exc))
    return out


def _dedup(seq):
    """Order-preserving de-duplication of a string list."""
    seen = set()
    out = []
    for s in seq:
        if s not in seen:
            seen.add(s)
            out.append(s)
    return out


def write_config_template(path):
    """Write a starter config (defaults pre-filled) to `path`, OUT of the repo.
    Best-effort: a failure here never aborts the scan."""
    template = {
        "_README": [
            "secret.py config - lives OUTSIDE the repo so it never leaks the",
            "internal paths it allow-lists. All lists below are MERGED onto the",
            "tool's built-in defaults (they add to, they do not replace).",
            "internal_path_patterns: regexes whose matches are flagged as a",
            "  leaked internal/dev path (e.g. your project scratch root).",
            "path_allowlist: anchored path PREFIX regexes that are fine (system",
            "  dirs + any intended deploy/dev path).",
            "report_allowlist: regexes suppressing ANY finding that matches",
            "  (public domains, placeholders, example values).",
            "file_exclude: glob patterns (vs repo-relative path or basename).",
            "text_extensions: EXTRA code/text extensions to scan (e.g. '.foo');",
            "  only allow-listed text/code files are scanned, binaries skipped.",
            "text_filenames: EXTRA extensionless text basenames to scan.",
            "weak_credentials: EXTRA generic login/password VALUES to NOT flag",
            "  (admin, login, password, test, ... are built in; add your own).",
            "redact_secrets: true masks credential values in the output; default",
            "  false shows the REAL value so you can locate and rotate it.",
            "extra_hints: free text appended to the LLM prompt.",
        ],
        "internal_path_patterns": [
            "/mnt/data/perso\\b",
        ],
        "path_allowlist": [],
        "report_allowlist": [],
        "file_exclude": [],
        "text_extensions": [],
        "text_filenames": [],
        "weak_credentials": [],
        "redact_secrets": False,
        "extra_hints": "",
    }
    try:
        os.makedirs(os.path.dirname(path), exist_ok=True)
        with open(path, "w") as fh:
            json.dump(template, fh, indent=2)
            fh.write("\n")
        sys.stderr.write("%s [config] wrote starter config (edit it): %s\n"
                         % (_ts(), path))
    except OSError as exc:
        sys.stderr.write("%s [config] could not write template %s: %s\n"
                         % (_ts(), path, exc))


def load_config():
    """Load the out-of-repo config (writing a template if absent) and compile
    the merged pattern lists into the module globals."""
    global INTERNAL_RES, INTERNAL_PATTERN_STRS, PATH_ALLOW_RES
    global REPORT_ALLOW_RES, FILE_EXCLUDE, EXTRA_HINTS, TEXT_EXTS, TEXT_FILENAMES
    global WEAK_CREDENTIALS, WEAK_SET, WEAK_VALUE_SET, REDACT_SECRETS
    data = {}
    if os.path.isfile(CONFIG_FILE):
        try:
            with open(CONFIG_FILE, "r", errors="replace") as fh:
                data = json.load(fh)
        except (OSError, ValueError) as exc:
            sys.stderr.write("%s [config] %s unreadable (%s); using defaults\n"
                             % (_ts(), CONFIG_FILE, exc))
            data = {}
        if not isinstance(data, dict):
            data = {}
        sys.stderr.write("%s [config] loaded %s\n" % (_ts(), CONFIG_FILE))
    else:
        write_config_template(CONFIG_FILE)

    def _list(key):
        v = data.get(key)
        return [s for s in v if isinstance(s, str)] if isinstance(v, list) else []

    INTERNAL_PATTERN_STRS = _dedup(DEFAULT_INTERNAL_PATTERNS + _list("internal_path_patterns"))
    INTERNAL_RES = _compile(INTERNAL_PATTERN_STRS, "internal_path_patterns")
    PATH_ALLOW_RES = _compile(_dedup(DEFAULT_PATH_ALLOWLIST + _list("path_allowlist")),
                              "path_allowlist")
    REPORT_ALLOW_RES = _compile(_dedup(DEFAULT_REPORT_ALLOWLIST + _list("report_allowlist")),
                                "report_allowlist")
    FILE_EXCLUDE = _dedup(DEFAULT_FILE_EXCLUDE + _list("file_exclude"))
    # Merge extra text extensions / filenames onto the built-in allowlist
    # (lowercased; a bare 'foo' extension is normalised to '.foo').
    TEXT_EXTS = set(DEFAULT_TEXT_EXTS)
    for e in _list("text_extensions"):
        e = e.strip().lower()
        if e:
            TEXT_EXTS.add(e if e.startswith(".") else "." + e)
    TEXT_FILENAMES = set(DEFAULT_TEXT_FILENAMES) | {n.strip().lower()
                                                    for n in _list("text_filenames") if n.strip()}
    WEAK_CREDENTIALS = _dedup(DEFAULT_WEAK_CREDENTIALS + _list("weak_credentials"))
    WEAK_SET = set(w.strip().lower() for w in WEAK_CREDENTIALS if w.strip())
    WEAK_VALUE_SET = WEAK_SET - _DESC_WORDS
    REDACT_SECRETS = bool(data.get("redact_secrets")) if isinstance(
        data.get("redact_secrets"), bool) else False
    EXTRA_HINTS = data.get("extra_hints") if isinstance(data.get("extra_hints"), str) else ""


# ===========================================================================
# File enumeration (via git: exactly what reaches GitHub).
# ===========================================================================
def _git_lines(args, zero=False):
    """Run `git <args>` in REPO_ROOT; return its output lines ([] on failure).
    zero=True splits NUL-separated output (-z), so odd/space filenames survive."""
    try:
        r = subprocess.run(["git", "-C", REPO_ROOT] + args,
                           capture_output=True, text=True, timeout=60)
    except (OSError, subprocess.SubprocessError) as exc:
        sys.stderr.write("    [git] %s failed: %s\n" % (" ".join(args), exc))
        return []
    if r.returncode != 0:
        sys.stderr.write("    [git] %s rc=%d: %s\n"
                         % (" ".join(args), r.returncode, r.stderr.strip()[:200]))
        return []
    if zero:
        return [s for s in r.stdout.split("\x00") if s]
    return [s for s in r.stdout.splitlines() if s.strip()]


def git_files(mode):
    """Absolute realpaths of the files for `mode`:
      all     - tracked + untracked-not-ignored (everything that reaches GitHub)
      staged  - the git index (pre-commit gate)
      changed - working-tree changes + new untracked files
    Deduped, order preserved."""
    if mode == "staged":
        rels = _git_lines(["diff", "--cached", "-z", "--name-only",
                           "--diff-filter=ACMR"], zero=True)
    elif mode == "changed":
        rels = _git_lines(["diff", "-z", "--name-only", "--diff-filter=ACMR"], zero=True)
        rels += _git_lines(["ls-files", "-z", "--others", "--exclude-standard"], zero=True)
    else:
        rels = _git_lines(["ls-files", "-z"], zero=True)
        rels += _git_lines(["ls-files", "-z", "--others", "--exclude-standard"], zero=True)
    out = []
    seen = set()
    for rel in rels:
        ap = os.path.realpath(os.path.join(REPO_ROOT, rel))
        if ap not in seen:
            seen.add(ap)
            out.append(ap)
    return out


def is_vendor(path):
    """True for a vendored third-party tree (never scanned)."""
    rp = os.path.realpath(path)
    return any(rp == d or rp.startswith(d + os.sep) for d in VENDOR_DIRS)


def _relpath(path):
    """Repo-relative path for display (absolute outside-repo path otherwise)."""
    try:
        rp = os.path.realpath(path)
        if rp.startswith(os.path.realpath(REPO_ROOT) + os.sep):
            return os.path.relpath(rp, REPO_ROOT)
    except OSError:
        pass
    return path


def _excluded(rel):
    """True if `rel` (or its basename) matches a file_exclude glob."""
    base = os.path.basename(rel)
    for pat in FILE_EXCLUDE:
        if fnmatch.fnmatch(rel, pat) or fnmatch.fnmatch(base, pat):
            return True
    return False


def _in_git_dir(path):
    """True if `path` is inside a .git directory (VCS metadata, never scanned)."""
    return ".git" in os.path.realpath(path).split(os.sep)


def _is_text_file(path):
    """True if `path` is an allow-listed code/text file (by extension or known
    basename); everything else (binary assets) is skipped."""
    base = os.path.basename(path).lower()
    if base in TEXT_FILENAMES:
        return True
    return os.path.splitext(base)[1] in TEXT_EXTS


def collect(mode, explicit):
    """Ordered list of files to scan. `explicit` (CLI file args) wins over git
    enumeration. Keeps ONLY allow-listed code/text files (binaries skipped),
    drops .git, vendor trees, excluded globs and oversized files; a null-byte
    test at read time is the final backstop."""
    if explicit:
        cand = [os.path.realpath(p) for p in explicit]
    else:
        cand = git_files(mode)
    out = []
    seen = set()
    for p in cand:
        if p in seen or not os.path.isfile(p):
            continue
        seen.add(p)
        rel = _relpath(p)
        if _in_git_dir(p) or is_vendor(p) or _excluded(rel):
            continue
        if not _is_text_file(p):          # not code/text -> skip (binary asset)
            continue
        try:
            if os.path.getsize(p) > MAX_FILE_BYTES:
                sys.stderr.write("    [skip big] %s (> %d bytes)\n" % (rel, MAX_FILE_BYTES))
                continue
        except OSError:
            continue
        out.append(p)
    if SCAN_MAX and len(out) > SCAN_MAX:
        sys.stderr.write("%s [INIT] CC_SECRET_MAX=%d: limiting to first %d of %d files\n"
                         % (_ts(), SCAN_MAX, SCAN_MAX, len(out)))
        out = out[:SCAN_MAX]
    return out


def read_text(path):
    """File text (utf-8, errors replaced), or None if unreadable / binary
    (a NUL byte in the content)."""
    try:
        with open(path, "rb") as fh:
            data = fh.read(MAX_FILE_BYTES + 1)
    except OSError:
        return None
    if b"\x00" in data:
        return None
    return data.decode("utf-8", "replace")


# ===========================================================================
# Detection.
# ===========================================================================
LINE_RE = re.compile(r"(?i)^\s*LINE\s+(\d+)\s*[:\-]\s*(.*)$")
# A model line that explicitly states the row is NOT a leak (chatty-model noise).
_NO_LEAK_RE = re.compile(r"(?i)\bno\s*(leak|secret)|not\s+a\s+(leak|secret)|"
                         r"\bclean\b|\bn/?a\b")

BASE_SYSTEM = (
    "You are a SECRET / CREDENTIAL LEAK detector. Files from a git repository "
    "are about to be pushed to a PUBLIC GitHub. Your only job is to find what "
    "must NOT become public.\n"
    "\n"
    "Your job is CREDENTIALS ONLY. Internal filesystem paths and public IP "
    "addresses are found by a separate exact-regex pass - do NOT report those "
    "here (you would only duplicate them, redacted, and bypass the allow-list).\n"
    "\n"
    "REPORT these leak classes (and only these):\n"
    "- Credentials & secrets: a HARDCODED password, secret key, API key, access "
    "token, OAuth/bearer token, JWT, session cookie, or a private key (a PEM "
    "'BEGIN ... PRIVATE KEY' block), and DB connection strings that embed a "
    "password.\n"
    "- Provider tokens by shape: AWS (AKIA...), GitHub (ghp_/gho_/ghs_/"
    "github_pat_), Slack (xox...), Stripe (sk_live_/rk_live_), Google (AIza...), "
    "Anthropic (sk-ant-...), OpenAI (sk-...), SendGrid (SG.), Twilio, npm, PyPI.\n"
    "- Login/password pairs: an explicit user + password, or basic-auth inside a "
    "URL (scheme://user:password@host).\n"
    "- Private / internal API endpoints: a non-public base URL, an internal "
    "hostname, an admin/staging endpoint, or any URL carrying an embedded "
    "credential or token query parameter.\n"
    "\n"
    "DO NOT report (these are NOT your job / NOT leaks):\n"
    "- Internal filesystem paths (/home/..., /mnt/..., /root/..., C:\\Users\\...) "
    "and public IP addresses - handled by the regex pass; never emit them.\n"
    "- Environment-variable NAMES or reads: os.environ[\"X\"], getenv(\"X\"), "
    "${VAR}, process.env.X, a config KEY name. Reading a secret from the "
    "environment is the SAFE pattern - never flag it.\n"
    "- Obvious placeholders / examples: YOUR_TOKEN, <token>, xxxx, changeme, "
    "sk-ant-api..., example.com, localhost, 127.0.0.1, clearly-test values.\n"
    "- Public, intended URLs (a project's public website/docs), license or "
    "copyright headers, SPDX ids.\n"
    "- Git commit SHAs, file checksums/hashes, UUIDs that are not credentials, "
    "public package/import/include names.\n"
    "\n"
    "OUTPUT FORMAT - your reply MUST be exactly one of two forms, nothing else:\n"
    "  (A) one line per leak:\n"
    "      LINE <n>: <CATEGORY> -> <why + the value>\n"
    "      CATEGORY is one of TOKEN, PASSWORD, PRIVATE_KEY, API_URL, "
    "CONNECTION_STRING, OTHER. <n> is the line number shown in the left margin.\n"
    "  (B) if nothing leaks:\n"
    "      No secrets identified\n"
    "Show the leaked VALUE IN FULL with its context, so the finding is actionable "
    "- this is the repo OWNER's local scan and they need the REAL value to locate "
    "and rotate it. No prose, no preamble, no summary - only LINE findings or the "
    "exact phrase above."
)


def _secret_system():
    """System prompt for the credential-only LLM pass (paths/IPs are deterministic
    and handled separately): the weak-credential exclude list, optional redaction,
    and any project hints."""
    out = BASE_SYSTEM
    if WEAK_CREDENTIALS:
        out += ("\n\nDo NOT flag a credential whose VALUE is a generic placeholder "
                "or common word (these are not real secrets): "
                + ", ".join(WEAK_CREDENTIALS) + ".")
    if REDACT_SECRETS:
        out += ("\n\nRedact each secret VALUE to its first 4 + last 2 chars "
                "(e.g. ghp_abcd...xy); never paste a full live credential.")
    if EXTRA_HINTS:
        out += "\n\nProject hints: " + EXTRA_HINTS
    return out


def _numbered(sublines, start):
    """Render lines with 1-based source line numbers in the left margin, long
    data lines truncated so a base64 blob can't blow the context."""
    out = []
    n = start
    for ln in sublines:
        if len(ln) > MAX_LINE_CHARS:
            ln = ln[:MAX_LINE_CHARS] + " …[line truncated]"
        out.append("%d: %s" % (n, ln))
        n += 1
    return "\n".join(out)


def _chunks_of(lines):
    """Yield (start_lineno, sublines) windows of CHUNK_LINES lines."""
    i = 0
    total = len(lines)
    while i < total:
        yield (i + 1, lines[i:i + CHUNK_LINES])
        i += CHUNK_LINES


def det_path_findings(lines):
    """Deterministic internal-path matches: (lineno, 'INTERNAL_PATH', path).
    Allow-listed (system / intended) paths are dropped here, precisely, by an
    anchored match against the bare path token."""
    out = []
    n = 0
    for ln in lines:
        n += 1
        for rx in INTERNAL_RES:
            for m in rx.finditer(ln):
                p = m.group(0)
                if not any(a.match(p) for a in PATH_ALLOW_RES):
                    out.append((n, "INTERNAL_PATH", p))
    return out


# IP-shaped tokens. The regexes are PERMISSIVE candidate-finders bounded so they
# do not match inside a longer dotted/hex run (version strings, MAC addresses,
# timestamps); the ipaddress module then VALIDATES each and decides public vs
# private. IPv4: exactly four dotted octets. IPv6: a hex/':' run with >=2 colons
# (so "12:00:00" times and "a:b" pairs are candidates but fail validation).
_IPV4_RE = re.compile(r"(?<![\w.])(?:\d{1,3}\.){3}\d{1,3}(?![\w.])")
# Word-boundary lookarounds use \w (not just hex+':') so a C++ scope fragment
# such as std::cerr / Foo::bar can never START a match (the char before the hex
# run is a letter -> rejected). Without this, "std::cerr" yielded "d::ce".
_IPV6_RE = re.compile(r"(?<![\w:.])[0-9A-Fa-f]{0,4}(?::[0-9A-Fa-f]{0,4}){2,}(?![\w:.])")
# A dotted-quad on a line that talks about a version (e.g. #define ..._VERSION
# "4.0.0.0", Version="1.0.0.0", "1.0.0.0 for CatchChallenger") is a software
# version, not an IP - skip IPv4 matches there. (A 'v' prefix like v1.2.3.4 is
# already rejected by the \w lookbehind.) Not applied to IPv6 (no version shape).
_VERSION_LINE_RE = re.compile(r"(?i)version|\brevision\b")


def _public_ip(token):
    """The ipaddress object for `token` IFF it is a leaked PUBLIC address; None
    otherwise.
    IPv4: any globally-routable address (is_global). Private/loopback/link-local/
    reserved/doc ranges are skipped.
    IPv6: ONLY genuine global-unicast (top 3 address bits == 0b001) AND is_global.
    This is the crux: ipaddress parses "::"-scope fragments left by C++ code
    (single-hex groups around '::') as syntactically valid global addresses, so
    is_global alone flooded the report; real routable IPv6 sits in the global-
    unicast block, which those fragments (and link-local/ULA/loopback/doc) are
    not. The block test is a bit-shift so no IPv6 literal sits here to self-flag."""
    try:
        ip = ipaddress.ip_address(token)
    except ValueError:
        return None
    if ip.version == 6:
        return ip if ((int(ip) >> 125) == 1 and ip.is_global) else None
    return ip if ip.is_global else None


def det_ip_findings(lines):
    """Deterministic public-IP matches: (lineno, 'PUBLIC_IP', ip). Private and
    reserved ranges are skipped (not a leak). The report_allowlist (applied in
    merge()) can still suppress a specific intended public IP."""
    out = []
    n = 0
    for ln in lines:
        n += 1
        version_line = _VERSION_LINE_RE.search(ln) is not None
        for m in _IPV4_RE.finditer(ln):
            if version_line:        # a dotted-quad here is a version, not an IP
                continue
            ip = _public_ip(m.group(0))
            if ip is not None:
                out.append((n, "PUBLIC_IP", str(ip)))
        for m in _IPV6_RE.finditer(ln):
            ip = _public_ip(m.group(0))
            if ip is not None:
                out.append((n, "PUBLIC_IP", str(ip)))
    return out


def parse_llm(answer, lo, hi):
    """Parse LINE findings from the model reply, keeping only line numbers within
    [lo, hi] (the chunk's range)."""
    if not answer:
        return []
    low = answer.lower()
    if "no secrets identified" in low and "line" not in low:
        return []
    out = []
    for raw in answer.splitlines():
        m = LINE_RE.match(raw)
        if not m:
            continue
        n = int(m.group(1))
        if n < lo or n > hi:
            continue
        rest = m.group(2).strip()
        # Chatty small models sometimes emit a line for a CLEAN row, e.g.
        # "LINE 3: OTHER (NO LEAK) -> ...". Drop anything that explicitly says it
        # is not a leak so such noise never reaches the report.
        if _NO_LEAK_RE.search(rest):
            continue
        cat, sep, why = rest.partition("->")
        cat = cat.strip().strip(":").upper() or "OTHER"
        why = why.strip() if sep else rest
        out.append((n, cat, why or rest))
    return out


def _usable_reply(ans):
    """A chunk reply we can trust: it either lists at least one LINE finding or
    explicitly says 'No secrets identified'. An empty / garbage reply (what a
    thinking model returns when it burns its budget reasoning) is NOT usable."""
    low = (ans or "").lower()
    if "no secrets identified" in low:
        return True
    for ln in (ans or "").splitlines():
        if LINE_RE.match(ln):
            return True
    return False


def llm_findings(rel, lines):
    """Ask the LLM to flag secrets in `rel`, chunk by chunk. Standalone: no
    related files are loaded - a secret is local to its line. Only called when
    the 'secret' category is active and run() already confirmed the backend is
    reachable (a per-chunk failure below is logged and skipped).

    In thinking mode the model occasionally spends its whole budget reasoning and
    returns an empty/garbage answer; such a chunk is RETRIED up to THINK_RETRIES
    times (the user opted into the slower thinking path) so a leak is not silently
    missed. think:false replies are always usable, so no retry happens there."""
    system = _secret_system()
    thinking = common._ollama_think() is True
    retries = THINK_RETRIES if thinking else 0
    out = []
    for start, sub in _chunks_of(lines):
        if not any(s.strip() for s in sub):
            continue
        user = ("Scan this file for leaks. Report findings for the lines shown "
                "(use the line number in the left margin).\n\n"
                "=== FILE: %s (lines %d-%d) ===\n%s"
                % (rel, start, start + len(sub) - 1, _numbered(sub, start)))
        msgs = [{"role": "system", "content": system},
                {"role": "user", "content": user}]
        ans = ""
        attempt = 0
        while True:
            attempt += 1
            try:
                ans = chat(msgs)
            except Exception as exc:  # network / backend failure: keep going
                sys.stderr.write("    [llm skip %s @%d: %s]\n" % (rel, start, exc))
                ans = ""
                break
            if attempt > retries or _usable_reply(ans):
                break
            sys.stderr.write("    [llm retry %s @%d: thinking gave no usable answer "
                             "(%d/%d)]\n" % (rel, start, attempt, retries))
        out += parse_llm(ans, start, start + len(sub) - 1)
    return out


def _suppressed(detail):
    """True if a finding's text matches a report_allowlist regex (public domain,
    placeholder, example value)."""
    for rx in REPORT_ALLOW_RES:
        if rx.search(detail):
            return True
    return False


def _weak_variants(v):
    """Forms of a value to test against the weak list: the value itself, the part
    after a single-letter credential flag (mysql -pPASS -> PASS) and the part
    after '='/':' (PGPASSWORD=PASS, --pass=PASS). So a known weak password is
    matched even when the model reports it with its CLI flag."""
    v = v.strip().strip("\"'`").strip(".,;:!()[]{}<>")
    out = {v.lower()}
    if len(v) > 2 and v[0] == "-" and v[1].isalpha():
        out.add(v[2:].lower())
    if "=" in v:
        out.add(v.rsplit("=", 1)[-1].lower())
    if ":" in v:
        out.add(v.rsplit(":", 1)[-1].lower())
    return out


def _is_weak_credential(detail):
    """True if the finding's apparent VALUE is a generic/placeholder credential
    (admin, login, password, catchchallenger, ...). Backstop to the prompt
    instruction. Only the VALUE POSITION is inspected - a quoted string, or the
    trailing token - so a real secret whose DESCRIPTION says 'password' is NOT
    suppressed."""
    if not WEAK_SET:
        return False
    # A QUOTED value (or its flag-stripped form) equal to any weak term.
    for v in re.findall(r"""["'`]([^"'`]{1,60})["'`]""", detail):
        if WEAK_SET & _weak_variants(v):
            return True
    # An UNQUOTED trailing value vs the value-only set (so a description ending in
    # "token"/"key"/"password" is never wrongly dropped).
    toks = detail.split()
    if toks and (WEAK_VALUE_SET & _weak_variants(toks[-1])):
        return True
    return False


def _finding_sort_key(f):
    """Sort findings by (line, category)."""
    return (f[0], f[1])


def merge(findings):
    """Apply the report allow-list, de-duplicate (by line + category + detail
    prefix) and sort by line number."""
    out = []
    seen = set()
    for n, cat, why in findings:
        if _suppressed(why) or _is_weak_credential(why):
            continue
        key = (n, cat.lower(), why.lower()[:48])
        if key in seen:
            continue
        seen.add(key)
        out.append((n, cat, why))
    out.sort(key=_finding_sort_key)
    return out


def scan_one(path):
    """Scan a single file standalone; return merged findings [(line, cat, why)].
    Runs only the layers selected in CATEGORIES."""
    rel = _relpath(path)
    text = read_text(path)
    if text is None:
        return []
    lines = text.split("\n")
    if not any(ln.strip() for ln in lines):
        return []
    findings = []
    if "path" in CATEGORIES:
        findings += det_path_findings(lines)
    if "ip" in CATEGORIES:
        findings += det_ip_findings(lines)
    if "secret" in CATEGORIES:
        findings += llm_findings(rel, lines)
    return merge(findings)


# ===========================================================================
# Driver.
# ===========================================================================
def _fatal_backend(url):
    """Loud abort (BOLD RED on a TTY) when an LLM-requiring scan cannot reach its
    backend. Printed to stderr; the run then exits non-zero WITHOUT scanning."""
    on = "\033[1;31m" if sys.stderr.isatty() else ""
    off = "\033[0m" if sys.stderr.isatty() else ""
    bar = "=" * 72
    sys.stderr.write(
        "%s%s\n"
        "  FATAL: IA/LLM BACKEND UNREACHABLE - ABORTING (no scan performed).\n"
        "\n"
        "  Token / password / API-URL / connection-string detection is LLM-only\n"
        "  and REQUIRES a reachable backend. Refusing to run a partial regex-only\n"
        "  scan that would give false confidence about secrets.\n"
        "\n"
        "    backend tried : %s\n"
        "    fix it        : OLLAMA_HOST=http://127.0.0.1:11434 python secret.py\n"
        "                    (or fix ~/.config/CatchChallenger/ia-settings.json,\n"
        "                     or start the Ollama daemon)\n"
        "    regex only    : python secret.py ip path   (no LLM needed)\n"
        "%s%s\n" % (on, bar, url, bar, off))


def run(mode, explicit, out):
    """Scan, print findings to `out`, return an exit code (0 clean, 2 leaks,
    1 error/abort)."""
    load_config()
    llm_needed = "secret" in CATEGORIES
    if llm_needed:
        # The token/password/API-URL/connection-string classes are LLM-only.
        # TEST the backend FIRST; if it is unreachable, ABORT loudly - never do a
        # silent regex-only partial scan that would give false confidence.
        ok, url, _is_local = common.preflight_backend()
        if not ok:
            _fatal_backend(url)
            return 1
        ensure_context()
    files = collect(mode, explicit)
    if not files:
        sys.stderr.write("%s [INIT] no files to scan (mode=%s)\n" % (_ts(), mode))
        print("No files to scan.", file=out)
        return 0
    start = time.time()
    src = "files: " + ", ".join(_relpath(p) for p in explicit) if explicit \
        else "git mode=%s" % mode
    via = ("LLM=%s + regex" % common.IA_LABEL) if llm_needed else "regex"
    sys.stderr.write("%s [INIT] %d files (%s) | checks: %s | via %s, %d worker(s)\n"
                     % (_ts(), len(files), src, ",".join(sorted(CATEGORIES)),
                        via, WORKERS))

    results = [None] * len(files)
    done = 0
    tty = sys.stderr.isatty()
    with concurrent.futures.ThreadPoolExecutor(max_workers=WORKERS) as ex:
        futs = {}
        i = 0
        while i < len(files):
            futs[ex.submit(scan_one, files[i])] = i
            i += 1
        for fut in concurrent.futures.as_completed(futs):
            idx = futs[fut]
            done += 1
            rel = _relpath(files[idx])
            try:
                results[idx] = fut.result()
            except Exception as exc:  # never let one file kill the sweep
                if tty and not PROGRESS:
                    sys.stderr.write("\r%-60s\r" % "")   # clear counter before the line
                sys.stderr.write("    [skip] %s: %s\n" % (rel, exc))
                results[idx] = []
            # Progress: verbose per-file line only when asked; otherwise a single
            # in-place counter on a TTY, nothing when stderr is redirected.
            if PROGRESS:
                sys.stderr.write("%s [%d/%d] %s (%d)\n"
                                 % (_ts(), done, len(files), rel, len(results[idx])))
                sys.stderr.flush()
            elif tty:
                sys.stderr.write("\r[%d/%d] scanning…" % (done, len(files)))
                sys.stderr.flush()
    if tty and not PROGRESS:
        sys.stderr.write("\r%-60s\r" % "")               # erase the counter line

    flagged = []
    total = 0
    i = 0
    while i < len(files):
        rel = _relpath(files[i])
        res = results[i] or []
        if res:
            flagged.append(rel)
            total += len(res)
            print("=" * 70, file=out)
            print("LEAKS: %s" % rel, file=out)
            print("=" * 70, file=out)
            for n, cat, why in res:
                print("LINE %d: %s -> %s" % (n, cat, why), file=out)
            print(file=out)
            out.flush()
        i += 1

    # The trailing SUMMARY block (count + re-listed file paths) duplicated the
    # per-file LEAKS sections, so it is off by default. Set CC_SECRET_SUMMARY=1 to
    # restore it. A clean scan still prints one line so it is never silent.
    if SHOW_SUMMARY:
        print("=" * 70, file=out)
        print("SUMMARY: %d leak(s) in %d/%d files" % (total, len(flagged), len(files)), file=out)
        print("=" * 70, file=out)
        for rel in flagged:
            print("  - %s" % rel, file=out)
        out.flush()
    if not flagged:
        print("No secrets identified across the scanned files.", file=out)
        out.flush()

    sys.stderr.write("%s [DONE] scanned %d files in %s - %d leak(s) in %d file(s)\n"
                     % (_ts(), len(files), _human_dur(time.time() - start),
                        total, len(flagged)))
    summary = claude_usage_summary()
    if summary:
        sys.stderr.write(summary)
    sys.stderr.flush()
    return 2 if flagged else 0


def _print_help(prog):
    sys.stdout.write(__doc__)
    sys.stdout.write("\nFindings: stdout only (set CC_SECRET_REPORT to also "
                     "write an out-of-repo file)\nConfig (out of repo): %s\n"
                     % CONFIG_FILE)


def main(argv):
    global CATEGORIES
    args = argv[1:]
    if any(a in ("-h", "--help", "help") for a in args):
        _print_help(argv[0])
        return 0
    mode = "all"
    explicit = []
    cats = set()
    for a in args:
        al = a.lower()
        if al in ("all", "staged", "changed"):
            mode = al
        elif al in CATEGORY_ALIASES:
            cats.add(CATEGORY_ALIASES[al])
        elif os.path.exists(a):
            explicit.append(a)
        else:
            sys.stderr.write("%s [warn] ignoring unknown argument / missing "
                             "file: %s\n" % (_ts(), a))
    # No category arg -> run them all (default). Otherwise only the chosen ones.
    if cats:
        CATEGORIES = cats

    # Findings go to STDOUT only. Nothing is written to disk unless the user
    # opts in with CC_SECRET_REPORT (which must point OUT of the repo - the
    # output may quote redacted secrets / internal paths).
    report_fh = None
    out = sys.stdout
    if REPORT_FILE:
        try:
            os.makedirs(os.path.dirname(REPORT_FILE) or ".", exist_ok=True)
            report_fh = open(REPORT_FILE, "w")
            out = _Tee(sys.stdout, report_fh)
        except OSError as exc:
            sys.stderr.write("%s [warn] cannot open report %s (%s); stdout only\n"
                             % (_ts(), REPORT_FILE, exc))
    try:
        rc = run(mode, explicit, out)
    finally:
        if report_fh is not None:
            report_fh.close()
            sys.stderr.write("%s [report] also written to %s\n" % (_ts(), REPORT_FILE))
    return rc


if __name__ == "__main__":
    sys.exit(main(sys.argv))
