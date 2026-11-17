#!/usr/bin/env python3
"""
PX4 / ELRS RC monitor GUI + optional virtual gamepad (uinput).

Reads RC_CHANNELS from PX4 over USB MAVLink and shows:
  - left stick: throttle / yaw
  - right stick: pitch / roll
  - switches ch5..ch8 (LOW / MID / HIGH)
  - all 16 raw channel values with bars

It also creates the same virtual gamepad as rc_to_hid.py ("PX4 ELRS RC")
so you can keep using the simulator at the same time.

Run:
  python3 rc_monitor_gui.py
  python3 rc_monitor_gui.py --no-hid
  python3 rc_monitor_gui.py --invert-pitch --invert-yaw

Stop the old rc_to_hid.py bridge first (it owns /dev/ttyACM0):
  pkill -f rc_to_hid.py
"""

import argparse
import threading
import time

import tkinter as tk
from tkinter import ttk

from pymavlink import mavutil

try:
    from evdev import UInput, AbsInfo, ecodes as e
    HAVE_EVDEV = True
except Exception:
    HAVE_EVDEV = False


# ---------------------------------------------------------------- HID mapping
AXES = [
    ("roll",     e.ABS_X,  -32767, 32767) if HAVE_EVDEV else ("roll", 0, -32767, 32767),
    ("pitch",    e.ABS_Y,  -32767, 32767) if HAVE_EVDEV else ("pitch", 0, -32767, 32767),
    ("throttle", e.ABS_Z,       0, 32767) if HAVE_EVDEV else ("throttle", 0, 0, 32767),
    ("yaw",      e.ABS_RZ, -32767, 32767) if HAVE_EVDEV else ("yaw", 0, -32767, 32767),
]
BUTTON_CODES = ([e.BTN_SOUTH, e.BTN_EAST, e.BTN_NORTH, e.BTN_WEST,
                 e.BTN_TL, e.BTN_TR, e.BTN_TL2, e.BTN_TR2] if HAVE_EVDEV else [])

# High-rate telemetry that competes with RC_CHANNELS on the USB MAVLink link.
TRIM_MSG_IDS = [30, 31, 32, 105, 141, 74, 111, 83, 85, 42, 1, 245, 29, 241, 230, 147]


def clamp(v, lo, hi):
    return max(lo, min(hi, v))


def scale(v, in_lo=1000, in_hi=2000, out_lo=-32767, out_hi=32767):
    v = clamp(v, in_lo, in_hi)
    return int(round(out_lo + (v - in_lo) * (out_hi - out_lo) / (in_hi - in_lo)))


def stick(v, deadband):
    if abs(v - 1500) <= deadband:
        return 0
    return scale(v)


class VirtualGamepad:
    def __init__(self, invert_roll=False, invert_pitch=False, invert_yaw=False, deadband=8):
        self.invert_roll = invert_roll
        self.invert_pitch = invert_pitch
        self.invert_yaw = invert_yaw
        self.deadband = deadband
        self.enabled = True
        self.prev_axes = {}
        self.prev_buttons = {}
        self.ui = None
        if not HAVE_EVDEV:
            return
        events = {
            e.EV_KEY: BUTTON_CODES,
            e.EV_ABS: [
                (code, AbsInfo(value=0, min=mn, max=mx, fuzz=0, flat=0, resolution=0))
                for _name, code, mn, mx in AXES
            ],
        }
        self.ui = UInput(events, name="PX4 ELRS RC", vendor=0x1209,
                         product=0x0002, version=0x0001, bustype=e.BUS_USB)
        print(f"[hid] {self.ui.device.path} {self.ui.device.name!r}")
        self._write_axes({code: 0 for _n, code, _a, _b in AXES}, force=True)
        self._write_buttons({c: 0 for c in BUTTON_CODES}, force=True)
        self.ui.syn()

    def _write_axes(self, values, force=False):
        changed = False
        for code, val in values.items():
            if force or self.prev_axes.get(code) != val:
                self.ui.write(e.EV_ABS, code, int(val))
                self.prev_axes[code] = int(val)
                changed = True
        return changed

    def _write_buttons(self, values, force=False):
        changed = False
        for code, val in values.items():
            if force or self.prev_buttons.get(code) != val:
                self.ui.write(e.EV_KEY, code, int(val))
                self.prev_buttons[code] = int(val)
                changed = True
        return changed

    def update(self, ch):
        if not self.ui or not self.enabled:
            return
        roll = stick(ch[0], self.deadband)
        pitch = stick(ch[1], self.deadband)
        yaw = stick(ch[3], self.deadband)
        throttle = scale(ch[2], out_lo=0, out_hi=32767)
        if self.invert_roll:
            roll = -roll
        if self.invert_pitch:
            pitch = -pitch
        if self.invert_yaw:
            yaw = -yaw
        axes = {e.ABS_X: roll, e.ABS_Y: pitch, e.ABS_Z: throttle, e.ABS_RZ: yaw}
        buttons = {}
        for i in range(4):
            v = ch[4 + i]
            buttons[BUTTON_CODES[i * 2]] = 1 if v < 1250 else 0
            buttons[BUTTON_CODES[i * 2 + 1]] = 1 if v > 1750 else 0
        if self._write_axes(axes) | self._write_buttons(buttons):
            self.ui.syn()

    def failsafe(self):
        if not self.ui:
            return
        if self._write_axes({code: 0 for _n, code, _a, _b in AXES}) | \
           self._write_buttons({c: 0 for c in BUTTON_CODES}):
            self.ui.syn()

    def close(self):
        if self.ui:
            try:
                self.ui.close()
            except Exception:
                pass


