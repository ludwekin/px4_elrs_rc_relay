#!/usr/bin/env python3
"""
Bridge PX4 / MAVLink RC_CHANNELS to a Linux virtual gamepad (uinput).

ELRS receiver -> flight controller (UART1) -> PX4 -> USB MAVLink -> this script
-> /dev/input/eventX (standard Linux joystick/gamepad) -> simulator.

Default mapping:
  ch1 roll      -> ABS_X   (-32767..32767)
  ch2 pitch     -> ABS_Y   (-32767..32767)
  ch3 throttle  -> ABS_Z   (0..32767)
  ch4 yaw       -> ABS_RZ  (-32767..32767)
  ch5..ch8      -> 8 buttons (each switch: low + high)

Run:
  python3 rc_to_hid.py                 # needs write access to /dev/uinput
  python3 rc_to_hid.py --invert-pitch  # if the sim sees pitch reversed
"""

import argparse
import os
import signal
import sys
import time
from datetime import datetime

from evdev import UInput, AbsInfo, ecodes as e
from pymavlink import mavutil

AXES = [
    ("roll",     e.ABS_X,  -32767, 32767),
    ("pitch",    e.ABS_Y,  -32767, 32767),
    ("throttle", e.ABS_Z,       0, 32767),
    ("yaw",      e.ABS_RZ, -32767, 32767),
]

# 4 switches x (low, high)
BUTTON_CODES = [
    e.BTN_SOUTH, e.BTN_EAST, e.BTN_NORTH, e.BTN_WEST,
    e.BTN_TL, e.BTN_TR, e.BTN_TL2, e.BTN_TR2,
]

running = True


def _sigint(_signum, _frame):
    global running
    running = False


def clamp(v, lo, hi):
    return max(lo, min(hi, v))


def scale(v, in_lo=1000, in_hi=2000, out_lo=-32767, out_hi=32767):
    v = clamp(v, in_lo, in_hi)
    return int(round(out_lo + (v - in_lo) * (out_hi - out_lo) / (in_hi - in_lo)))


def stick(v, deadband):
    if abs(v - 1500) <= deadband:
        return 0
    return scale(v)


