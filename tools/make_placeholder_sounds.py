#!/usr/bin/env python3
"""PLACEHOLDER interaction sounds (SCRUM-999), generated so the D-pad has audio feedback until real cues are sourced.

Writes assets/sounds/placeholder-*.wav (44.1 kHz, mono, 16-bit). Run from the repo root: python3 tools/make_placeholder_sounds.py
"""
import math
import random
import struct
import wave

RATE = 44100


def tone(freq, seconds, peak, attack=0.004, decay=None, sweep_to=None):
    """A sine with a short attack and an exponential decay; optionally sliding in pitch."""
    n = int(RATE * seconds)
    out = []
    phase = 0.0
    decay = decay or seconds / 4
    for i in range(n):
        t = i / RATE
        f = freq if sweep_to is None else freq + (sweep_to - freq) * (t / seconds)
        phase += 2 * math.pi * f / RATE
        env = min(1.0, t / attack) * math.exp(-t / decay)
        out.append(peak * env * math.sin(phase))
    return out


def noise_click(seconds, peak, seed):
    """A short burst of high-passed noise: the metallic tick of a handle."""
    rng = random.Random(seed)
    n = int(RATE * seconds)
    out, last = [], 0.0
    for i in range(n):
        t = i / RATE
        raw = rng.uniform(-1, 1)
        hp = raw - last  # first difference: tilts the noise towards the treble
        last = raw
        out.append(peak * 0.5 * hp * math.exp(-t / (seconds / 3)))
    return out


def silence(seconds):
    return [0.0] * int(RATE * seconds)


def mix(*parts):
    length = max(len(p) for p in parts)
    return [sum(p[i] for p in parts if i < len(p)) for i in range(length)]


def write(path, samples):
    peak = max(abs(s) for s in samples) or 1.0
    if peak > 1.0:
        samples = [s / peak for s in samples]
    with wave.open(path, "wb") as out:
        out.setnchannels(1)
        out.setsampwidth(2)
        out.setframerate(RATE)
        out.writeframes(b"".join(struct.pack("<h", int(max(-1.0, min(1.0, s)) * 32767)) for s in samples))


def db(level):
    return 10 ** (level / 20)


# Success: two rising notes (C6 then G6), 170 ms; bright and short so it reads as "done".
write("assets/sounds/placeholder-success.wav", tone(1047, 0.07, db(-12), decay=0.03) + tone(1568, 0.10, db(-12), decay=0.04))
# Failure: a low thud that drops in pitch, 160 ms, with a little second harmonic for body; dull rather than harsh.
write("assets/sounds/placeholder-failure.wav", mix(tone(190, 0.16, db(-10), decay=0.05, sweep_to=110), tone(380, 0.16, db(-22), decay=0.03, sweep_to=220)))
# Target acquired: one soft high sine, 80 ms, well under the others so it is barely there.
write("assets/sounds/placeholder-target.wav", tone(1760, 0.08, db(-28), attack=0.008, decay=0.025))
# Locked door: three quick handle clicks over a faint low knock, about 220 ms: a rattle, not a buzz.
rattle = noise_click(0.025, db(-12), 1) + silence(0.045) + noise_click(0.025, db(-14), 2) + silence(0.045) + noise_click(0.03, db(-13), 3)
write("assets/sounds/placeholder-locked.wav", mix(rattle, tone(120, 0.18, db(-24), decay=0.05)))
