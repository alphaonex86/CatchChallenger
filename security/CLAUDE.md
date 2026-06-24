# security/ — IA-driven security tooling

Three Python files, dependency-free (stdlib `urllib` only), KISS:
* `common.py` — shared IA backend transport (Ollama/Claude). Both tools `from common import *`. Owns backend selection, multi-host Ollama routing, `num_ctx` sizing, Claude streaming/auth/caching/usage, `chat()`/`chat_with()`.
* `server.py` — agentic code AUDITOR of the TCP-reachable server (scan + exploit phases). Loads related/counterpart/caller context per file.
* `secret.py` — leak SCANNER. Standalone per file (NO related-file loading). Deterministic regex (internal paths, public IPs) + LLM (tokens/passwords/API-URLs). Scans git-tracked+untracked (`all`, default), `staged`, `changed`, or explicit files. Exit 2 = leaks found.

## IA backend — default model `gemma4:26b`
`OLLAMA_HOST` (a scheme-less `host:port` is accepted; BRACKET IPv6) selects the access mode by URL alone — the code path is identical:
* **local** (default): `http://127.0.0.1:11434`.
* **remote direct**: a remote Ollama daemon on `:11434`.
* **remote via PHP router** (recommended for shared use): an Ollama-API-compatible PHP endpoint that routes to the optimized instance.
* **Claude (API)**: `CC_IA_BACKEND=claude` + a credential (`ANTHROPIC_API_KEY` console key, or out-of-repo `claude_api_key`). Console-key / pay-per-token path.
* **Claude (CLI)**: `CC_IA_BACKEND=claude-cli` — shells out to the official `claude -p` (must be logged in once). HARD RULE: a Pro/Max SUBSCRIPTION may ONLY be used via this CLI backend; replaying its OAuth token through the API backend violates Anthropic's Consumer Terms (tokens were BLOCKED Jan 2026). No token/key is held — `claude` authenticates itself. All built-in tools disabled; runs from a temp cwd; `claude_cli_preflight()` pings once up front.

**HARD RULE — never commit a remote/router URL or its IP / SSH coords.** They embed infrastructure IP. The real backend config lives OUT of the repo: `~/.config/CatchChallenger/ia-settings.json` (override `CC_IA_SETTINGS`) — `{"ollama_host": "http://[...]:11434"}` (single) or `ollama_backends` (multi-host table); the router's deploy SSH coords go in its `_router_ssh` note there. `OLLAMA_HOST` env overrides the file. In-repo `ia-settings.example.json` is an IP-free template only. Default model overridable via `CC_IA_MODEL`, `--model=` (server), `CC_SECRET_MODEL` (secret).

## secret.py config + report — also OUT of the repo
Path allow-list / detection patterns: `~/.config/CatchChallenger/secret-scan.json` (`CC_SECRET_CONFIG`); a starter template is written on first run. Findings print to **stdout only** — nothing is written to disk unless `CC_SECRET_REPORT=<path>` opts into a file (keep it out of the repo; it may quote redacted secrets). The config is out-of-repo so the scanner never writes a leak back into the tree it scans.