# ---------------------------------------------------------------- MAVLink
class RcState:
    def __init__(self):
        self.lock = threading.Lock()
        self.ch = [0] * 16
        self.ts = 0.0
        self.count = 0
        self.connected = False

    def update(self, ch):
        with self.lock:
            self.ch = ch
            self.ts = time.time()
            self.count += 1
            self.connected = True

    def snapshot(self):
        with self.lock:
            return list(self.ch), self.ts, self.count, self.connected


class Reader(threading.Thread):
    def __init__(self, state, gamepad, port, baud, rate, trim=True):
        super().__init__(daemon=True)
        self.state = state
        self.gamepad = gamepad
        self.port = port
        self.baud = baud
        self.rate = rate
        self.trim = trim
        self.running = True
        self.mav = None

    def connect(self):
        try:
            self.mav = mavutil.mavlink_connection(self.port, baud=self.baud, autoreconnect=False)
            self.mav.wait_heartbeat(timeout=6)
            interval_us = int(1e6 / max(1, self.rate))
            self.mav.mav.command_long_send(1, 1, 511, 0, 65, interval_us, 0, 0, 0, 0, 0)
            if self.trim:
                for msg_id in TRIM_MSG_IDS:
                    self.mav.mav.command_long_send(1, 1, 511, 0, msg_id, -1, 0, 0, 0, 0, 0)
            print(f"[mav] connected (trim={self.trim})")
        except Exception as exc:
            print(f"[mav] connect failed: {exc}")
            self.mav = None
            time.sleep(1.0)

    def run(self):
        while self.running:
            if self.mav is None:
                self.connect()
                continue
            try:
                msg = self.mav.recv_match(type="RC_CHANNELS", blocking=True, timeout=0.2)
            except Exception as exc:
                print(f"[mav] read error: {exc}")
                self.mav = None
                continue
            if msg is None:
                if self.state.connected and time.time() - self.state.ts > 1.0:
                    self.state.connected = False
                    self.gamepad.failsafe()
                continue
            ch = [getattr(msg, f"chan{i}_raw", 0) for i in range(1, 17)]
            self.state.update(ch)
            self.gamepad.update(ch)


# ---------------------------------------------------------------- GUI widgets
class StickCanvas(tk.Canvas):
    def __init__(self, parent, title, size=230):
        super().__init__(parent, width=size, height=size, bg="#101010",
                         highlightthickness=2, highlightbackground="#3a3a3a")
        self.size = size
        self.title = title
        self._draw()
        self.dot = self.create_oval(0, 0, 0, 0, fill="#39ff14", outline="")

    def _draw(self):
        c = self.size // 2
        r = self.size // 2 - 20
        self.create_oval(c - r, c - r, c + r, c + r, outline="#4a4a4a", width=2)
        self.create_line(c - r, c, c + r, c, fill="#2a2a2a", dash=(4, 4))
        self.create_line(c, c - r, c, c + r, fill="#2a2a2a", dash=(4, 4))
        self.create_text(10, 10, text=self.title, anchor="nw", fill="#7fc4ff",
                         font=("Sans", 11, "bold"))
        self.create_text(c, self.size - 8, text="1000", anchor="s", fill="#666")
        self.create_text(c, 22, text="2000", anchor="n", fill="#666")
        self.create_text(8, c, text="1000", anchor="w", fill="#666")
        self.create_text(self.size - 8, c, text="2000", anchor="e", fill="#666")
        self.create_text(c, c, text="+", fill="#333", font=("Sans", 14))

    def set_pos(self, x, y):
        x = clamp(x, -1.0, 1.0)
        y = clamp(y, -1.0, 1.0)
        c = self.size // 2
        r = self.size // 2 - 20
        px = c + x * r
        py = c - y * r
        self.coords(self.dot, px - 7, py - 7, px + 7, py + 7)


