# tools/codecheck/ — general C/C++ code-quality auditor (NOT security)

**Always read the root `../../CLAUDE.md` first.**

`codecheck.py` is a GENERAL code-quality reviewer — bugs / logic / naming / clarity /
perf / duplication / dead code. Security is explicitly OUT of scope (that is
`security/server.py`). It lives here, OUT of `security/`, because it is unrelated to
security; it only REUSES the shared IA core under `security/` (`common.py` transport,
`codetree.py` / `pycodetree.py` call graph, `agentic.py` multi-IA engine), put on
`sys.path` at import. `security/server.py codecheck` and `security/agentic.py` import
THIS file's engine (build_index / build_views / verdict cache / tidy / types) across
the boundary, so all three run the SAME per-function logic — the file is the engine
AND the CLI. (Because `codecheck.py` runs as `__main__` AND is imported by agentic,
its `__main__` block re-enters via `import codecheck` so there is ONE module — else
the cache dir + the recomputed/reused counter would diverge.)

Audit C/C++ (excl vendor; `codetree.is_vendor`) FUNCTION-BY-FUNCTION: per-turn view =
headers (.h/.hpp whole) + the one body + caller tree + ONE callee branch at a time +
clang `-ast-dump` var types — small enough for a 30B. Functions walked leaves-first.
`--panel` runs the multi-IA agentic engine (each IA may READ/GREP/pull the next
BRANCH, bounded by `CC_AGENTIC_ROUNDS` / `CC_AGENTIC_FUNC_SECS`; the conversation is
held within the model's context (`CC_AGENTIC_CTX_BUDGET`, else the model's real ctx)
so the prompt is NEVER truncated, and a runaway LLM text-loop is collapsed).

**The LLM is MANDATORY and NEVER auto-chosen.** `--llm` (repeatable) or `CC_IA_PANEL`
(comma list). Each spec is `<model>[@<ollama-url>]` (Ollama, optional per-model backend
URL) or `claude` (the official `claude` CLI; ToS-safe). `>= 2` LLMs => panel/workgroup
(consensus brief only on agreement, `CC_CONSENSUS_MIN`). Thinking models (gemma4) need
`CC_OLLAMA_THINK=false`.

Four force-multipliers (read-only, no C/C++ change): (1) **algorithmic — FILE-LEVEL
clang-tidy SWEEP** (`--sweep` = deterministic only, no LLM) over EVERY function in
EVERY file — NOT just codetree's ~4-6 indexed functions/file (that index is sparse,
so a per-indexed-function scan misses most of the code; the sweep is the fix). A
HIGH-SIGNAL curated check-set tuned to the project style (`CC_SWEEP_CHECKS`; drops
std::endl / snake_case_ / short-names / no-auto / include-cleaner / narrowing /
swappable; `CC_TIDY_DEEP=1` adds the slow clang static analyzer), emitted ALWAYS as a
coherent where(file:line)/what(check)/how(message) list — the bulk of the
improvements; the LLM adds judgment on top (clang-tidy on PATH, else graceful no-op);
(2) **triviality pre-filter** — skip
destructors/getters/tiny (~half the tree); (3) **incremental, COMPUTE-ONLY** — every
run STILL prints all findings (nothing silently forgotten); the verdict cache, keyed
on the FULL view + model + mode, only skips the LLM RE-RUN on unchanged input
(`CC_NO_CACHE` forces recompute; logs `cache: N recomputed, M reused`); (4)
**adversarial verify** — double-check each LLM finding (`CC_NO_VERIFY` to skip).

`num_ctx` is dynamic (common.fit_ctx): sized to the payload, not a fixed floor.
Persistent SSD cache (clang-IR + types + tidy + verdict + auto-gen compile DB) at
`/mnt/data/perso/tmp/codecheck` (`CC_CODECHECK_CACHE`). Bound a run: `--scope` /
`--limit`.

**NOT `test/codecheck.py`** — that is the all.sh HARNESS self-test of this engine
(view-size invariant + one smoke call), not the auditor. Running it with `--llm` does
nothing (it ignores those args). The auditor is THIS `tools/codecheck/codecheck.py`.
