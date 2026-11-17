#!/usr/bin/env python3
"""The angles harness/smoke.sh looks at to pick a dropped item up from the floor: the item rolls a little, so the
player looks at a few points around where it fell, pressing E at each. Reads the player's heading from the console's
output file and writes one console batch file per point.

Usage: python3 harness/play/smoke_sweep.py CONSOLE_FILE PREFIX
  CONSOLE_FILE  the `scof` output holding `player.getangle z` (the last one counts)
  PREFIX        written: PREFIX1.txt ... PREFIX8.txt, each `player.setangle z A` and `player.setangle x P` (positive
                looks down), CRLF lines for the console's `bat`
"""
import re
import sys

text = open(sys.argv[1], encoding="latin-1").read()
found = re.findall(r">\s*player\.getangle z\s+GetAngle: Z >> (-?[\d.]+)", text, re.I)
if not found:
    sys.exit(f"smoke_sweep: no player.getangle z in {sys.argv[1]}")
heading = float(found[-1])
# the point straight ahead first, then left and right of it, then a little farther out
points = [(0, 75), (8, 75), (-8, 75), (16, 75), (-16, 75), (0, 62), (10, 62), (-10, 62)]
for i, (turn, pitch) in enumerate(points, 1):
    with open(f"{sys.argv[2]}{i}.txt", "w", newline="") as f:
        f.write(f"player.setangle z {(heading + turn) % 360:.1f}\r\nplayer.setangle x {pitch}\r\n")
print(f"smoke_sweep: heading {heading:.1f}, {len(points)} points")
