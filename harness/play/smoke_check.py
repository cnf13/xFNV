#!/usr/bin/env python3
"""The verdict of a harness/smoke.sh run: one line per step, from the evidence in its output folder.

Usage: python3 harness/play/smoke_check.py [--long] [--skip STEP,...] OUT SAVES
  OUT      the run's output folder: play.log, exit.txt, the screenshots, the console's output files smoke_con_* (the
           game's `scof`, moved there by smoke.sh), a crash log FalloutNV.crash-*.log if there was one; for --long
           also FalloutPrefs.run1.ini (the first run's prefs, as the game left them) and restart/ (the second run)
  SAVES    the slot's Saves folder (the save file smoke1.fos)
  --long   the long walk's steps too; --skip: steps left out (reported as skipped)
Exit status 0 when every step that ran is reached and there is no crash log, else 1.
"""
import argparse
import glob
import math
import os
import re
import shutil
import subprocess
import sys

parser = argparse.ArgumentParser()
parser.add_argument("--long", action="store_true")
parser.add_argument("--skip", default="")
parser.add_argument("out")
parser.add_argument("saves")
args = parser.parse_args()
out, saves = args.out, args.saves
skipped = {s for s in args.skip.split(",") if s}

STIMPAK, CAPS, AMMO = 0x15169, 0xF, 0x8ED03


def console(name, folder=None):
    """[(command, answer)] of a console output file: each `> command` line with the `X >> value` answer or the
    error text below it."""
    path = os.path.join(folder or out, "smoke_con_" + name)
    if not os.path.exists(path):
        return None
    pairs = []
    with open(path, encoding="latin-1") as f:
        for raw in f:
            line = raw.strip()
            if not line:
                continue
            if line.startswith(">") and not line.startswith(">>"):
                pairs.append([line.lstrip("> ").strip(), ""])
            elif pairs:
                pairs[-1][1] += (" " if pairs[-1][1] else "") + line
    return pairs


def answers(pairs, command):
    """Every number a command answered, in order (None where it answered no number)."""
    found = []
    for c, a in pairs or []:
        if c.lower() == command.lower():
            m = re.search(r">>\s*(-?[\d.]+)", a)
            found.append(float(m.group(1)) if m else None)
    return found


def answer(pairs, command, nth=0):
    """The number a command answered (its nth occurrence; -1 the last), else None."""
    hits = answers(pairs, command)
    if not hits or nth >= len(hits) or nth < -len(hits):
        return None
    return hits[nth]


def inventories(pairs):
    """The `player.showinventory` answers, in order, each as {form id: count} (`50 - 9mm Round (0008ED03)` lines;
    an item missing from a list has count 0; an equipped item has a line of its own, so the lines of a form add)."""
    lists = []
    for c, a in pairs or []:
        if c.lower() == "player.showinventory":
            counts = {}
            for n, f in re.findall(r"(\d+) - [^()]*\(([0-9A-Fa-f]{8})\)", a):
                counts[int(f, 16)] = counts.get(int(f, 16), 0) + int(n)
            lists.append(counts)
    return lists


def inventory(pairs, nth):
    lists = inventories(pairs)
    return lists[nth] if -len(lists) <= nth < len(lists) else {}


def shot(name, folder=None):
    path = os.path.join(folder or out, name + ".png")
    return path if os.path.exists(path) else None


def differs(a, b):
    """The normalised RMS difference of two screenshots (magick compare), None when one is missing."""
    if not (a and b):
        return None
    r = subprocess.run(["magick", "compare", "-metric", "RMSE", a, b, "null:"], capture_output=True, text=True)
    m = re.search(r"\(([\d.e-]+)\)", r.stderr)
    return float(m.group(1)) if m else None


