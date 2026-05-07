# CLAUDE.md

Project-wide guidance for Claude Code.

## Test sessions

**Before starting any new test run** — whether a single `testing*.py` or the
full `test/all.sh` global suite — `git add` / `git commit` / `git push` any
pending work first. Tests are slow and noisy; if a run blows up the working
tree or takes hours, uncommitted changes are at risk and unreviewable. Commit
the snapshot you intend to test so the failures (and any subsequent debug
edits) are isolated to commits made *during* the session.
