#!/usr/bin/env python3
"""Input for a headless game run: XTest key presses and mouse moves on the run's X display (harness/play.sh).

Usage: /usr/bin/python3 harness/play/keys.py DISPLAY STEP...   (the system Python: it needs python-xlib)

Steps (each followed by a 0.5 s pause):
  tap:KEYSYM          press and release (0.15 s)
  hold:KEYSYM:SECS    hold a key; the game misses a key held less than about 0.4 s
  code:KEYCODE        press a raw keycode for 0.3 s
  text:TEXT           type TEXT ('+' and ' ' are spaces)
  con:CMD;CMD...      open the console (grave), type and enter each command ('+' = space), close it
  click               left mouse button
  mhold:SECS[:BUTTON] hold a mouse button (1 left = fire, 3 right = aim)
  move:X:Y            absolute pointer position (park the cursor off the menu items)
  turn:DX, look:DY    relative mouse motion in 20 steps (look: positive is down)
  rel:DX:DY           relative motion in 4-pixel steps
  keymap              print the display's keyboard map
"""
import sys
import time

from Xlib import X, XK, display
from Xlib.ext import xtest

d = display.Display(sys.argv[1])
SYMS = {" ": "space", "+": "space", ".": "period", "-": "minus", "_": "underscore", ",": "comma", "/": "slash",
        ":": "colon", "'": "apostrophe", '"': "quotedbl", "=": "equal", "(": "parenleft", ")": "parenright"}
SHIFTED = set('_:"()')


def code(sym):
    c = d.keysym_to_keycode(XK.string_to_keysym(sym))
    print("key", sym, "keycode", c, flush=True)
    return c


def press(c, secs):
    xtest.fake_input(d, X.KeyPress, c)
    d.sync()
    time.sleep(secs)
    xtest.fake_input(d, X.KeyRelease, c)
    d.sync()


def type_text(text, delay):
    for ch in text:
        k = d.keysym_to_keycode(XK.string_to_keysym(SYMS.get(ch, ch)))
        shift = ch in SHIFTED or ch.isupper()
        if shift:
            s = d.keysym_to_keycode(XK.string_to_keysym("Shift_L"))
            xtest.fake_input(d, X.KeyPress, s)
            d.sync()
        press(k, delay)
        if shift:
            xtest.fake_input(d, X.KeyRelease, s)
            d.sync()
        time.sleep(delay)


def motion(dx, dy, n, pause):
    for i in range(n):
        sx = dx * (i + 1) // n - dx * i // n
        sy = dy * (i + 1) // n - dy * i // n
        xtest.fake_input(d, X.MotionNotify, detail=1, x=sx, y=sy)
        d.sync()
        time.sleep(pause)


for step in sys.argv[2:]:
    kind, _, arg = step.partition(":")
    if kind == "tap":
        press(code(arg), 0.15)
    elif kind == "hold":
        sym, secs = arg.rsplit(":", 1)
        press(code(sym), float(secs))
    elif kind == "code":
        print("keycode", arg, flush=True)
        press(int(arg), 0.3)
    elif kind == "text":
        type_text(arg, 0.1)
        print("text", arg, flush=True)
    elif kind == "con":
        c = code("grave")
        press(c, 0.4)
        time.sleep(1.0)
        for cmd in arg.split(";"):
            type_text(cmd, 0.08)
            press(code("Return"), 0.15)
            time.sleep(1.0)
            print("con", cmd, flush=True)
        press(c, 0.4)
    elif kind == "click":
        xtest.fake_input(d, X.ButtonPress, 1)
        d.sync()
        time.sleep(0.1)
        xtest.fake_input(d, X.ButtonRelease, 1)
        d.sync()
    elif kind == "mhold":
        secs, _, btn = arg.partition(":")
        b = int(btn or 1)
        xtest.fake_input(d, X.ButtonPress, b)
        d.sync()
        time.sleep(float(secs))
        xtest.fake_input(d, X.ButtonRelease, b)
        d.sync()
        print("mhold", arg, flush=True)
    elif kind == "move":
        x, y = (int(v) for v in arg.split(":"))
        xtest.fake_input(d, X.MotionNotify, detail=0, x=x, y=y)
        d.sync()
        print("move", x, y, flush=True)
    elif kind == "turn":
        motion(int(arg), 0, 20, 0.05)
        print("turn", arg, flush=True)
    elif kind == "look":
        motion(0, int(arg), 20, 0.05)
        print("look", arg, flush=True)
    elif kind == "rel":
        dx, dy = (int(v) for v in arg.split(":"))
        motion(dx, dy, max(abs(dx), abs(dy)) // 4 or 1, 0.01)
        print("rel", dx, dy, flush=True)
    elif kind == "keymap":
        lo = d.display.info.min_keycode
        for i, syms in enumerate(d.get_keyboard_mapping(lo, 255 - lo + 1)):
            names = [XK.keysym_to_string(s) or hex(s) for s in syms[:2] if s]
            if names:
                print("map", lo + i, names, flush=True)
    else:
        sys.exit(f"keys.py: unknown step {step}")
    time.sleep(0.5)