class SwitchIndicator(tk.Frame):
    def __init__(self, parent, title):
        super().__init__(parent, bg="#1b1b1b", padx=6, pady=4)
        tk.Label(self, text=title, bg="#1b1b1b", fg="#bbb",
                 font=("Sans", 9)).pack(pady=(0, 2))
        self.canvas = tk.Canvas(self, width=78, height=26, bg="#1b1b1b",
                                highlightthickness=0)
        self.canvas.pack()
        self.segs = []
        for i, (x0, label) in enumerate([(2, "L"), (28, "M"), (54, "H")]):
            rect = self.canvas.create_rectangle(x0, 2, x0 + 20, 24,
                                                fill="#2c2c2c", outline="#4a4a4a")
            self.canvas.create_text(x0 + 10, 13, text=label, fill="#888",
                                    font=("Sans", 9, "bold"))
            self.segs.append(rect)
        self.value = tk.Label(self, text="---", bg="#1b1b1b", fg="#ccc",
                              font=("Mono", 9))
        self.value.pack()

    def set_state(self, raw):
        if raw < 1250:
            state = 0
        elif raw > 1750:
            state = 2
        else:
            state = 1
        colors = ["#2c2c2c"] * 3
        colors[state] = "#39ff14"
        for rect, color in zip(self.segs, colors):
            self.canvas.itemconfigure(rect, fill=color)
        self.value.config(text=str(raw))


class ChannelBar(tk.Frame):
    def __init__(self, parent, index):
        super().__init__(parent, bg="#1b1b1b")
        self.index = index
        tk.Label(self, text=f"CH{index:02d}", bg="#1b1b1b", fg="#9ad",
                 font=("Mono", 9), width=4, anchor="w").pack(side="left")
        self.canvas = tk.Canvas(self, width=150, height=12, bg="#0d0d0d",
                                highlightthickness=1, highlightbackground="#333")
        self.canvas.pack(side="left", padx=4)
        self.bar = self.canvas.create_rectangle(0, 0, 0, 12, fill="#39ff14",
                                                outline="")
        self.value = tk.Label(self, text="0", bg="#1b1b1b", fg="#ddd",
                              font=("Mono", 9), width=5, anchor="e")
        self.value.pack(side="left")

    def set_value(self, v):
        v = clamp(int(v), 0, 2048)
        width = int(150 * clamp((v - 1000) / 1000.0, 0.0, 1.0))
        self.canvas.coords(self.bar, 0, 0, width, 12)
        color = "#39ff14" if 1250 <= v <= 1750 else "#ffb020"
        if v < 900:
            color = "#333"
        self.canvas.itemconfigure(self.bar, fill=color)
        self.value.config(text=str(v))


