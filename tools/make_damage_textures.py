#!/usr/bin/env python3
"""PLACEHOLDER Force-break textures: crack overlays (white cracks on transparent, 256x256) and a debris chip for particles.

Writes assets/textures/damage/crack_stage1-3.png and debris.png. Run from the repo root: python3 tools/make_damage_textures.py
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

# Debris: a soft-edged white chip; the particle colour tints it to the object's material.
chip = Image.new("RGBA", (32, 32), (255, 255, 255, 0))
d = ImageDraw.Draw(chip)
d.polygon([(6, 10), (16, 4), (27, 9), (25, 24), (12, 28), (5, 20)], fill=(255, 255, 255, 255))
chip.filter(ImageFilter.GaussianBlur(0.8)).save("assets/textures/damage/debris.png")
