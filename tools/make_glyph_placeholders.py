#!/usr/bin/env python3
"""PLACEHOLDER button glyphs, 64x64 PNGs with alpha, until real art arrives.

Writes assets/textures/glyphs/{kb,xbox,ps}/*.png: keyboard keycaps and mouse buttons, Xbox face buttons in their colours,
PlayStation face-button symbols, D-pads with one arm lit, and shoulder/trigger plates.
Run from the repo root: python3 tools/make_glyph_placeholders.py
"""
import os
from PIL import Image, ImageDraw, ImageFont

SIZE = 64
FONT = "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf"
ROOT = "assets/textures/glyphs"


def font(size):
    return ImageFont.truetype(FONT, size)


def centred(draw, text, size, fill, box=(0, 0, SIZE, SIZE), dy=0):
    f = font(size)
    l, t, r, b = draw.textbbox((0, 0), text, font=f)
    x = box[0] + (box[2] - box[0] - (r - l)) / 2 - l
    y = box[1] + (box[3] - box[1] - (b - t)) / 2 - t + dy
    draw.text((x, y), text, font=f, fill=fill)


def save(image, folder, name):
    os.makedirs(os.path.join(ROOT, folder), exist_ok=True)
    image.save(os.path.join(ROOT, folder, name + ".png"))


def keycap(label, size):
    """A light keycap with a darker lip, the key's legend in dark ink."""
    image = Image.new("RGBA", (SIZE, SIZE), (0, 0, 0, 0))
    d = ImageDraw.Draw(image)
    d.rounded_rectangle([4, 6, 60, 60], 9, fill=(146, 140, 130, 255))
    d.rounded_rectangle([4, 4, 60, 54], 9, fill=(238, 235, 228, 255))
    centred(d, label, size, (30, 28, 25, 255), (4, 4, 60, 54))
    return image


def mouse(side):
    """A mouse body with the left or right button lit."""
    image = Image.new("RGBA", (SIZE, SIZE), (0, 0, 0, 0))
    d = ImageDraw.Draw(image)
    d.rounded_rectangle([14, 4, 50, 60], 18, fill=(238, 235, 228, 255), outline=(60, 56, 50, 255), width=3)
    lit = [16, 6, 32, 28] if side == "left" else [32, 6, 48, 28]
    d.rectangle(lit, fill=(212, 168, 67, 255))
    d.line([32, 6, 32, 28], fill=(60, 56, 50, 255), width=3)
    d.line([16, 28, 48, 28], fill=(60, 56, 50, 255), width=3)
    return image


def face(draw_symbol):
    """A dark round face button; the symbol is drawn on top."""
    image = Image.new("RGBA", (SIZE, SIZE), (0, 0, 0, 0))
    d = ImageDraw.Draw(image)
    d.ellipse([3, 3, 61, 61], fill=(32, 32, 36, 255), outline=(90, 90, 96, 255), width=3)
    draw_symbol(d)
    return image


def dpad(arm):
    """A cross-shaped D-pad on a dark plate with one arm lit."""
    image = Image.new("RGBA", (SIZE, SIZE), (0, 0, 0, 0))
    d = ImageDraw.Draw(image)
    d.rounded_rectangle([2, 2, 62, 62], 12, fill=(32, 32, 36, 255))
    d.rectangle([24, 8, 40, 56], fill=(110, 110, 116, 255))
    d.rectangle([8, 24, 56, 40], fill=(110, 110, 116, 255))
    arms = {"up": [24, 8, 40, 24], "down": [24, 40, 40, 56], "left": [8, 24, 24, 40], "right": [40, 24, 56, 40]}
    d.rectangle(arms[arm], fill=(245, 245, 245, 255))
    return image


def plate(label):
    """A shoulder or trigger: a dark plate, rounded on top, with its name."""
    image = Image.new("RGBA", (SIZE, SIZE), (0, 0, 0, 0))
    d = ImageDraw.Draw(image)
    d.rounded_rectangle([3, 12, 61, 52], 12, fill=(32, 32, 36, 255), outline=(90, 90, 96, 255), width=3)
    centred(d, label, 22, (235, 235, 235, 255), (3, 12, 61, 52))
    return image


# Keyboard and mouse.
for key, label, size in [("e", "E", 32), ("q", "Q", 32), ("tab", "Tab", 20), ("esc", "Esc", 20),
                         ("1", "1", 32), ("2", "2", 32), ("3", "3", 32), ("4", "4", 32)]:
    save(keycap(label, size), "kb", "key_" + key)
save(mouse("left"), "kb", "mouse_left")
save(mouse("right"), "kb", "mouse_right")

# Xbox: letters in the pad's colours.
for name, colour in [("a", (96, 190, 70)), ("b", (225, 70, 60)), ("x", (60, 130, 225)), ("y", (240, 200, 50))]:
    save(face(lambda d, n=name, c=colour: centred(d, n.upper(), 30, c + (255,))), "xbox", name)
for arm in ["up", "down", "left", "right"]:
    save(dpad(arm), "xbox", "dpad_" + arm)
    save(dpad(arm), "ps", "dpad_" + arm)
for name in ["lb", "rb", "lt", "rt"]:
    save(plate(name.upper()), "xbox", name)

# PlayStation: the four symbols in their colours.
def cross(d):
    d.line([20, 20, 44, 44], fill=(110, 150, 230, 255), width=6)
    d.line([44, 20, 20, 44], fill=(110, 150, 230, 255), width=6)

def circle(d):
    d.ellipse([19, 19, 45, 45], outline=(230, 90, 90, 255), width=6)

def square(d):
    d.rectangle([20, 20, 44, 44], outline=(230, 130, 200, 255), width=6)

def triangle(d):
    d.polygon([(32, 17), (47, 43), (17, 43)], outline=(80, 200, 160, 255), width=6)

for name, symbol in [("cross", cross), ("circle", circle), ("square", square), ("triangle", triangle)]:
    save(face(symbol), "ps", name)
for name in ["l1", "r1", "l2", "r2"]:
    save(plate(name.upper()), "ps", name)
