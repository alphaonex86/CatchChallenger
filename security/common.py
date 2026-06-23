#!/usr/bin/env python3
"""common.py - shared IA backend transport for the security tools.

Both security/server.py (the agentic code AUDITOR) and security/secret.py (the
secret/leak SCANNER) drive the SAME LLM backend the same way:
  - local Ollama by default (settings-driven, multi-host model routing), OR
  - Anthropic Claude when CC_IA_BACKEND=claude (credential read from the
    environment, never written to disk).
Rather than duplicate ~450 lines of transport in each tool, it lives here and is
imported (`from common import *`) by both. This module owns: backend selection,
the Ollama backend table + per-model routing, num_ctx sizing, Claude streaming +
auth + prompt-caching + usage accounting, and the chat()/chat_with() entry
points. It also carries a few tiny shared helpers (_ts, _Tee, _human_dur).

Three ways to reach a model (default is local Ollama):
  1. LOCAL Ollama          - the default: http://127.0.0.1:11434 with MODEL_NAME
                             (gemma4:26b). Just run a tool; needs `ollama serve`.
  2. REMOTE direct Ollama  - a remote Ollama daemon, OLLAMA_HOST=http://[<ipv6>]:11434
                             python server.py  (BRACKET an IPv6 host; raw port).
  3. REMOTE via PHP router - a PHP HTTP endpoint that ROUTES requests to the
                             optimized instance (recommended for shared use; it
                             is Ollama-API-compatible, so the client is unchanged).
  4. CLAUDE                 - CC_IA_BACKEND=claude (+ a credential below).
Modes 1-3 are all just "an Ollama HTTP backend at some URL", so one knob selects
between them; the code path is identical.

The remote/router URL embeds infrastructure IP and MUST NOT be committed. Put it
in an OUT-OF-REPO config (default ~/.config/CatchChallenger/ia-settings.json) as
  {"ollama_host": "http://[...]:11434"}     (router/deploy coords go here too)
or pass it via the OLLAMA_HOST env var. Never write the URL/IP into the repo.

Env contract (identical for both tools):
  CC_IA_BACKEND     ollama (default) | claude
  CC_IA_MODEL       default Ollama model (default gemma4:26b); a tool's own
                    --model / CC_SECRET_MODEL overrides it
  OLLAMA_HOST       base URL of the default Ollama backend (a local daemon, a
                    remote daemon, or the PHP router). A scheme-less host:port is
                    accepted (http:// assumed). CC_IA_SETTINGS, if present,
                    overrides this with an explicit multi-host table.
  CC_CLAUDE_MODEL   Claude model id (default claude-opus-4-8)
  CC_IA_SETTINGS    JSON declaring multiple Ollama backends (default
                    ./ia-settings.json next to this file)
  ANTHROPIC_API_KEY / CLAUDE_CODE_OAUTH_TOKEN / ANTHROPIC_AUTH_TOKEN  credential

Runtime-tunable config (MODEL_NAME / CLAUDE_MODEL / IA_LABEL) is mutated by a
tool's main() via `common.<NAME> = ...` (NOT a bare rebind in the importing
module, which would not reach this module's functions)."""

import hashlib
import json
import os
import sys
import threading
import time
import urllib.error
import urllib.request

HERE = os.path.dirname(os.path.realpath(__file__))

# ===========================================================================
# Backend configuration
# ===========================================================================
# Default Ollama model. gemma4:26b is the optimized instance's served model;
# override per tool (server.py --model=..., secret.py CC_SECRET_MODEL=...) or
# globally with CC_IA_MODEL.
MODEL_NAME = os.environ.get("CC_IA_MODEL", "gemma4:26b")
# Default Ollama backend URL. Honours the standard OLLAMA_HOST env var so the
# SAME command reaches a local daemon, a remote daemon, or the PHP router just
# by changing OLLAMA_HOST (a scheme-less host:port is accepted). When
# CC_IA_SETTINGS declares an explicit backend table, that wins over this.
_OLLAMA_HOST = (os.environ.get("OLLAMA_HOST") or "http://127.0.0.1:11434").strip()
if "://" not in _OLLAMA_HOST:
    _OLLAMA_HOST = "http://" + _OLLAMA_HOST
OLLAMA_DEFAULT_URL = _OLLAMA_HOST.rstrip("/")
OLLAMA_API = OLLAMA_DEFAULT_URL + "/api/generate"
OLLAMA_CHAT = OLLAMA_DEFAULT_URL + "/api/chat"

# Default backend is the local Ollama model above. For a run that drives the
# SAME loop with Anthropic Claude, set CC_IA_BACKEND=claude and provide a
# credential VIA THE ENVIRONMENT (never store a key/token in the repo):
#   CC_IA_BACKEND=claude ANTHROPIC_API_KEY=sk-ant-api...      python3 server.py all
#   CC_IA_BACKEND=claude CLAUDE_CODE_OAUTH_TOKEN=sk-ant-oat... python3 server.py all
# Only the chat() transport changes; the credential is read at call time and is
# never written to disk. Dependency-free: raw HTTP over the stdlib urllib.
# The backend may ALSO be selected out-of-repo: a settings key 'ia_backend'
# ('ollama' | 'claude') is the fallback when CC_IA_BACKEND env is unset, so a
# shared/host config can default to Claude without per-run env vars. The
# credential may likewise live in that OUT-OF-REPO settings file instead of the
# env: 'claude_oauth_token' (a Claude Code subscription token, sk-ant-oat...,
# preferred) or 'claude_api_key' (a console key, sk-ant-api...). A subscription
# token always wins over a console key, so an empty-balance console key in
# ANTHROPIC_API_KEY can't shadow it. Secrets stay OUT of the repo.

# Out-of-repo settings file path. Defined early (before _resolve_ia_backend)
# because the backend selection reads it at import time. Holds infra URL/IP +
# tokens/keys that must never be committed; override via CC_IA_SETTINGS env.
_XDG_CONFIG = os.environ.get("XDG_CONFIG_HOME") or os.path.expanduser("~/.config")
IA_SETTINGS_FILE = os.environ.get(
    "CC_IA_SETTINGS", os.path.join(_XDG_CONFIG, "CatchChallenger", "ia-settings.json"))


