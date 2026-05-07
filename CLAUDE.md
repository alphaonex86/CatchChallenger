# CLAUDE.md

Project-wide guidance for Claude Code.

## Test sessions

**Before starting any new test run** — whether a single `testing*.py` or the
full `test/all.sh` global suite — `git add` / `git commit` / `git push` any
pending work first. Tests are slow and noisy; if a run blows up the working
tree or takes hours, uncommitted changes are at risk and unreviewable. Commit
the snapshot you intend to test so the failures (and any subsequent debug
edits) are isolated to commits made *during* the session.

## C++ style

**No lambdas.** When you encounter a lambda — and especially when fixing or
extending the code that uses one — replace it with a regular class method (or
a free function) and pass the method pointer to whatever callback machinery
needed it (e.g. the pointer-to-member overload of `QTimer::singleShot`,
`QObject::connect`, etc.). Lambdas hide implementation in a translation-unit-
local closure type, generate harder-to-read linker errors when the body
references something undefined (e.g. `main::{lambda()#2}::operator()() const`
undefined-reference messages), and resist normal navigation tools. A named
method is greppable, debuggable, overridable, and reusable.
