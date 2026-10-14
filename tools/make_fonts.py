import math
import os
import sys

from PIL import Image, ImageDraw, ImageFont

PLEX = "/usr/share/fonts/truetype/ibm-plex/IBMPlexSans-%s.ttf"
DEJAVU = "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"
OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "assets", "fonts")

FONTS = [
    ("title", "SemiBold", 54, 3),
    ("heading", "SemiBold", 34, 1),
    ("button", "Medium", 18, 1),
    ("body", "Regular", 15, 0),
    ("label", "SemiBold", 12, 2),
]

TOP_LEFT = (255, 0, 0, 255)
LOWER_RIGHT = (255, 255, 0, 255)
SHEET_WIDTH = 1024


def glyphs():
    return [chr(code) for code in range(32, 127)] + ["✓"]


def make(name, weight, size, tracking):
    face = ImageFont.truetype(PLEX % weight, size)
    check = ImageFont.truetype(DEJAVU, size)
    ascent, descent = face.getmetrics()
    height = ascent + descent

    cells = []
    for ch in glyphs():
        font = check if ch == "✓" else face
        cells.append((ch, font, int(math.ceil(font.getlength(ch))) + tracking))

    rows = [[]]
    x = 0
    for cell in cells:
        if x + cell[2] + 1 >= SHEET_WIDTH:
            rows.append([])
            x = 0
        rows[-1].append(cell)
        x += cell[2]

    image = Image.new("RGBA", (SHEET_WIDTH, len(rows) * (height + 1) + 1), (0, 0, 0, 0))
    ink = Image.new("L", image.size, 0)
    draw = ImageDraw.Draw(ink)
    y = 0
    for row in rows:
        x = 0
        for ch, font, width in row:
            draw.text((x + tracking // 2, y + ascent), ch, font=font, fill=255, anchor="ls")
            x += width
        y += height + 1
    image.putalpha(ink)
    white = Image.new("RGBA", image.size, (255, 255, 255, 255))
    white.putalpha(ink)
    image = white

    y = 0
    for row in rows:
        x = 0
        for ch, font, width in row:
            image.putpixel((x, y), TOP_LEFT)
            image.putpixel((x + width, y + height), LOWER_RIGHT)
            x += width
        y += height + 1
    image.putpixel((1, 0), LOWER_RIGHT)
    image.putpixel((2, 0), (0, 0, 0, 0))
    image.save(os.path.join(OUT, name + ".png"))
    return height


if __name__ == "__main__":
    for spec in FONTS:
        print(spec[0], make(*spec), file=sys.stdout)
