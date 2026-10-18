#!/usr/bin/env python3
"""The home screen tile for a title: icon0.png (512x512) from square art with the title set in
its dark top third, in a plain typeface (nothing taken from either game's logo). The art is
original (assets/tiles, made for the port). The backgrounds are tools/make-backgrounds.py's.
    tools/make-tiles.py assets/tiles/pd-icon.png "PERFECT DARK" sce_sys/icon0.png
"""
import sys
from PIL import Image, ImageDraw, ImageFilter, ImageFont

FONT = "/usr/share/fonts/opentype/inter/InterDisplay-Bold.otf"
src, title, out = sys.argv[1], sys.argv[2], sys.argv[3]
S = 512
img = Image.open(src).convert("RGB").resize((S, S), Image.LANCZOS)

# a soft dark band behind the title so it reads on any sky
band = Image.new("L", (S, S), 0)
ImageDraw.Draw(band).rectangle([0, 0, S, int(S * 0.30)], fill=150)
band = band.filter(ImageFilter.GaussianBlur(40))
img = Image.composite(Image.new("RGB", (S, S), (0, 0, 0)), img, band)

d = ImageDraw.Draw(img)
size = 64
spacing = 0.18  # tracking, in ems
def width(f):
    return sum(d.textlength(c, font=f) for c in title) + spacing * f.size * (len(title) - 1)
f = ImageFont.truetype(FONT, size)
while width(f) > S * 0.86:
    size -= 2
    f = ImageFont.truetype(FONT, size)
x = (S - width(f)) / 2
y = S * 0.075
# shadow, then the letters
shadow = Image.new("L", (S, S), 0)
sd = ImageDraw.Draw(shadow)
cx = x
for c in title:
    sd.text((cx + 2, y + 3), c, font=f, fill=200)
    cx += d.textlength(c, font=f) + spacing * size
img = Image.composite(Image.new("RGB", (S, S), (0, 0, 0)), img, shadow.filter(ImageFilter.GaussianBlur(4)))
d = ImageDraw.Draw(img)
cx = x
for c in title:
    d.text((cx, y), c, font=f, fill=(238, 240, 245))
    cx += d.textlength(c, font=f) + spacing * size
img.save(out)
print(f"make-tiles.py: {out} ({title}, {size} px)")