def load_settings():
    """Parsed settings dict, or {} if absent/unreadable/invalid. Best-effort:
    a bad settings file never aborts the run."""
    try:
        with open(IA_SETTINGS_FILE, "r", errors="replace") as fh:
            data = json.load(fh)
    except (OSError, ValueError):
        return {}
    return data if isinstance(data, dict) else {}


def _resolve_ia_backend():
    env = os.environ.get("CC_IA_BACKEND", "").strip().lower()
    if env:
        return env
    cfg = load_settings().get("ia_backend")
    if isinstance(cfg, str) and cfg.strip().lower() in ("ollama", "claude"):
        return cfg.strip().lower()
    return "ollama"


IA_BACKEND = _resolve_ia_backend()
USE_CLAUDE = IA_BACKEND == "claude"
CLAUDE_MODEL = os.environ.get("CC_CLAUDE_MODEL", "claude-opus-4-8")
CLAUDE_API = "https://api.anthropic.com/v1/messages"
CLAUDE_VERSION = "2023-06-01"
# Hard ceiling per streamed reply. Adaptive thinking counts against this, so
# leave headroom: we stream, so a high ceiling costs nothing unless emitted.
CLAUDE_MAX_TOKENS = 16384

# User-facing label so a Claude run doesn't print the (unused) Ollama name.
IA_LABEL = CLAUDE_MODEL if USE_CLAUDE else MODEL_NAME

# Running Claude spend tally (whole process), filled from the stream usage
# events. Guarded because tools may call chat from worker threads.
CLAUDE_USAGE = {"input": 0, "output": 0, "cache_write": 0, "cache_read": 0}
CLAUDE_CALLS = 0
_USAGE_LOCK = threading.Lock()
# USD per million tokens (input, output) by model-family prefix. Cache writes
# bill at 1.25x input, cache reads at 0.10x input.
CLAUDE_PRICES = {
    "claude-fable": (10.0, 50.0),
    "claude-opus": (5.0, 25.0),
    "claude-sonnet": (3.0, 15.0),
    "claude-haiku": (1.0, 5.0),
}


def claude_usage_summary():
    """One-line spend summary for the Claude backend, or '' on the Ollama path
    (local model, no per-token cost to report)."""
    if not USE_CLAUDE or CLAUDE_CALLS == 0:
        return ""
    price_in, price_out = (5.0, 25.0)  # default to Opus-tier if unknown
    for prefix, prices in CLAUDE_PRICES.items():
        if CLAUDE_MODEL.startswith(prefix):
            price_in, price_out = prices
            break
    u = CLAUDE_USAGE
    cost = (u["input"] * price_in
            + u["cache_write"] * price_in * 1.25
            + u["cache_read"] * price_in * 0.10
            + u["output"] * price_out) / 1_000_000.0
    return ("%s [usage] %s: %d calls | in %d (cache w %d / r %d) out %d tok "
            "| ~$%.4f\n"
            % (_ts(), CLAUDE_MODEL, CLAUDE_CALLS, u["input"], u["cache_write"],
               u["cache_read"], u["output"], cost))


# ===========================================================================
# Ollama backends, declared in a JSON settings file kept OUT OF THE REPO by
# default (~/.config/CatchChallenger/ia-settings.json; override CC_IA_SETTINGS).
# It is out-of-repo on purpose: a remote/router URL embeds infrastructure IP that
# must never be committed. Keys (all optional):
#   "ollama_host": "http://[...]:11434"   single default backend (local, a remote
#                                         daemon, or the Ollama-compatible PHP
#                                         router). The OLLAMA_HOST env var wins.
#   "ollama_backends": ["http://127.0.0.1:11434",
#                       {"url": "http://gpu1:11434", "models": ["gemma4:26b"]}]
#                                         multi-host table (overrides ollama_host):
#     a bare string is a backend URL; an object may PIN models to it. Each model
#     routes to the SAME backend every run (a pin wins, else a backend that has
#     it by a stable hash) - so it stays warm and a panel spreads across hosts.
# With NO settings and no OLLAMA_HOST we use the single local backend.
# ===========================================================================
_OLLAMA_BACKENDS = None   # cached [(url, pinned_models_set)]
_MODEL_LOCATIONS = None   # cached {model_name: [backend_url, ...]}
# CLI per-model pins: a model spec may carry '@<host:port>' to route THAT model
# to a specific backend without editing the settings JSON. Filled by
# _register_model_spec(); consulted first by backend_for_model().
_MODEL_URL_PINS = {}      # {model_name: backend_url}


def ollama_backends():
    """Ordered [(url, pinned_models_set)], cached. Falls back to the single local
    backend when settings declare none."""
    global _OLLAMA_BACKENDS
    if _OLLAMA_BACKENDS is None:
        out = []
        declared = load_settings().get("ollama_backends")
        if not isinstance(declared, list):
            declared = []   # a non-list (e.g. a bare string) is not iterable as entries
        for entry in declared:
            if isinstance(entry, str):
                url, pins = entry, []
            elif isinstance(entry, dict):
                url, pins = entry.get("url"), entry.get("models") or []
            else:
                continue
            # Guard malformed entries so a hand-written settings file can never
            # abort the run.
            if isinstance(url, str) and url:
                if not isinstance(pins, (list, tuple)):
                    pins = []
                out.append((url.rstrip("/"),
                            set(p for p in pins if isinstance(p, str))))
        if not out:
            # No multi-host table: single default backend. OLLAMA_HOST (already
            # folded into OLLAMA_DEFAULT_URL) wins; else an out-of-repo config
            # "ollama_host" supplies it (keeps the URL/IP out of the repo).
            url = OLLAMA_DEFAULT_URL
            if not os.environ.get("OLLAMA_HOST", "").strip():
                cfg = load_settings().get("ollama_host")
                if isinstance(cfg, str) and cfg.strip():
                    u = cfg.strip()
                    if "://" not in u:
                        u = "http://" + u
                    url = u.rstrip("/")
            out.append((url, set()))
        _OLLAMA_BACKENDS = out
    return _OLLAMA_BACKENDS


