#!/usr/bin/env python3
"""Turns the player towards the NPC for harness/smoke.sh: reads the last player and NPC positions from the console's
output file and writes a console batch file that sets the player's angles at a point of the NPC.

Usage: python3 harness/play/smoke_aim.py CONSOLE_FILE BATCH_FILE [HEIGHT]
  CONSOLE_FILE  the `scof` output holding `player.getpos x|y|z` then `getpos x|y|z` (the NPC, picked with prid); the
                last of each count
  BATCH_FILE    written: `player.setangle z A` (0 is north, clockwise) and `player.setangle x P` (positive looks down),
                CRLF lines for the console's `bat`
  HEIGHT        the aim's height over the NPC's feet (default 90: the chest; 0: the floor at its feet)
"""
import math
import re
import sys

text = open(sys.argv[1], encoding="latin-1").read()
height = float(sys.argv[3]) if len(sys.argv) > 3 else 90.0
player, npc = {}, {}
for cmd, axis, value in re.findall(r">\s*((?:player\.)?getpos) ([xyz])\s+GetPos: [XYZ] >> (-?[\d.]+)", text, re.I):
    (player if cmd.lower().startswith("player") else npc)[axis.lower()] = float(value)
if len(player) < 3 or len(npc) < 3:
    sys.exit(f"smoke_aim: positions missing in {sys.argv[1]} (player {player}, npc {npc})")
dx, dy = npc["x"] - player["x"], npc["y"] - player["y"]
# the camera at about 120 units over the player's feet
dz = (npc["z"] + height) - (player["z"] + 120)
yaw = math.degrees(math.atan2(dx, dy)) % 360
pitch = -math.degrees(math.atan2(dz, math.hypot(dx, dy)))
with open(sys.argv[2], "w", newline="") as f:
    f.write(f"player.setangle z {yaw:.1f}\r\nplayer.setangle x {pitch:.1f}\r\n")
print(f"smoke_aim: distance {math.hypot(dx, dy):.0f}, yaw {yaw:.1f}, pitch {pitch:.1f}")