class App(tk.Tk):
    def __init__(self, state, reader, gamepad):
        super().__init__()
        self.state = state
        self.reader = reader
        self.gamepad = gamepad
        self.title("PX4 ELRS RC Monitor")
        self.configure(bg="#1b1b1b")
        self.resizable(False, False)
        self._last_count = 0
        self._last_rate_t = time.time()
        self._rate = 0.0
        self._build()
        self.protocol("WM_DELETE_WINDOW", self._on_close)
        self.after(30, self._tick)

    def _build(self):
        header = tk.Frame(self, bg="#1b1b1b")
        header.pack(fill="x", padx=10, pady=(8, 2))
        tk.Label(header, text="PX4 ELRS RC", bg="#1b1b1b", fg="#7fc4ff",
                 font=("Sans", 14, "bold")).pack(side="left")
        self.status = tk.Label(header, text="○ WAITING", bg="#1b1b1b",
                               fg="#ff5555", font=("Sans", 10, "bold"))
        self.status.pack(side="right")
        self.rate_label = tk.Label(header, text="0 Hz", bg="#1b1b1b",
                                   fg="#888", font=("Mono", 9))
        self.rate_label.pack(side="right", padx=10)

        sticks = tk.Frame(self, bg="#1b1b1b")
        sticks.pack(padx=10, pady=4)
        self.left = StickCanvas(sticks, "THROTTLE / YAW")
        self.left.pack(side="left", padx=6)
        self.right = StickCanvas(sticks, "PITCH / ROLL")
        self.right.pack(side="left", padx=6)

        sw_frame = tk.Frame(self, bg="#1b1b1b")
        sw_frame.pack(padx=10, pady=4)
        tk.Label(sw_frame, text="SWITCHES", bg="#1b1b1b", fg="#888",
                 font=("Sans", 9)).pack(side="left", padx=(0, 6))
        self.switches = []
        for i in range(4):
            w = SwitchIndicator(sw_frame, f"CH{5+i}")
            w.pack(side="left", padx=4)
            self.switches.append(w)

        ch_frame = tk.Frame(self, bg="#1b1b1b")
        ch_frame.pack(padx=10, pady=(6, 4))
        left_col = tk.Frame(ch_frame, bg="#1b1b1b")
        right_col = tk.Frame(ch_frame, bg="#1b1b1b")
        left_col.pack(side="left", padx=(0, 10))
        right_col.pack(side="left")
        self.bars = []
        for i in range(16):
            bar = ChannelBar(left_col if i < 8 else right_col, i + 1)
            bar.pack(anchor="w", pady=1)
            self.bars.append(bar)

        footer = tk.Frame(self, bg="#1b1b1b")
        footer.pack(fill="x", padx=10, pady=(2, 8))
        hid_txt = "HID: PX4 ELRS RC (on)" if (HAVE_EVDEV and self.gamepad.ui) else "HID: off"
        tk.Label(footer, text=hid_txt, bg="#1b1b1b", fg="#666",
                 font=("Mono", 8)).pack(side="left")
        tk.Label(footer, text="QGC must be closed while this runs", bg="#1b1b1b",
                 fg="#666", font=("Mono", 8)).pack(side="right")

    def _tick(self):
        ch, ts, count, connected = self.state.snapshot()
        now = time.time()
        age = now - ts if ts else 999

        if connected and age < 0.5:
            self.status.config(text="● CONNECTED", fg="#39ff14")
        else:
            self.status.config(text="○ WAITING", fg="#ff5555")

        if now - self._last_rate_t >= 0.5:
            dt = now - self._last_rate_t
            self._rate = (count - self._last_count) / dt
            self._last_count = count
            self._last_rate_t = now
        self.rate_label.config(text=f"{self._rate:4.1f} Hz")

        yaw = (ch[3] - 1500) / 500.0
        throttle = (ch[2] - 1000) / 500.0 - 1.0
        roll = (ch[0] - 1500) / 500.0
        pitch = (ch[1] - 1500) / 500.0
        self.left.set_pos(yaw, throttle)
        self.right.set_pos(roll, pitch)

        for i in range(4):
            self.switches[i].set_state(ch[4 + i])
        for i in range(16):
            self.bars[i].set_value(ch[i])

        self.after(33, self._tick)

    def _on_close(self):
        self.reader.running = False
        try:
            self.reader.join(timeout=1.0)
        except Exception:
            pass
        self.gamepad.close()
        self.destroy()


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--port", default="/dev/ttyACM0")
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--rate", type=int, default=50)
    parser.add_argument("--deadband", type=int, default=8)
    parser.add_argument("--no-hid", action="store_true")
    parser.add_argument("--no-trim", dest="trim", action="store_false",
                        help="do not disable other high-rate MAVLink streams")
    parser.add_argument("--invert-roll", action="store_true")
    parser.add_argument("--invert-pitch", action="store_true")
    parser.add_argument("--invert-yaw", action="store_true")
    args = parser.parse_args()

    state = RcState()
    gamepad = VirtualGamepad(args.invert_roll, args.invert_pitch, args.invert_yaw,
                             args.deadband)
    if args.no_hid:
        gamepad.enabled = False
    reader = Reader(state, gamepad, args.port, args.baud, args.rate, args.trim)
    reader.start()

    App(state, reader, gamepad).mainloop()


if __name__ == "__main__":
    main()