class RcToHid:
    def __init__(self, args):
        self.args = args
        self.ui = None
        self.mav = None
        self.prev_axes = {}
        self.prev_buttons = {}
        self.last_rc = 0.0
        self.last_status = 0.0
        self.samples = 0
        self.last_log = 0.0
        self.log_f = None
        self.log_w = None
        self._make_uinput()
        self._connect()

    def _make_uinput(self):
        events = {
            e.EV_KEY: BUTTON_CODES,
            e.EV_ABS: [
                (code, AbsInfo(value=0, min=mn, max=mx, fuzz=0, flat=0, resolution=0))
                for _name, code, mn, mx in AXES
            ],
        }
        self.ui = UInput(
            events,
            name="PX4 ELRS RC",
            vendor=0x1209,
            product=0x0002,
            version=0x0001,
            bustype=e.BUS_USB,
        )
        print(f"[hid] created {self.ui.device.path}  name={self.ui.device.name!r}")
        print("[hid] axes: X=roll Y=pitch Z=throttle RZ=yaw; buttons: ch5-8 low/high")
        self._write_axes({code: 0 for _n, code, _mn, _mx in AXES}, force=True)
        self._write_buttons({code: 0 for code in BUTTON_CODES}, force=True)

    def _connect(self):
        if self.mav is not None:
            try:
                self.mav.close()
            except Exception:
                pass
            self.mav = None
        try:
            self.mav = mavutil.mavlink_connection(
                self.args.port, baud=self.args.baud, autoreconnect=False
            )
            hb = self.mav.wait_heartbeat(timeout=6)
            interval_us = int(1e6 / max(1, self.args.rate))
            self.mav.mav.command_long_send(
                1, 1, 511, 0, 65, interval_us, 0, 0, 0, 0, 0
            )
            print(f"[mav] connected to {self.args.port} (sys {hb.get_srcSystem()}, "
                  f"RC_CHANNELS requested at {self.args.rate} Hz)")
        except Exception as exc:
            print(f"[mav] connect failed: {exc}")
            self.mav = None

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

    def _sync(self):
        self.ui.syn()

    def failsafe(self):
        axes = {code: 0 for _n, code, _mn, _mx in AXES}
        buttons = {code: 0 for code in BUTTON_CODES}
        if self._write_axes(axes) | self._write_buttons(buttons):
            self._sync()
            print("[rc] signal lost -> centered / buttons released")

    def update(self, msg):
        ch = [getattr(msg, f"chan{i}_raw", 0) for i in range(1, 17)]
        a = self.args
        roll = stick(ch[0], a.deadband)
        pitch = stick(ch[1], a.deadband)
        yaw = stick(ch[3], a.deadband)
        throttle = scale(ch[2], out_lo=0, out_hi=32767)
        if a.invert_roll:
            roll = -roll
        if a.invert_pitch:
            pitch = -pitch
        if a.invert_yaw:
            yaw = -yaw
        axes = {
            e.ABS_X: roll,
            e.ABS_Y: pitch,
            e.ABS_Z: throttle,
            e.ABS_RZ: yaw,
        }
        buttons = {}
        for i in range(4):
            v = ch[4 + i]
            low, high = BUTTON_CODES[i * 2], BUTTON_CODES[i * 2 + 1]
            buttons[low] = 1 if v < 1250 else 0
            buttons[high] = 1 if v > 1750 else 0
        changed = self._write_axes(axes) | self._write_buttons(buttons)
        if changed:
            self._sync()
        self.samples += 1
        now = time.time()
        if now - self.last_status >= 1.0:
            print(f"[rc] {self.samples/ max(1e-9, now-self.args._t0):5.1f} Hz  "
                  f"ch1-4={ch[0]},{ch[1]},{ch[2]},{ch[3]}  "
                  f"ch5-8={ch[4]},{ch[5]},{ch[6]},{ch[7]}  "
                  f"axes=({roll},{pitch},{throttle},{yaw})")
            self.last_status = now
        if self.log_w and now - self.last_log >= 0.05:
            self.log_w.writerow([round(now - self.args._t0, 3)] + ch)
            self.log_f.flush()
            self.last_log = now

    def run(self):
        self.args._t0 = time.time()
        if self.args.log:
            self.log_f = open(self.args.log, "w", newline="")
            import csv
            self.log_w = csv.writer(self.log_f)
            self.log_w.writerow(["t"] + [f"ch{i}" for i in range(1, 17)])
            print(f"[log] writing {self.args.log}")
        try:
            while running:
                if self.mav is None:
                    time.sleep(0.5)
                    self._connect()
                    continue
                try:
                    msg = self.mav.recv_match(type="RC_CHANNELS", blocking=True, timeout=0.2)
                except Exception as exc:
                    print(f"[mav] read error: {exc}")
                    self.mav = None
                    continue
                if msg is None:
                    if self.last_rc and time.time() - self.last_rc > 1.0:
                        self.failsafe()
                    continue
                self.last_rc = time.time()
                self.update(msg)
        finally:
            self.failsafe()
            if self.log_f:
                self.log_f.close()
            try:
                self.ui.close()
            except Exception:
                pass
            if self.mav:
                try:
                    self.mav.close()
                except Exception:
                    pass


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--port", default="/dev/ttyACM0", help="PX4 MAVLink serial device")
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--rate", type=int, default=50, help="RC_CHANNELS request rate in Hz")
    parser.add_argument("--deadband", type=int, default=8, help="stick center deadband in us")
    parser.add_argument("--invert-roll", action="store_true")
    parser.add_argument("--invert-pitch", action="store_true")
    parser.add_argument("--invert-yaw", action="store_true")
    parser.add_argument("--log", default=None, help="optional CSV log path")
    args = parser.parse_args()
    signal.signal(signal.SIGINT, _sigint)
    signal.signal(signal.SIGTERM, _sigint)
    RcToHid(args).run()


if __name__ == "__main__":
    main()
