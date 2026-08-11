#!/usr/bin/env python3
"""Report what every generated town really got, from the [General] cityDebug
overlay written into all.tmx.

The per-city files on disk do NOT tell you this: a big-city filler house is a
doorless facade with no .tmx of its own, so counting files under-reports badly.
The "City" object layer carries the numbers the generator itself measured.

    ./report-cities.py <dest dir or all.tmx>

Prints the distribution of buildings placed against [city] <size>\\minBuilding,
the density reached against <size>\\densityPercent, and lists the towns that
missed their minimum — those are the ones that read as an empty field.
"""

import collections
import os
import re
import sys

# "<name> <size> lvl<n> <type> <style> chunk<x>,<y> hole<w>x<h> dens<d>/<max>% bld<n>/<min>"
LABEL = re.compile(r'^(?P<name>.+?) (?P<size>small|medium|big) lvl(?P<level>\d+) '
                   r'(?P<type>\S+) (?P<style>\S+) chunk(?P<cx>\d+),(?P<cy>\d+) '
                   r'hole(?P<hw>\d+)x(?P<hh>\d+) dens(?P<dens>\d+)/(?P<densmax>\d+)% '
                   r'bld(?P<bld>\d+)/(?P<bldmin>\d+)$')


def find_all_tmx(source):
    if os.path.isfile(source):
        return source
    for root, _dirs, files in os.walk(source):
        if "all.tmx" in files:
            return os.path.join(root, "all.tmx")
    sys.exit("No all.tmx under %s (is [General] doallmap true?)" % source)


def main():
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    path = find_all_tmx(sys.argv[1])
    text = open(path, encoding="utf-8").read()
    start = text.find('name="City"')
    if start < 0:
        sys.exit("No City object layer in %s — set [General] cityDebug=true and regenerate" % path)
    end = text.find("</objectgroup>", start)
    cities = []
    for name in re.findall(r'<object [^>]*name="([^"]*)"', text[start:end]):
        match = LABEL.match(name)
        if match:
            cities.append(match.groupdict())
    if not cities:
        sys.exit("City layer holds no parsable label in %s" % path)

    print("%d towns" % len(cities))
    for size in ("small", "medium", "big"):
        group = [city for city in cities if city["size"] == size]
        if not group:
            continue
        missed = [city for city in group if int(city["bld"]) < int(city["bldmin"])]
        buildings = collections.Counter(int(city["bld"]) for city in group)
        densities = sorted(int(city["dens"]) for city in group)
        print("\n%-6s %3d towns   hole %sx%s   minBuilding %s   densityPercent %s"
              % (size, len(group), group[0]["hw"], group[0]["hh"],
                 group[0]["bldmin"], group[0]["densmax"]))
        print("       buildings: " + ", ".join("%d->%d" % (count, buildings[count])
                                               for count in sorted(buildings)))
        print("       density  : min %d%%  median %d%%  max %d%%"
              % (densities[0], densities[len(densities) // 2], densities[-1]))
        print("       below minBuilding: %d town(s)%s"
              % (len(missed), (" - " + ", ".join(city["name"] for city in missed[:8])) if missed else ""))
    return 0


if __name__ == "__main__":
    sys.exit(main())
