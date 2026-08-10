"""Find the map LABEL a generator run wrote under dest/map/main/.

The label is no longer hard coded ("official" is gone): the generator resolves it from
its settings ("maincode" key) and writes dest/map/main/<label>/. The tools that consume
that tree therefore have to discover it instead of assuming a name.

"tileset" is the run-staging pool that lives next to the labels, never a label itself.
"""
import os

#dest/map/main/tileset/ is the staging pool the settings paths use, not a map label
NOT_A_LABEL = {"tileset"}


def find(dest):
    """(label, path) of the single generated label, or (None, message)."""
    root = os.path.join(dest, "map", "main")
    if not os.path.isdir(root):
        return None, "no " + root
    labels = sorted(name for name in os.listdir(root)
                    if name not in NOT_A_LABEL and
                    os.path.isdir(os.path.join(root, name)))
    if not labels:
        return None, "no map label in " + root
    if len(labels) > 1:
        return None, ("several map labels in " + root + ": " + ", ".join(labels) +
                      " - wipe the stale ones, a run writes exactly one")
    return labels[0], os.path.join(root, labels[0])