def _model_locations():
    """{model_name: [backend_url, ...]} from each backend's /api/tags, cached.
    Only consulted when more than one backend is configured."""
    global _MODEL_LOCATIONS
    if _MODEL_LOCATIONS is None:
        loc = {}
        for url, _pins in ollama_backends():
            try:
                req = urllib.request.Request(url + "/api/tags", headers=_ollama_headers())
                with urllib.request.urlopen(req, timeout=10) as resp:
                    info = json.loads(resp.read().decode("utf-8", "replace"))
            except (urllib.error.URLError, OSError, ValueError):
                continue
            for m in info.get("models") or []:
                name = m.get("name") or m.get("model")
                if name:
                    loc.setdefault(name, []).append(url)
        _MODEL_LOCATIONS = loc
    return _MODEL_LOCATIONS


def _register_model_spec(spec):
    """Split an optional '@<backend>' suffix off a model spec and record a
    per-model backend pin (the CLI way to say WHICH Ollama host:port serves THIS
    model). Returns the BARE model name. A spec without '@' is returned as-is.
    Claude specs ('claude'/'claude:<id>') ignore any '@...'. The backend accepts
    a full URL or a bare host:port (assumed http)."""
    name, sep, backend = spec.partition("@")
    name = name.strip()
    backend = backend.strip()
    if sep and backend and not (name == "claude" or name.startswith("claude:")):
        if "://" not in backend:
            backend = "http://" + backend
        _MODEL_URL_PINS[name] = backend.rstrip("/")
    return name


def backend_for_model(model):
    """Base URL of the backend serving `model`. A CLI per-model pin wins; else
    one backend -> that one; else a backend that PINS the model; else one that
    actually has it (stable hash of the name); else the hash over all backends."""
    pinned = _MODEL_URL_PINS.get(model)
    if pinned:
        return pinned
    backends = ollama_backends()
    if len(backends) == 1:
        return backends[0][0]
    for url, pins in backends:
        if model in pins:
            return url
    located = _model_locations().get(model)
    candidates = located if located else [u for u, _ in backends]
    h = int(hashlib.sha1(model.encode("utf-8")).hexdigest(), 16)
    return candidates[h % len(candidates)]


def _ollama_token():
    """Bearer token for the Ollama backend, when it sits behind an authenticating
    proxy (e.g. our PHP router's `Authorization: Bearer ...`). Read from the env
    (OLLAMA_API_KEY / OLLAMA_TOKEN) else the OUT-OF-REPO settings ('ollama_token').
    '' when none - the token is a SECRET and is never stored in the repo."""
    tok = (os.environ.get("OLLAMA_API_KEY") or os.environ.get("OLLAMA_TOKEN") or "").strip()
    if not tok:
        cfg = load_settings().get("ollama_token")
        if isinstance(cfg, str):
            tok = cfg.strip()
    return tok


def _ollama_headers():
    """JSON request headers for the Ollama backend, plus the Bearer token when
    one is configured (harmless on a tokenless local daemon, which ignores it)."""
    h = {"Content-Type": "application/json"}
    tok = _ollama_token()
    if tok:
        h["Authorization"] = "Bearer " + tok
    return h


def _ollama_api_kind():
    """Which wire API the backend speaks: 'ollama' (default - native /api/chat
    with a messages array) or 'router' (our PHP front controller: a single POST
    of {model, prompt, preprompt} with a Bearer token, replying {response,...}).
    From env CC_OLLAMA_API or the out-of-repo settings key 'ollama_api'."""
    kind = (os.environ.get("CC_OLLAMA_API") or "").strip().lower()
    if not kind:
        cfg = load_settings().get("ollama_api")
        if isinstance(cfg, str):
            kind = cfg.strip().lower()
    return "router" if kind == "router" else "ollama"


def _ollama_think():
    """Tri-state thinking control: None = omit the `think` field (safe for models
    that don't support it), True/False = send it. From env CC_OLLAMA_THINK or the
    out-of-repo settings 'ollama_think'. A reasoning model (e.g. gemma4:26b) burns
    hundreds of slow thinking tokens before its answer, so set it FALSE there."""
    v = os.environ.get("CC_OLLAMA_THINK")
    if not v:
        v = load_settings().get("ollama_think")
    if isinstance(v, bool):
        return v
    if isinstance(v, str):
        s = v.strip().lower()
        if s in ("1", "true", "yes", "on"):
            return True
        if s in ("0", "false", "no", "off"):
            return False
    return None


def _ollama_num_predict():
    """Max output tokens for the router. Configurable (CC_OLLAMA_NUM_PREDICT /
    settings 'ollama_num_predict'); defaults much higher when thinking is ON so
    the reasoning tokens don't crowd out the answer (a reasoning model can else
    fill num_predict with thought and return an empty answer)."""
    v = os.environ.get("CC_OLLAMA_NUM_PREDICT")
    if not v:
        cfg = load_settings().get("ollama_num_predict")
        if cfg is not None:
            v = str(cfg)
    if v:
        try:
            return int(v)
        except ValueError:
            pass
    return 16384 if _ollama_think() is True else 2048


def _ollama_num_ctx():
    """Context-window size (tokens) for the router backend. Configurable
    (CC_OLLAMA_NUM_CTX / settings 'ollama_num_ctx'); default 131072 so a LARGE
    audit prompt (server.py attaches the file + related files + analyzer
    candidates - tens of thousands of tokens) fits WITHOUT being truncated and
    still leaves room for the answer. gemma4:26b uses compact sliding-window KV,
    so 128K fits the rtx5090 (~21GB). LOWER it for a smaller GPU."""
    v = os.environ.get("CC_OLLAMA_NUM_CTX")
    if not v:
        cfg = load_settings().get("ollama_num_ctx")
        if cfg is not None:
            v = str(cfg)
    if v:
        try:
            return int(v)
        except ValueError:
            pass
    return 131072


