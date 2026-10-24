#!/usr/bin/env python3
"""PLACEHOLDER Force-break textures: crack overlays (white cracks on transparent, 256x256), a pale wood grain for the debris
planks (tinted per object in code), and a soft dust blob for the dust cloud.

Writes assets/textures/damage/crack_stage1-3.png, plank.png and dust.png. Run from the repo root: python3 tools/make_damage_textures.py
"""
import math
import random
from PIL import Image, ImageDraw, ImageFilter

SIZE = 256


def crack(draw, rng, start, angle, length, width, branches):
    """A jagged crack walking out from start, splitting off thinner branches."""
    x, y = start
    steps = max(3, int(length / 12))
    for i in range(steps):
        angle += rng.uniform(-0.45, 0.45)
        step = length / steps
        nx, ny = x + math.cos(angle) * step, y + math.sin(angle) * step
        draw.line([(x, y), (nx, ny)], fill=(255, 255, 255, 255), width=max(1, int(width)))
        if branches and rng.random() < 0.3:
            crack(draw, rng, (nx, ny), angle + rng.choice([-1, 1]) * rng.uniform(0.5, 1.1), length * 0.4, width * 0.6, branches - 1)
        x, y = nx, ny
        width = max(1.0, width * 0.92)


def stage(path, seed, count, reach, width, branches):
    rng = random.Random(seed)
    image = Image.new("RGBA", (SIZE, SIZE), (0, 0, 0, 0))
    draw = ImageDraw.Draw(image)
    centre = (SIZE * 0.5 + rng.uniform(-20, 20), SIZE * 0.45 + rng.uniform(-20, 20))
    for i in range(count):
        angle = 2 * math.pi * i / count + rng.uniform(-0.3, 0.3)
        crack(draw, rng, centre, angle, reach * rng.uniform(0.7, 1.0), width, branches)
    # A soft dark edge under the white so the cracks also read on pale surfaces.
    shadow = Image.new("RGBA", image.size, (20, 14, 8, 0))
    shadow.putalpha(image.getchannel("A").filter(ImageFilter.MaxFilter(3)).point(lambda a: a * 0.55))
    Image.alpha_composite(shadow, image).save(path)


# Lines are thick because the texture is stretched over a whole door: thin cracks vanish at play distance.
stage("assets/textures/damage/crack_stage1.png", 1, 4, 85, 5.5, 1)
stage("assets/textures/damage/crack_stage2.png", 1, 6, 115, 6.5, 2)
stage("assets/textures/damage/crack_stage3.png", 1, 10, 145, 7.5, 3)

# Planks: pale, near-white wood grain (long streaks along X with a few knots), so the vertex tint gives each object its colour.
rng = random.Random(7)
plank = Image.new("RGB", (256, 64), (235, 228, 214))
d = ImageDraw.Draw(plank)
for row in range(0, 64, 2):
    shade = rng.randint(185, 240)
    wobble = rng.uniform(0, 6.28)
    for x in range(0, 256, 4):
        y = row + 1.5 * math.sin(x / 37.0 + wobble)
        d.line([(x, y), (x + 4, y)], fill=(shade, shade - 8, shade - 20), width=1)
for _ in range(3):
    cx, cy = rng.randint(20, 236), rng.randint(10, 54)
    d.ellipse([cx - 5, cy - 3, cx + 5, cy + 3], outline=(150, 140, 125), width=2)
plank.filter(ImageFilter.GaussianBlur(0.6)).save("assets/textures/damage/plank.png")

# Dust: a soft white blob, densest in the middle and gone by the edge, for a cloud rather than squares.
dust = Image.new("RGBA", (64, 64), (255, 255, 255, 0))
for y in range(64):
    for x in range(64):
        r = math.hypot(x - 31.5, y - 31.5) / 32.0
        a = max(0.0, 1.0 - r) ** 1.8
        dust.putpixel((x, y), (255, 255, 255, int(200 * a)))
dust.save("assets/textures/damage/dust.png")
