#!/usr/bin/env python3
"""Draws the port's own home screen art: assets/icon.png (the tile) and assets/background.png.
Plain shapes and a plain typeface, nothing taken from the game's box or logo."""
import sys
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont, ImageFilter

FONT = "/usr/share/fonts/truetype/dejavu/DejaVuSansCondensed-Bold.ttf"
out = Path(sys.argv[1])

def art(w, h, title_size, sub_size, sub="PS5"):
    img = Image.new("RGB", (w, h), (8, 10, 16))
    d = ImageDraw.Draw(img)
    # a cold vertical gradient
    for y in range(h):
        t = y / h
        d.line([(0, y), (w, y)], fill=(int(8 + 10 * t), int(12 + 14 * t), int(22 + 30 * t)))
    # a red slash across the lower third, blurred into a glow
    glow = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    g = ImageDraw.Draw(glow)
    g.polygon([(0, h * 0.70), (w, h * 0.52), (w, h * 0.58), (0, h * 0.76)], fill=(200, 20, 30, 255))
    img.paste(glow.filter(ImageFilter.GaussianBlur(w / 60)), (0, 0), glow.filter(ImageFilter.GaussianBlur(w / 60)))
    d.polygon([(0, h * 0.72), (w, h * 0.54), (w, h * 0.555), (0, h * 0.735)], fill=(235, 40, 45))
    f = ImageFont.truetype(FONT, title_size)
    s = ImageFont.truetype(FONT, sub_size)
    lines = ["PERFECT", "DARK"]
    y = h * 0.16
    for line in lines:
        tw = d.textlength(line, font=f)
        d.text(((w - tw) / 2, y), line, font=f, fill=(232, 236, 242))
        y += title_size * 1.02
    tw = d.textlength(sub, font=s)
    d.text(((w - tw) / 2, h * 0.80), sub, font=s, fill=(150, 160, 175))
    return img

out.mkdir(parents=True, exist_ok=True)
art(1024, 1024, 190, 70).save(out / "icon.png")
art(1920, 1080, 210, 60, "DAB'S MOD  ·  PS5").save(out / "background.png")
