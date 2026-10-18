#!/usr/bin/env python3
"""A square box cover as the home screen's 16:9 background: the cover at full height in the
middle, a blurred, darkened zoom of it filling the sides. Then tools/make-backgrounds.py:
    tools/boxart-background.py assets/boxart/perfect-dark.png /tmp/bg.png
    tools/make-backgrounds.py /tmp/bg.png sce_sys
(icon0.png is the cover at 512x512.)"""
import sys
from PIL import Image, ImageDraw, ImageEnhance, ImageFilter

W, H = 3840, 2160
art = Image.open(sys.argv[1]).convert("RGB")
bg = art.resize((W, W), Image.LANCZOS).crop((0, (W - H) // 2, W, (W - H) // 2 + H))
bg = ImageEnhance.Brightness(bg.filter(ImageFilter.GaussianBlur(60))).enhance(0.45)
edge = Image.new("L", (H, H), 0)
ImageDraw.Draw(edge).rectangle([80, 0, H - 80, H], fill=255)
bg.paste(art.resize((H, H), Image.LANCZOS), ((W - H) // 2, 0), edge.filter(ImageFilter.GaussianBlur(50)))
bg.save(sys.argv[2])
