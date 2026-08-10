"""Load the franchise wording denylist from OUTSIDE the repository.

The words to reject are themselves the trademarks/character names we must not carry,
so the list lives on the machine, never in git:

    ~/.config/CatchChallenger/franchise-denylist.txt

One lowercase word or expression per line, blank lines and lines starting with "#"
ignored, matched as a substring against the lowercased text. Override the path with
CATCHCHALLENGER_FRANCHISE_DENYLIST.

When the file is absent the check is SKIPPED (with a notice): a fresh clone still runs
the generator and its checks, it just cannot enforce this particular rule until the
operator drops their list in place.
"""
import os

DEFAULT_PATH = os.path.join(
    os.environ.get("XDG_CONFIG_HOME", os.path.expanduser("~/.config")),
    "CatchChallenger", "franchise-denylist.txt")


def denylist_path():
    return os.environ.get("CATCHCHALLENGER_FRANCHISE_DENYLIST", DEFAULT_PATH)


def load(quiet=False):
    """[] when the operator has no list on this machine."""
    path = denylist_path()
    if not os.path.isfile(path):
        if not quiet:
            print("NOTE no franchise denylist at " + path +
                  ", the franchise wording check is skipped")
        return []
    words = []
    for line in open(path, encoding="utf-8"):
        line = line.strip().lower()
        if line and not line.startswith("#"):
            words.append(line)
    return words
