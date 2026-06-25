# security/ — IA-driven security tooling

Python files, dependency-free (stdlib `urllib` only), KISS:
* `common.py` — shared IA backend transport (Ollama / Claude API / Claude CLI). Backend selection, multi-host Ollama routing + per-model `@url` pins, DYNAMIC `num_ctx` (fit_ctx sizes to the payload + the 2048 reply cap + margin, low floor → a small view runs a small/fast KV cache; grows per-call as the agentic convo grows; `CC_OLLAMA_MIN_CTX`), `think` control, `chat()`/`chat_with()`.
* `server.py` — agentic SECURITY auditor of the TCP-reachable server (whole-file scan + exploit). `codecheck` mode = per-function security review (`agentic.py`) + exploit.
* `codecheck.py` → **MOVED to `tools/codecheck/`** (general code-quality, NOT security). Its per-function engine (build_index / build_views / verdict cache / tidy / types) is REUSED by `server.py codecheck` + `agentic.py` (imported across the boundary via sys.path); see `tools/codecheck/CLAUDE.md`.
* `agentic.py` — multi-IA agentic WORKGROUP engine shared by codecheck (tools/) + server.
* `codetree.py` / `pycodetree.py` — C/C++ (clang LLVM-IR) / Python (ast) call graph: function index + caller/callee trees.
* `secret.py` — leak SCANNER. Standalone per file (NO related-file loading). Deterministic regex (internal paths, public IPs) + LLM (tokens/passwords/API-URLs). Scans git-tracked+untracked (`all`, default), `staged`, `changed`, or explicit files. Exit 2 = leaks found.

## IA backend — default model `gemma4:26b`
`OLLAMA_HOST` (a scheme-less `host:port` is accepted; BRACKET IPv6) selects the access mode by URL alone — the code path is identical:
* **local** (default): `http://127.0.0.1:11434`.
* **remote direct**: a remote Ollama daemon on `:11434`.
* **remote via PHP router** (recommended for shared use): an Ollama-API-compatible PHP endpoint that routes to the optimized instance.
* **Claude (API)**: `CC_IA_BACKEND=claude` + a credential (`ANTHROPIC_API_KEY` console key, or out-of-repo `claude_api_key`). Console-key / pay-per-token path.
* **Claude (CLI)**: `CC_IA_BACKEND=claude-cli` — shells out to the official `claude -p` (must be logged in once). HARD RULE: a Pro/Max SUBSCRIPTION may ONLY be used via this CLI backend; replaying its OAuth token through the API backend violates Anthropic's Consumer Terms (tokens were BLOCKED Jan 2026). No token/key is held — `claude` authenticates itself. All built-in tools disabled; runs from a temp cwd; `claude_cli_preflight()` pings once up front.

**HARD RULE — never commit a remote/router URL or its IP / SSH coords.** They embed infrastructure IP. The real backend config lives OUT of the repo: `~/.config/CatchChallenger/ia-settings.json` (override `CC_IA_SETTINGS`) — `{"ollama_host": "http://[...]:11434"}` (single) or `ollama_backends` (multi-host table); the router's deploy SSH coords go in its `_router_ssh` note there. `OLLAMA_HOST` env overrides the file. In-repo `ia-settings.example.json` is an IP-free template only. Default model overridable via `CC_IA_MODEL`, `--model=` (server), `CC_SECRET_MODEL` (secret).

## `server.py codecheck` — per-function SECURITY review (shared engine)
`server.py codecheck` reuses the function-by-function engine in `tools/codecheck/codecheck.py` (via `agentic.py`) but with the SECURITY prompt + the clang static-analyzer check-set, then `run_exploit()` develops a proof (no proof => no claim). The LLM is MANDATORY (`CC_IA_PANEL`, never auto; each spec `<model>[@ollama-url]` or `claude` = the official CLI); `>= 2` => panel/workgroup. Bound a run: `CC_CODECHECK_SCOPE` / `CC_CODECHECK_LIMIT`. The general (non-security) auditor, the limited-view design, the four force-multipliers (algorithmic clang-tidy / triviality skip / compute-only incremental cache / adversarial verify) and the context-safety (dynamic num_ctx, agentic budget guard, text-loop collapse) all live in **`tools/codecheck/`** — see its CLAUDE.md. (`test/codecheck.py` is the all.sh harness self-test of that engine, NOT the auditor.)

## secret.py config + report — also OUT of the repo
Path allow-list / detection patterns: `~/.config/CatchChallenger/secret-scan.json` (`CC_SECRET_CONFIG`); a starter template is written on first run. Findings print to **stdout only** — nothing is written to disk unless `CC_SECRET_REPORT=<path>` opts into a file (keep it out of the repo; it may quote redacted secrets). The config is out-of-repo so the scanner never writes a leak back into the tree it scans.