# ---------------------------------------------------------------------------
# Truncation flag. Every transport sets this when its reply was cut at the
# output-token cap (router/ollama done_reason=="length", Claude stop_reason==
# "max_tokens"). At our low temperature a truncated reply is DETERMINISTIC, so
# it re-truncates to the SAME bytes and repeats verbatim - the agent loop reads
# this to tell a real stuck model (fatal) from infrastructure truncation
# (recoverable). Reset at the start of each transport call.
# ---------------------------------------------------------------------------
_LAST_REPLY_TRUNCATED = False


def last_reply_truncated():
    """True if the most recent chat reply was cut off at the output-token cap."""
    return _LAST_REPLY_TRUNCATED


def _set_truncated(flag):
    global _LAST_REPLY_TRUNCATED
    _LAST_REPLY_TRUNCATED = bool(flag)


def _router_split(messages):
    """Flatten our [{system},{user},{assistant},...] messages into the router's
    single (preprompt, prompt) pair. One user turn -> prompt is just its text;
    a multi-turn agent loop (server.py) is rendered as labelled text since the
    router is single-shot /api/generate with no message history."""
    sysp = []
    turns = []
    for m in messages:
        content = m.get("content", "")
        if m.get("role") == "system":
            sysp.append(content)
        else:
            turns.append((m.get("role", "user"), content))
    preprompt = "\n\n".join(p for p in sysp if p)
    if len(turns) == 1:
        prompt = turns[0][1]
    else:
        prompt = "\n\n".join("%s:\n%s" % (r.upper(), c) for r, c in turns)
    return preprompt, prompt


def _chat_router(messages, timeout=None, model=None):
    """One turn against the PHP router (custom API). POST {model, prompt,
    preprompt} with the Bearer token; the reply's `response` field is the text.
    The router forces stream=false, so the whole JSON arrives at once. An empty
    body (router `exit` on a bad token / unsupported model / bad payload) returns
    '' so the caller's preflight aborts rather than passing silently."""
    mdl = model or MODEL_NAME
    url = backend_for_model(mdl)
    preprompt, prompt = _router_split(messages)
    # Cap generation (the router runs stream=false and waits for the WHOLE reply,
    # so an uncapped/rambly model blocks for minutes) and keep the model warm
    # across chunks. Mirrors the native /api/chat options.
    payload = {"model": mdl, "prompt": prompt, "preprompt": preprompt,
               "options": {"temperature": 0.1, "num_predict": _ollama_num_predict(),
                           "num_ctx": _ollama_num_ctx(), "repeat_penalty": 1.3},
               "keep_alive": "30m"}
    # Disable slow reasoning tokens on a thinking model (gemma4:26b spends ~1300
    # tokens thinking before a ~30-token answer otherwise). Omitted unless the
    # config/env asks, so non-thinking models are unaffected.
    think = _ollama_think()
    if think is not None:
        payload["think"] = think
    data = json.dumps(payload).encode("utf-8")
    deadline = (time.time() + timeout) if timeout else None
    _set_truncated(False)
    attempt = 0
    while True:
        attempt += 1
        sock_to = max(1, int(deadline - time.time())) if deadline else 1200
        try:
            req = urllib.request.Request(url, data=data, headers=_ollama_headers())
            with urllib.request.urlopen(req, timeout=sock_to) as resp:
                body = resp.read().decode("utf-8", "replace")
            if not body.strip():
                return ""
            try:
                obj = json.loads(body)
            except ValueError:
                return ""
            if obj.get("done_reason") == "length":
                _set_truncated(True)
                sys.stderr.write("    [router] reply hit num_predict cap "
                                 "(raise ollama_num_predict, or set "
                                 "ollama_think:false) - answer may be truncated\n")
            return (obj.get("response") or "").strip()
        except (urllib.error.URLError, ConnectionError, OSError) as exc:
            if deadline and time.time() >= deadline:
                return ""
            if attempt >= 5:
                raise
            sys.stderr.write("    [chat retry %d/5 after %ds %s]\n" % (attempt, sock_to, exc))
            nap = 3 * attempt
            if deadline:
                nap = min(nap, max(0, int(deadline - time.time())))
            time.sleep(nap)


# ===========================================================================
# Tiny shared helpers
# ===========================================================================
def _ts():
    """Wall-clock stamp for progress lines: H:m D/M/Y (e.g. 18:42 23/05/2026)."""
    return time.strftime("%H:%M %d/%m/%Y")


class _Tee:
    """Write to multiple streams at once (used to tee stdout + a report file)."""
    def __init__(self, *streams):
        self._streams = streams

    def write(self, s):
        for st in self._streams:
            st.write(s)

    def flush(self):
        for st in self._streams:
            st.flush()