def screen_text(name, folder=None, crop=None):
    """The text tesseract reads in a screenshot (the game's own subtitles and menus), '' when there is none. The
    game's text is amber: only those pixels are kept, black on white, which tesseract reads where the full picture
    gives it nothing. crop (x, y, width, height as fractions of the picture) reads one block of lines: a backdrop
    with amber in it puts the whole picture's words out of order."""
    path = shot(name, folder)
    if not path or not shutil.which("tesseract"):
        return ""
    cut, mode = [], "11"
    if crop:
        size = subprocess.run(["magick", "identify", "-format", "%w %h", path], capture_output=True, text=True)
        w, h = (int(v) for v in size.stdout.split())
        cut = ["-crop", "%dx%d+%d+%d" % (crop[2] * w, crop[3] * h, crop[0] * w, crop[1] * h), "+repage"]
        mode = "6"
    mask = subprocess.run(["magick", path, "-alpha", "off", *cut, "-fx", "(r-b>0.45 && r>0.6)?0:1", "-resize",
                           "300%" if crop else "200%", "png:-"], capture_output=True)
    r = subprocess.run(["tesseract", "-", "-", "--psm", mode], input=mask.stdout, capture_output=True)
    return " ".join(r.stdout.decode(errors="replace").split())


def exit_line(folder):
    path = os.path.join(folder, "exit.txt")
    return open(path).read().strip() if os.path.exists(path) else ""


log = open(os.path.join(out, "play.log"), encoding="utf-8", errors="replace").read() \
    if os.path.exists(os.path.join(out, "play.log")) else ""
crashes = sorted(glob.glob(os.path.join(out, "FalloutNV.crash-*.log")) +
                 glob.glob(os.path.join(out, "restart", "FalloutNV.crash-*.log")))
menu, cell, quit_, load = (console(n) for n in ("menu", "cell", "quit", "load"))
rows = []


def step(name, ok, evidence):
    rows.append((name, ok))
    print(f"{name:10} {'reached' if ok else 'NOT reached'}: {evidence}")


def skip(name, why="--skip"):
    print(f"{name:10} skipped: {why}")


def count_change(pairs, form, before=0, after=-1):
    """(count in the before-th inventory list, count in the after-th), None for a list that is missing."""
    lists = inventories(pairs)
    pick = (lambda i: lists[i].get(form, 0) if -len(lists) <= i < len(lists) else None)
    return pick(before), pick(after)


# --- the short walk -------------------------------------------------------------------------------------------
step("start", "play: slot" in log and shot("menu") is not None,
     "the game's display came up; screenshot menu.png" if shot("menu") else "no display or no menu screenshot")
in_m = answer(menu, "player.getincell GSDocMitchellHouse")
step("menu", in_m == 0, f"the console answers at the main menu, in no cell: GetInCell {in_m} (smoke_con_menu)")
incell = answer(cell, "player.getincell GSDocMitchellHouse")
step("coc", incell == 1, f"GetInCell GSDocMitchellHouse {incell} (smoke_con_cell; coc.png)")
p0 = [answer(cell, f"player.getpos {a}") for a in "xyz"]
p1 = [answer(cell, f"player.getpos {a}", 1) for a in "xyz"]
if None in p0 or None in p1:
    step("walk", False, "positions missing (smoke_con_cell)")
else:
    d = math.hypot(p1[0] - p0[0], p1[1] - p0[1])
    step("walk", d > 30, f"moved {d:.0f} units on the floor, z {p0[2]:.0f} -> {p1[2]:.0f} (walk.png)")
out_ = answer(cell, "player.isweaponout")
step("draw", out_ == 1, f"IsWeaponOut {out_} (aim.png)")
a0 = inventory(cell, 0).get(AMMO)
a1 = inventory(cell, 1).get(AMMO)
step("fire", None not in (a0, a1) and a1 < a0, f"9mm rounds {a0} -> {a1} (fire.png)")
h0, h1 = answer(cell, "getav health"), answer(cell, "getav health", 1)
step("hit", None not in (h0, h1) and h1 < h0, f"the bighorner's health {h0} -> {h1} (aim.png, fire.png)")
d_open = differs(shot("pipboy_before"), shot("pipboy"))
d_close = differs(shot("pipboy"), shot("pipboy_closed"))
# the Pip-Boy over a dark interior changes the screen by about 0.09 (RMSE); a missed key by about 0.01 (the HUD)
step("pipboy", d_open is not None and d_close is not None and d_open > 0.05 and d_close > 0.05,
     f"screen change opening {d_open}, closing {d_close} (RMSE; pipboy.png, pipboy_closed.png)")
