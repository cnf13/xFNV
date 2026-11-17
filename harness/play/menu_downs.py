#!/usr/bin/env python3
"""How many Down presses take a game menu's selection from its top row to a row, for harness/smoke.sh: Down skips
a disabled row (the pause menu's Save while saving is not allowed), so the count is that of the enabled rows after
the top one up to the target. A row is enabled when its label is bright amber in the screenshot: only those pixels
are kept, and tesseract reads the labels.

Usage: python3 harness/play/menu_downs.py SCREENSHOT TARGET ROW...
  SCREENSHOT  the menu with its top row selected
  TARGET      the label to reach
  ROW...      every row's label from the top
Prints the count. Where the target's label is not read, every row counts (and a note goes to stderr).
"""
import os
import re
import subprocess
import sys
import time

shot, target, rows = sys.argv[1], sys.argv[2], sys.argv[3:]
# gamescope writes the screenshot after its command returns: wait (up to 10 s) until the file is there and its size
# holds still
size = -1
for _ in range(20):
    time.sleep(0.5)
    now = os.path.getsize(shot) if os.path.exists(shot) else -1
    if now > 0 and now == size:
        break
    size = now
mask = subprocess.run(["magick", shot, "-alpha", "off", "-fx", "(r-b>0.45 && r>0.6)?0:1", "-resize", "200%",
                       "png:-"], capture_output=True)
text = subprocess.run(["tesseract", "-", "-", "--psm", "11"], input=mask.stdout, capture_output=True)
words = set(re.findall(r"[a-z]+", text.stdout.decode(errors="replace").lower()))
between = rows[1:rows.index(target) + 1]
if target.lower() not in words:
    print(f"menu_downs: '{target}' not read in {shot} (read: {sorted(words)}); every row counted", file=sys.stderr)
    print(len(between))
else:
    print(sum(1 for r in between if r.lower() in words))
