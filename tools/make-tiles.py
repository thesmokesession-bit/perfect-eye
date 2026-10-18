#!/usr/bin/env python3
"""The two titles' home screen art from the port's own square key art (assets/tiles, made for
the port in the mood of the original covers: no character, logo or wordmark of either game):
icon0.png with the title set over it, and a 16:9 background of the same picture.
    tools/make-tiles.py pd assets/tiles/perfect-dark.png sce_sys
    tools/make-tiles.py ge assets/tiles/goldeneye.png sce_sys_ge
    tools/make-tiles.py box assets/boxart/perfect-dark.png sce_sys   (the box cover, untitled)
then tools/make-backgrounds.py <out>/background.png <out> for pic0.dds and pic1.dds."""
import sys
from pathlib import Path
from PIL import Image, ImageDraw, ImageEnhance, ImageFilter, ImageFont

STYLES = {
    # cold chrome over the black at the top, as the late-90s sci-fi covers set their titles
    "pd": dict(text="PERFECT DARK", font="/usr/share/texmf/fonts/opentype/public/tex-gyre/texgyreadventor-bold.otf",
               top=True, colors=[(235, 245, 255), (120, 180, 255), (20, 60, 170), (150, 200, 255)], tracking=0.10),
    # gold serif on the dark ground at the bottom, as the 90s spy posters did
    "ge": dict(text="GOLDENEYE", font="/usr/share/texmf/fonts/opentype/public/tex-gyre/texgyrebonum-bold.otf",
               top=False, colors=[(255, 244, 190), (232, 180, 40), (150, 92, 10), (245, 205, 90)], tracking=0.06),
}


def gradient(w, h, stops):
    g = Image.new("RGB", (w, h))
    d = ImageDraw.Draw(g)
    for y in range(h):
        t = y / max(1, h - 1) * (len(stops) - 1)
        i = min(int(t), len(stops) - 2)
        f = t - i
        c = tuple(int(stops[i][k] * (1 - f) + stops[i + 1][k] * f) for k in range(3))
        d.line([(0, y), (w, y)], fill=c)
    return g


def title(img, style, scale):
    w, h = img.size
    tracking = style["tracking"]
    size = int(110 * scale)
    font = ImageFont.truetype(style["font"], size)
    probe = ImageDraw.Draw(img)

    def width(f):
        return sum(probe.textlength(c, font=f) for c in style["text"]) + tracking * f.size * (len(style["text"]) - 1)

    while width(font) > w * 0.88:
        size -= 2
        font = ImageFont.truetype(style["font"], size)
    x0 = (w - width(font)) / 2
    asc, desc = font.getmetrics()
    y0 = h * 0.07 if style["top"] else h - h * 0.07 - asc - desc
    mask = Image.new("L", (w, h), 0)
    md = ImageDraw.Draw(mask)
    x = x0
    for c in style["text"]:
        md.text((x, y0), c, font=font, fill=255)
        x += probe.textlength(c, font=font) + tracking * size
    # a dark glow, a thin light rim, then the metal
    glow = mask.filter(ImageFilter.MaxFilter(int(9 * scale) | 1)).filter(ImageFilter.GaussianBlur(10 * scale))
    img = Image.composite(Image.new("RGB", (w, h), (0, 0, 0)), img, glow.point(lambda v: int(v * 0.85)))
    rim = mask.filter(ImageFilter.MaxFilter(max(3, int(3 * scale) | 1)))
    img = Image.composite(Image.new("RGB", (w, h), (255, 255, 255)), img, rim)
    box = mask.getbbox()
    metal = Image.new("RGB", (w, h))
    metal.paste(gradient(w, box[3] - box[1], style["colors"]), (0, box[1]))
    return Image.composite(metal, img, mask)


def main():
    # "box": a cover that carries its own title (assets/boxart), set as it is
    style, src, out = STYLES.get(sys.argv[1]), Path(sys.argv[2]), Path(sys.argv[3])
    if style is None:
        global title
        title = lambda img, style, scale: img
    art = Image.open(src).convert("RGB")
    icon = title(art.resize((1024, 1024), Image.LANCZOS), style, 2.0).resize((512, 512), Image.LANCZOS)
    icon.save(out / "icon0.png")
    # the background: the titled picture at full height in the middle, a blurred, darkened zoom
    # of it filling the sides
    W, H = 3840, 2160
    titled = title(art.resize((H, H), Image.LANCZOS), style, H / 512)
    bg = art.resize((W, W), Image.LANCZOS).crop((0, (W - H) // 2, W, (W - H) // 2 + H))
    bg = ImageEnhance.Brightness(bg.filter(ImageFilter.GaussianBlur(60))).enhance(0.45)
    edge = Image.new("L", (H, H), 0)
    ImageDraw.Draw(edge).rectangle([80, 0, H - 80, H], fill=255)
    bg.paste(titled, ((W - H) // 2, 0), edge.filter(ImageFilter.GaussianBlur(50)))
    bg.save(out / "background.png")
    print(f"make-tiles.py: {out}/icon0.png and {out}/background.png ({style['text'] if style else 'as it is'})")


main()