fos = os.path.join(saves, "smoke1.fos")
size = os.path.getsize(fos) if os.path.exists(fos) else 0
step("save", size > 0, f"smoke1.fos {size} bytes (save.png)")
in_q = answer(quit_, "player.getincell GSDocMitchellHouse")
step("quit menu", in_q == 0, f"back at the main menu, in no cell: GetInCell {in_q} (smoke_con_quit; quit_menu.png)")
in_l = answer(load, "player.getincell GSDocMitchellHouse")
caps_saved, caps_l = inventory(cell, 2).get(CAPS), inventory(load, 0).get(CAPS)
step("load", in_l == 1 and caps_saved is not None and caps_l == caps_saved,
     f"GetInCell {in_l}, caps {caps_l} as saved ({caps_saved}; one added after the save) (load.png)")


# --- the long walk --------------------------------------------------------------------------------------------
def check_container():
    con = console("container")
    n0, n1 = count_change(con, STIMPAK)
    step("container", None not in (n0, n1) and n1 == n0 + 1,
         f"stimpaks {n0} -> {n1} after Take All on a dresser holding one (smoke_con_container; container.png)")


def check_drop():
    con = console("drop")
    n0, n1 = count_change(con, STIMPAK, 0, 1)
    n2 = count_change(con, STIMPAK, 0, 2)[1]
    step("drop", None not in (n0, n1, n2) and n1 == n0 - 1 and n2 == n0,
         f"stimpaks {n0} -> {n1} dropped -> {n2} picked up again from the floor (smoke_con_drop; drop.png)")


def check_vats():
    con = console("vats")
    ap0, ap1 = answer(con, "player.getav actionpoints"), answer(con, "player.getav actionpoints", -1)
    a_0, a_1 = count_change(con, AMMO)
    step("vats", None not in (ap0, ap1, a_0, a_1) and ap1 < ap0 and a_1 < a_0,
         f"action points {ap0} -> {ap1}, 9mm rounds {a_0} -> {a_1} (smoke_con_vats; vats.png, vats_fired.png)")


def check_grenade():
    con = console("grenade")
    g0, g1 = count_change(con, 0x4330)
    h_0, h_1 = answer(con, "getav health"), answer(con, "getav health", -1)
    step("grenade", None not in (h_0, h_1, g0, g1) and g1 < g0 and h_1 < h_0,
         f"frag grenades {g0} -> {g1}, the bighorner's health {h_0} -> {h_1} (smoke_con_grenade; grenade.png)")


def check_kill():
    con = console("kill")
    dead = answers(con, "getdead")
    n0, n1 = count_change(con, STIMPAK, -2, -1)
    step("kill", len(dead) >= 2 and dead[0] == 0 and dead[-1] == 1 and None not in (n0, n1) and n1 > n0,
         f"the bighorner GetDead {dead[0] if dead else None} -> {dead[-1] if dead else None} after the shots, "
         f"stimpaks {n0} -> {n1} after looting the corpse (smoke_con_kill; kill.png, loot.png)")


def check_levelup():
    con = console("levelup")
    lv = answers(con, "player.getlevel")
    ew = answers(con, "player.getav energyweapons")
    xs, ys = answers(con, "player.getpos x"), answers(con, "player.getpos y")
    moved = math.hypot(xs[-1] - xs[0], ys[-1] - ys[0]) if len(xs) >= 2 and len(ys) >= 2 and None not in xs + ys \
        else None
    step("levelup", len(lv) >= 2 and lv[-1] == lv[0] + 1 and len(ew) >= 2 and ew[-1] > ew[0] and
         moved is not None and moved > 30,
         f"level {lv[0] if lv else None} -> {lv[-1] if lv else None}, Energy Weapons {ew[0] if ew else None} -> "
         f"{ew[-1] if ew else None}; the menu gone: walked {moved if moved is None else round(moved)} units after it "
         "(smoke_con_levelup; levelup.png, levelup_closed.png)")


def prefs_value(path, section, key):
    value, current = None, None
    if os.path.exists(path):
        for line in open(path, encoding="latin-1"):
            line = line.strip()
            if line.startswith("["):
                current = line.strip("[]").lower()
            elif "=" in line and current == section.lower():
                k, v = line.split("=", 1)
                if k.strip().lower() == key.lower():
                    value = v.strip()
    return value


DIFFICULTY = 'getinisetting "iDifficulty:GamePlay"'
LEVELS = ["Very Easy", "Easy", "Normal", "Hard", "Very Hard"]