def _human_dur(seconds):
    """Elapsed seconds as MINmSECs (e.g. 99min09s)."""
    seconds = int(seconds)
    return "%dmin%02ds" % (seconds // 60, seconds % 60)


# ===========================================================================
# num_ctx sizing (Ollama). Floor 16K: smaller contexts truncate a big payload.
# Ceiling 1M, capped to what the model actually honours (ensure_context()).
# ===========================================================================
MIN_CTX = 16384
MAX_CTX = 1024 * 1024
# Model's actual trained context, discovered at runtime by ensure_context();
# fit_ctx caps to it so we never ask Ollama for more than the model can honour.
MODEL_CTX = None


def fit_ctx(*texts):
    """Pick a num_ctx that fits the payload (~3 chars/token + reply headroom),
    clamped to [MIN_CTX, MAX_CTX] and rounded up to a 2048 multiple."""
    chars = 0
    for t in texts:
        chars += len(t)
    tokens = chars // 3 + 2048
    tokens = ((tokens + 2047) // 2048) * 2048
    # Ceiling: MAX_CTX, but never beyond what the model actually supports.
    ceiling = MAX_CTX if MODEL_CTX is None else min(MAX_CTX, MODEL_CTX)
    if tokens > ceiling:
        tokens = ceiling
    if tokens < MIN_CTX:
        tokens = MIN_CTX
    return tokens


def ensure_context():
    """Verify at runtime that the model can honour a >=16K context. Asks Ollama
    (/api/show) for the trained context_length and warns if below MIN_CTX. Stores
    it in MODEL_CTX so fit_ctx caps num_ctx to it. Best-effort, never fatal."""
    global MODEL_CTX
    if USE_CLAUDE:
        # No Ollama to probe; Claude carries a >=200K context and ignores the
        # num_ctx knob entirely, so there is nothing to discover or cap.
        sys.stderr.write("%s [ctx] backend=claude model=%s (Ollama probe "
                         "skipped)\n" % (_ts(), CLAUDE_MODEL))
        return None
    if _ollama_api_kind() == "router":
        # The PHP router only accepts authenticated POST chat - it has no GET
        # /api/show, so probing it just returns empty. num_ctx is managed by the
        # router/Ollama server-side, so there is nothing to discover here.
        sys.stderr.write("%s [ctx] backend=router (model metadata not exposed; "
                         "num_ctx managed server-side)\n" % _ts())
        return None
    try:
        req = urllib.request.Request(
            backend_for_model(MODEL_NAME) + "/api/show",
            data=json.dumps({"model": MODEL_NAME}).encode("utf-8"),
            headers=_ollama_headers(),
        )
        with urllib.request.urlopen(req, timeout=30) as resp:
            info = json.loads(resp.read().decode("utf-8", "replace"))
    except (urllib.error.URLError, OSError, ValueError) as exc:
        sys.stderr.write("%s [ctx] could not query model context length (%s); "
                         "requesting num_ctx in [%d,%d] anyway\n"
                         % (_ts(), exc, MIN_CTX, MAX_CTX))
        return None
    # model_info keys are arch-prefixed, e.g. "qwen2.context_length".
    ctx = None
    for key, val in (info.get("model_info") or {}).items():
        if key.endswith(".context_length"):
            try:
                ctx = int(val)
            except (TypeError, ValueError):
                ctx = None
            break
    if ctx is None:
        sys.stderr.write("%s [ctx] model %s context length unknown; requesting "
                         "num_ctx in [%d,%d]\n"
                         % (_ts(), MODEL_NAME, MIN_CTX, MAX_CTX))
    elif ctx < MIN_CTX:
        sys.stderr.write("%s [ctx] WARNING: model %s max context %d < required "
                         "%d - prompts will be TRUNCATED. Use a model with a "
                         "bigger context.\n" % (_ts(), MODEL_NAME, ctx, MIN_CTX))
    else:
        sys.stderr.write("%s [ctx] model %s context %d >= %d OK (num_ctx capped "
                         "to min(%d, %d))\n"
                         % (_ts(), MODEL_NAME, ctx, MIN_CTX, MAX_CTX, ctx))
    MODEL_CTX = ctx
    return ctx


# ===========================================================================
# Claude transport
# ===========================================================================
def _claude_split(messages):
    """Map our [{system}, {user}, {assistant}, ...] list onto Claude's wire
    shape: a single `system` string plus a user/assistant `messages` array.
    The agent loop starts with one system turn then strictly alternates
    user/assistant, which is exactly what the Messages API wants."""
    system_parts = []
    convo = []
    for m in messages:
        role = m.get("role")
        content = m.get("content", "")
        if role == "system":
            system_parts.append(content)
        else:
            convo.append({"role": role, "content": content})
    return "\n\n".join(system_parts), convo


def _cache_text(text):
    """One ephemeral-cached text content block."""
    return [{"type": "text", "text": text, "cache_control": {"type": "ephemeral"}}]


# Env vars that can carry a credential, in precedence order. ANTHROPIC_API_KEY is
# a console API key (x-api-key); the others carry a Claude Code / `ant` OAuth
# token (Authorization: Bearer + the oauth beta header), e.g. `claude setup-token`.
CLAUDE_CRED_VARS = ("ANTHROPIC_API_KEY", "ANTHROPIC_AUTH_TOKEN",
                    "CLAUDE_CODE_OAUTH_TOKEN")


def _settings_claude_key():
    """A console API key (sk-ant-api...) declared OUT-OF-REPO in the settings
    file (key 'claude_api_key'), or ''. The key is a SECRET - it lives only in
    the out-of-repo config, never in the repo, and is read at call time."""
    cfg = load_settings().get("claude_api_key")
    if isinstance(cfg, str):
        return cfg.strip()
    return ""


def _settings_claude_oauth_token():
    """A Claude Code / `ant` OAuth token (sk-ant-oat...) declared OUT-OF-REPO in
    the settings file (key 'claude_oauth_token', alias 'claude_code_oauth_token'),
    or ''. SUBSCRIPTION-billed, so it is preferred over a console api key (which
    is pay-per-token and 400s on an empty balance). The token is a SECRET: it
    lives only in the out-of-repo config, read at call time, never in the repo."""
    cfg = load_settings()
    for name in ("claude_oauth_token", "claude_code_oauth_token"):
        v = cfg.get(name)
        if isinstance(v, str) and v.strip():
            return v.strip()
    return ""


def _claude_has_creds():
    """True if a credential is available (env var OR out-of-repo settings)."""
    for name in CLAUDE_CRED_VARS:
        if os.environ.get(name, "").strip():
            return True
    return bool(_settings_claude_oauth_token() or _settings_claude_key())


def _claude_auth_headers():
    """Resolve auth headers for the Messages API.

    An OAuth token (Claude Code / `ant`, sk-ant-oat...) is SUBSCRIPTION-billed; a
    console API key (sk-ant-api...) is pay-per-token and 400s on an empty
    balance. So whenever an OAuth token is present - from EITHER the env or the
    out-of-repo config - it WINS, and a stale/empty-balance console key lingering
    in ANTHROPIC_API_KEY can never shadow a configured subscription token.

    Credential sources, OAuth token first (each: env, then settings):
    - env ANTHROPIC_AUTH_TOKEN / CLAUDE_CODE_OAUTH_TOKEN, or an sk-ant-oat value
      pasted into ANTHROPIC_API_KEY by mistake.
    - settings 'claude_oauth_token' (alias 'claude_code_oauth_token'), or an
      sk-ant-oat value stored in 'claude_api_key'.
    - env ANTHROPIC_API_KEY (a console key) -> x-api-key.
    - settings 'claude_api_key' (a console key) -> x-api-key.
    Secrets live only in the env or the OUT-OF-REPO config, are read at call
    time, and are never written to the repo. Bearer carries the mandatory
    `oauth-2025-04-20` beta header (a bearer token without it 401s). Raises if no
    credential is present."""
    env_key = os.environ.get("ANTHROPIC_API_KEY", "").strip()
    # 1) OAuth token (preferred): explicit env vars, an oat token mis-pasted into
    #    the api-key slot, then the out-of-repo config (dedicated key, or an oat
    #    token stored in claude_api_key).
    token = (os.environ.get("ANTHROPIC_AUTH_TOKEN", "").strip()
             or os.environ.get("CLAUDE_CODE_OAUTH_TOKEN", "").strip())
    if not token and env_key.startswith("sk-ant-oat"):
        token = env_key
    if not token:
        token = _settings_claude_oauth_token()
    if not token:
        settings_key = _settings_claude_key()
        if settings_key.startswith("sk-ant-oat"):
            token = settings_key
    if token:
        return {"authorization": "Bearer " + token,
                "anthropic-beta": "oauth-2025-04-20"}
    # 2) Console API key fallback: env first, then the out-of-repo config. An
    #    sk-ant-oat value here was already promoted to a token above.
    key = env_key
    if not key:
        settings_key = _settings_claude_key()
        if not settings_key.startswith("sk-ant-oat"):
            key = settings_key
    if key:
        return {"x-api-key": key}
    raise RuntimeError(
        "CC_IA_BACKEND=claude but no credential found - set CLAUDE_CODE_OAUTH_TOKEN "
        "(env, a Claude Code subscription token) or ANTHROPIC_API_KEY (env, a "
        "console key), or put 'claude_oauth_token' / 'claude_api_key' in the "
        "out-of-repo settings (%s)." % IA_SETTINGS_FILE)


class _ClaudeStreamError(OSError):
    """An `error` event arriving IN-BAND in an already-200 stream (e.g.
    overloaded_error). Subclasses OSError so it lands in chat_claude's existing
    transient-retry handler and backs off like an HTTP 529, instead of being
    swallowed and returned as a (bogus) clean completion."""


def chat_claude(messages, timeout=None, model=None):
    """One streamed Anthropic Messages turn. Same contract as chat(): returns
    the assistant text. Mirrors the Ollama path's wall-clock `timeout` (a hard
    cap used by the exploit phase) and its transient-error retry/backoff.

    Prompt caching: the system prompt is byte-identical on every turn (and
    across files), and the first user turn is identical across one file's
    tool-loop turns. Both are marked as cache breakpoints, so turns 2..N read
    that prefix at ~0.1x input price. We never mutate the shared `messages`
    list - the content-block conversion happens here, on a private copy, so the
    Ollama path (which expects plain-string content) is unaffected."""
    global CLAUDE_CALLS
    mdl = model or CLAUDE_MODEL
    auth_headers = _claude_auth_headers()  # raises if no credential present
    system, convo = _claude_split(messages)
    # Cache the system block and the first user turn (the large, stable prefix).
    cached_convo = []
    first_user_cached = False
    for msg in convo:
        if not first_user_cached and msg["role"] == "user":
            cached_convo.append({"role": "user",
                                 "content": _cache_text(msg["content"])})
            first_user_cached = True
        else:
            cached_convo.append(msg)
    deadline = (time.time() + timeout) if timeout else None
    payload = {
        "model": mdl,
        "max_tokens": CLAUDE_MAX_TOKENS,
        "system": _cache_text(system) if system else "",
        "messages": cached_convo,
        "stream": True,
        # Adaptive thinking: let the model reason as deeply as the task needs
        # before emitting its answer. We only collect the visible text deltas.
        "thinking": {"type": "adaptive"},
        "output_config": {"effort": "high"},
    }
    data = json.dumps(payload).encode("utf-8")
    headers = {"content-type": "application/json",
               "anthropic-version": CLAUDE_VERSION}
    headers.update(auth_headers)
    _set_truncated(False)
    attempt = 0
    while True:
        attempt += 1
        if deadline:
            sock_to = max(1, int(deadline - time.time()))
        else:
            sock_to = 1200
        try:
            req = urllib.request.Request(CLAUDE_API, data=data, headers=headers)
            chunks = []
            stop_reason = None
            cut = False
            # Per-stream token tally; folded into the global totals only on a
            # clean completion (below), so a retried/aborted partial never
            # double-counts.
            u_in = u_cw = u_cr = u_out = 0
            with urllib.request.urlopen(req, timeout=sock_to) as resp:
                for raw in resp:
                    if deadline and time.time() >= deadline:
                        sys.stderr.write("    [chat cut: budget reached mid-stream]\n")
                        cut = True
                        break
                    raw = raw.strip()
                    # SSE: we only care about the JSON `data:` lines.
                    if not raw.startswith(b"data:"):
                        continue
                    body = raw[len(b"data:"):].strip()
                    if not body:
                        continue
                    try:
                        obj = json.loads(body)
                    except ValueError:
                        continue
                    etype = obj.get("type")
                    if etype == "message_start":
                        usage = (obj.get("message") or {}).get("usage") or {}
                        u_in = usage.get("input_tokens", 0) or 0
                        u_cw = usage.get("cache_creation_input_tokens", 0) or 0
                        u_cr = usage.get("cache_read_input_tokens", 0) or 0
                        u_out = usage.get("output_tokens", 0) or 0
                    elif etype == "content_block_delta":
                        delta = obj.get("delta") or {}
                        # Ignore thinking_delta - keep only the answer text.
                        if delta.get("type") == "text_delta":
                            piece = delta.get("text")
                            if piece:
                                chunks.append(piece)
                    elif etype == "message_delta":
                        stop_reason = (obj.get("delta") or {}).get("stop_reason") or stop_reason
                        # output_tokens here is cumulative for this message.
                        u_out = (obj.get("usage") or {}).get("output_tokens", u_out) or u_out
                    elif etype == "error":
                        # Raise BEFORE the usage-commit so a partial stream is
                        # retried (or, after 5 tries, surfaced) rather than
                        # passed off as a finished answer.
                        err = obj.get("error") or {}
                        raise _ClaudeStreamError(err.get("message", err))
            if cut and chunks:
                # The cumulative output_tokens only arrives in message_delta,
                # which a mid-stream cut never reaches - u_out still holds the
                # tiny message_start value. Approximate from the streamed text
                # (~4 chars/token) so the spend summary is a lower bound, not ~0.
                u_out = max(u_out, len("".join(chunks)) // 4)
            # Clean completion: commit this stream's usage to the global totals.
            with _USAGE_LOCK:
                CLAUDE_USAGE["input"] += u_in
                CLAUDE_USAGE["cache_write"] += u_cw
                CLAUDE_USAGE["cache_read"] += u_cr
                CLAUDE_USAGE["output"] += u_out
                CLAUDE_CALLS += 1
            if stop_reason == "refusal":
                # Authorised use, but the safety classifier can still decline.
                sys.stderr.write("    [claude declined this turn (stop_reason=refusal)]\n")
            elif stop_reason == "max_tokens":
                # Don't let a truncated reply pass as a complete answer.
                _set_truncated(True)
                sys.stderr.write("    [claude reply truncated: hit max_tokens=%d "
                                 "(adaptive thinking + answer share the budget); "
                                 "answer may be partial]\n" % CLAUDE_MAX_TOKENS)
            return "".join(chunks).strip()
        except urllib.error.HTTPError as exc:
            # 4xx (bad request / auth / model id) won't fix themselves - read the
            # body so the user sees why, then raise. Retry only on 429/5xx/529.
            detail = ""
            try:
                detail = exc.read().decode("utf-8", "replace")[:500]
            except OSError:
                pass
            if exc.code not in (429, 500, 502, 503, 529):
                raise RuntimeError("Claude API %d: %s" % (exc.code, detail))
            if deadline and time.time() >= deadline:
                return ""
            if attempt >= 5:
                raise
            sys.stderr.write("    [chat retry %d/5 after HTTP %d %s]\n"
                             % (attempt, exc.code, detail))
            nap = 3 * attempt
            if deadline:
                nap = min(nap, max(0, int(deadline - time.time())))
            time.sleep(nap)
        except (urllib.error.URLError, ConnectionError, OSError) as exc:
            if deadline and time.time() >= deadline:
                sys.stderr.write("    [chat give up: budget reached after %s]\n" % exc)
                return ""
            if attempt >= 5:
                raise
            sys.stderr.write("    [chat retry %d/5 after %ds %s]\n" % (attempt, sock_to, exc))
            nap = 3 * attempt
            if deadline:
                nap = min(nap, max(0, int(deadline - time.time())))
            time.sleep(nap)


# ===========================================================================
# Ollama transport
# ===========================================================================
def chat_ollama(messages, timeout=None, model=None):
    """One streamed Ollama /api/chat turn. Returns the assistant message text;
    `model` overrides MODEL_NAME (a panel targets a specific local model).

    Retries transient Ollama outages (ECONNREFUSED) up to 5x with backoff.

    `timeout` (seconds, optional) is a HARD wall-clock cap on the whole call,
    used by the exploit phase to clamp each turn to the remaining budget. When
    set: the socket read timeout is bounded by it, the stream stops early and
    returns whatever has arrived once the cap is hit, and we never retry past it.
    None = unbounded.

    Delegates to the custom PHP router (_chat_router) when CC_OLLAMA_API /
    settings 'ollama_api' selects 'router' - that backend speaks a different wire
    format, so every Ollama entry point funnels through this dispatch."""
    if _ollama_api_kind() == "router":
        return _chat_router(messages, timeout=timeout, model=model)
    _set_truncated(False)
    deadline = (time.time() + timeout) if timeout else None
    mdl = model or MODEL_NAME
    chat_url = backend_for_model(mdl) + "/api/chat"
    body = "".join(m["content"] for m in messages)
    payload = {
        "model": mdl,
        "messages": messages,
        "stream": True,
        "options": {
            "temperature": 0.1,
            "num_ctx": fit_ctx(body),
            # Bound a single reply and discourage the 33B's repetition loops
            # (it sometimes spews the same line thousands of times).
            "num_predict": 2048,
            "repeat_penalty": 1.3,
        },
    }
    data = json.dumps(payload).encode("utf-8")
    attempt = 0
    while True:
        attempt += 1
        # Socket-level timeout: bound a stalled read by the time left (so a hung
        # connection can't block past the budget), else a sane default.
        if deadline:
            sock_to = max(1, int(deadline - time.time()))
        else:
            sock_to = 1200
        try:
            req = urllib.request.Request(chat_url, data=data, headers=_ollama_headers())
            chunks = []
            with urllib.request.urlopen(req, timeout=sock_to) as resp:
                for raw in resp:
                    # Hard wall-clock cap: stop reading and return the partial
                    # reply; the caller's deadline check then ends the work.
                    if deadline and time.time() >= deadline:
                        sys.stderr.write("    [chat cut: budget reached mid-stream]\n")
                        break
                    raw = raw.strip()
                    if raw:
                        try:
                            obj = json.loads(raw)
                        except ValueError:
                            pass
                        else:
                            msg = obj.get("message") or {}
                            piece = msg.get("content")
                            if piece:
                                chunks.append(piece)
                            # Final stream object carries the stop reason; "length"
                            # means the 2048 num_predict cap chopped the reply.
                            if obj.get("done_reason") == "length":
                                _set_truncated(True)
            return "".join(chunks).strip()
        except (urllib.error.URLError, ConnectionError, OSError) as exc:
            # Don't retry/backoff past the wall-clock cap - return empty so the
            # caller's deadline check fires.
            if deadline and time.time() >= deadline:
                sys.stderr.write("    [chat give up: budget reached after %s]\n" % exc)
                return ""
            if attempt >= 5:
                raise
            sys.stderr.write("    [chat retry %d/5 after %ds %s]\n" % (attempt, sock_to, exc))
            # Don't sleep past the deadline either.
            nap = 3 * attempt
            if deadline:
                nap = min(nap, max(0, int(deadline - time.time())))
            time.sleep(nap)


def chat(messages, timeout=None):
    """Single-model turn against the CONFIGURED backend (Claude when
    CC_IA_BACKEND=claude, else Ollama)."""
    if USE_CLAUDE:
        return chat_claude(messages, timeout)
    return chat_ollama(messages, timeout)


def chat_with(spec, messages, timeout=None):
    """Single turn against a SPECIFIC model. spec None -> chat() (the configured
    backend); 'claude'/'claude:<id>' -> the Claude backend with that model id;
    anything else -> Ollama with that model name."""
    if not spec:
        return chat(messages, timeout)
    if spec == "claude" or spec.startswith("claude:"):
        mdl = spec.split(":", 1)[1] if ":" in spec else None
        return chat_claude(messages, timeout, model=mdl)
    return chat_ollama(messages, timeout, model=spec)


# Backend preflight timeout (seconds). The probe is a REAL minimal chat round-trip
# (POST + Bearer + proxy + model) because the router only accepts authenticated
# POSTs - a plain GET /api/tags returns empty there. A positive CC_IA_PROBE_TIMEOUT
# forces a fixed value; otherwise (0/unset) preflight derives it from the think
# setting via _probe_timeout(): a reasoning run needs FAR longer to first-reply.
try:
    PROBE_TIMEOUT = int(os.environ.get("CC_IA_PROBE_TIMEOUT", "0") or "0")
except ValueError:
    PROBE_TIMEOUT = 0
# Wall budgets (seconds) for the two thinking regimes. think:false = only a cold
# load + a short answer; thinking enabled/omitted = allow a long reasoning pass
# before the first token (bounded by the router's own 400s-per-backend curl cap).
PROBE_TIMEOUT_FAST = int(os.environ.get("CC_IA_PROBE_TIMEOUT_FAST", "180"))
PROBE_TIMEOUT_THINK = int(os.environ.get("CC_IA_PROBE_TIMEOUT_THINK", "420"))


def _probe_timeout():
    """Preflight wall budget: a forced CC_IA_PROBE_TIMEOUT if set, else think-
    aware (short when think:false, long when the model may reason)."""
    if PROBE_TIMEOUT > 0:
        return PROBE_TIMEOUT
    return PROBE_TIMEOUT_FAST if _ollama_think() is False else PROBE_TIMEOUT_THINK


def _is_local_url(url):
    """True if `url` points at the local machine (loopback)."""
    return ("//127.0.0.1" in url or "//localhost" in url
            or "//[::1]" in url or "://localhost" in url)


def preflight_backend():
    """Resolve, LOG and (for Ollama) reachability-probe the backend that the run
    will actually use. Returns (ok, url, is_local).

    The URL is logged so it is always obvious which instance the run hits -
    especially a non-local/optimized remote. For Ollama it then probes
    `<url>/api/tags` with a SHORT timeout: an unreachable remote returns ok=False
    immediately, so callers can fail fast (server.py) or fall back to
    deterministic-only (secret.py) instead of hanging for hours. For Claude there
    is nothing to probe - chat_claude owns its retries/credential errors."""
    if USE_CLAUDE:
        sys.stderr.write("%s [backend] Claude API, model %s\n"
                         % (_ts(), CLAUDE_MODEL))
        return (True, CLAUDE_API, False)
    url = backend_for_model(MODEL_NAME)
    is_local = _is_local_url(url)
    sys.stderr.write("%s [backend] Ollama %s: %s (model %s)\n"
                     % (_ts(), "local" if is_local else "REMOTE", url, MODEL_NAME))
    try:
        # Real minimal chat: validates the EXACT path the scan uses. The router
        # requires an authenticated POST and proxies only chat, so a non-empty
        # reply here means method + Bearer token + proxy + model all work. An
        # empty reply (router exit on missing/bad token, or no proxy) must NOT
        # pass as healthy - otherwise the scan silently finds nothing.
        reply = chat_ollama([{"role": "user",
                              "content": "Reply with the single word: pong"}],
                            timeout=_probe_timeout())
        if not reply.strip():
            raise ValueError("empty reply - missing/invalid OLLAMA token, or the "
                             "router is not proxying chat (set ollama_token in the "
                             "out-of-repo settings)")
        return (True, url, is_local)
    except (urllib.error.URLError, OSError, ValueError, RuntimeError) as exc:
        sys.stderr.write(
            "%s [backend] UNREACHABLE: %s (%s)\n"
            "%s           check OLLAMA_HOST / ~/.config/CatchChallenger/"
            "ia-settings.json (URL + ollama_token), that the Ollama daemon/router "
            "is up, and host/port/firewall.\n" % (_ts(), url, exc, " " * len(_ts())))
        return (False, url, is_local)


# Names re-exported via `from common import *`. Underscore-prefixed helpers are
# listed explicitly (import * would otherwise skip them).
__all__ = [
    "MODEL_NAME", "OLLAMA_DEFAULT_URL", "OLLAMA_API", "OLLAMA_CHAT",
    "IA_BACKEND", "USE_CLAUDE", "CLAUDE_MODEL", "CLAUDE_API", "CLAUDE_VERSION",
    "CLAUDE_MAX_TOKENS", "IA_LABEL", "CLAUDE_PRICES", "IA_SETTINGS_FILE",
    "MIN_CTX", "MAX_CTX", "MODEL_CTX", "CLAUDE_CRED_VARS",
    "claude_usage_summary", "load_settings", "ollama_backends",
    "_model_locations", "_register_model_spec", "backend_for_model",
    "_ts", "_Tee", "_human_dur", "fit_ctx", "ensure_context",
    "_claude_split", "_cache_text", "_claude_has_creds", "_claude_auth_headers",
    "_settings_claude_key", "_resolve_ia_backend",
    "_ClaudeStreamError", "chat_claude", "chat_ollama", "chat", "chat_with",
    "PROBE_TIMEOUT", "preflight_backend", "last_reply_truncated",
]