def check_settings():
    con = console("settings")
    values = answers(con, DIFFICULTY)
    before, after_menu = (values[0], values[-1]) if len(values) >= 2 else (None, None)
    written = prefs_value(os.path.join(out, "FalloutPrefs.run1.ini"), "GamePlay", "iDifficulty")
    rdir = os.path.join(out, "restart")
    held = answer(console("restart", rdir), DIFFICULTY)
    # the Gameplay page's options column reads "Difficulty < Hard >"
    m = re.search(r"Difficulty\W+(?:\S\s+)?(Very Easy|Very Hard|Easy|Normal|Hard)\b",
                  screen_text("restart_settings", rdir, crop=(0.36, 0.32, 0.30, 0.42)))
    shown = m.group(1) if m else None
    restart_exit = exit_line(rdir)
    ok = None not in (before, after_menu, held, written, shown) and after_menu != before and \
        float(written) == after_menu and held == after_menu and 0 <= after_menu < len(LEVELS) and \
        shown == LEVELS[int(after_menu)] and restart_exit.startswith(("wine exit 0", "linux exit 0"))
    step("settings", ok,
         f"difficulty {before} -> {after_menu} in the Settings menu; FalloutPrefs.ini after the quit: iDifficulty="
         f"{written}; after the restart the game holds {held} and its Gameplay page shows {shown} "
         f"(smoke_con_settings, restart/smoke_con_restart; settings_changed.png, restart/restart_settings.png); "
         f"restart: {restart_exit or 'no exit.txt'}")


def check_death():
    con = console("death")
    caps_saved = inventory(con, 0).get(CAPS) if con else None
    caps_dead = inventory(con, 1).get(CAPS) if con else None
    con2 = console("death_load")
    caps_l = inventory(con2, 0).get(CAPS) if con2 else None
    in_c = answer(con2, "player.getincell GSDocMitchellHouse")
    step("death", None not in (caps_saved, caps_dead, caps_l) and caps_dead > caps_saved and caps_l == caps_saved
         and in_c == 1,
         f"caps {caps_saved} saved, {caps_dead} at the death, {caps_l} after the reload; GetInCell {in_c} "
         "(smoke_con_death, smoke_con_death_load; death.png, death_load.png)")


def check_barter():
    con = console("barter")
    lists = inventories(con)
    first, last = (lists[0], lists[-1]) if len(lists) >= 2 else ({}, {})
    c0, c1 = first.get(CAPS), last.get(CAPS)
    bought = {f: (first.get(f, 0), n) for f, n in last.items() if f != CAPS and n > first.get(f, 0)}
    got = ", ".join(f"{f:08X} {a} -> {b}" for f, (a, b) in bought.items()) or "nothing"
    step("barter", None not in (c0, c1) and c1 < c0 and bool(bought),
         f"caps {c0} -> {c1}; bought: {got} (smoke_con_barter; barter_offer.png, barter_done.png)")


def check_dialogue():
    greeting = screen_text("dialogue")
    topics = screen_text("dialogue_topics")
    reply = screen_text("dialogue_reply")
    con = console("dialogue")
    talked = answers(con, "gettalkedtopc")
    ok = bool(greeting) and bool(reply) and reply not in (greeting, topics) and talked and talked[-1] == 1
    step("dialogue", ok, f"GetTalkedToPC {talked[-1] if talked else None}; the greeting: '{greeting[:60]}', the "
         f"topics: '{topics[:60]}', after the topic: '{reply[:60]}' (dialogue.png, dialogue_topics.png, "
         "dialogue_reply.png)")


LONG = [("container", check_container), ("drop", check_drop), ("vats", check_vats), ("grenade", check_grenade),
        ("kill", check_kill), ("levelup", check_levelup), ("settings", check_settings), ("death", check_death),
        ("barter", check_barter), ("dialogue", check_dialogue)]
if args.long:
    for name, check in LONG:
        if name in skipped:
            skip(name)
        else:
            check()

exit_txt = exit_line(out)
step("quit", exit_txt.startswith(("wine exit 0", "linux exit 0")), exit_txt or "no exit.txt: the game did not end")
for c in crashes:
    print(f"crash log  {c}")
ok = all(r[1] for r in rows) and not crashes
print(f"smoke: {'PASS' if ok else 'FAIL'}: {sum(r[1] for r in rows)}/{len(rows)} steps reached, "
      f"{len(crashes)} crash log(s); output {out}")
sys.exit(0 if ok else 1)
