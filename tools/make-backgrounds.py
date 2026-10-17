#!/usr/bin/env python3
"""Writes the PS5 home screen backgrounds, sce_sys/pic0.dds and pic1.dds.

The console wants both files as a single 3840x2160 BC7_UNORM texture in a
DX10 DDS container without mipmaps; the boilerplate's validate-assets.sh
checks exactly that. The source is cropped to 16:9 from the middle and
scaled up, and the same picture is used for both files.

Needs Pillow and etcpak (its BC7 encoder):
    python3 -m venv .venv && .venv/bin/pip install pillow etcpak
    .venv/bin/python tools/make-backgrounds.py assets/background.jpg pkg/PPSA69421/sce_sys
"""
import struct
import sys
from pathlib import Path

import etcpak
from PIL import Image

WIDTH, HEIGHT = 3840, 2160
DXGI_FORMAT_BC7_UNORM = 98
DDS_DIMENSION_TEXTURE2D = 3


def load_16_9(path: Path) -> Image.Image:
    image = Image.open(path).convert("RGBA")
    w, h = image.size
    if w * 9 > h * 16:      # too wide: trim the sides
        new_w = h * 16 // 9
        left = (w - new_w) // 2
        image = image.crop((left, 0, left + new_w, h))
    else:                   # too tall: trim top and bottom
        new_h = w * 9 // 16
        top = (h - new_h) // 2
        image = image.crop((0, top, w, top + new_h))
    return image.resize((WIDTH, HEIGHT), Image.LANCZOS)


def dds_bytes(blocks: bytes) -> bytes:
    flags = 0x1 | 0x2 | 0x4 | 0x1000 | 0x80000   # caps, height, width, pixelformat, linearsize
    pixel_format = struct.pack("<II4s5I", 32, 0x4, b"DX10", 0, 0, 0, 0, 0)
    header = struct.pack("<7I", 124, flags, HEIGHT, WIDTH, len(blocks), 0, 1)
    header += b"\0" * 44 + pixel_format + struct.pack("<5I", 0x1000, 0, 0, 0, 0)
    dx10 = struct.pack("<5I", DXGI_FORMAT_BC7_UNORM, DDS_DIMENSION_TEXTURE2D, 0, 1, 0)
    return b"DDS " + header + dx10 + blocks


def main() -> None:
    if len(sys.argv) != 3:
        raise SystemExit("usage: make-backgrounds.py <image> <sce_sys directory>")
    source, target = Path(sys.argv[1]), Path(sys.argv[2])
    image = load_16_9(source)
    blocks = etcpak.compress_bc7(image.tobytes(), WIDTH, HEIGHT)
    expected = WIDTH // 4 * (HEIGHT // 4) * 16
    if len(blocks) != expected:
        raise SystemExit(f"BC7 encoder returned {len(blocks)} bytes, expected {expected}")
    data = dds_bytes(blocks)
    for name in ("pic0.dds", "pic1.dds"):
        (target / name).write_bytes(data)
        print(f"wrote {target / name} ({len(data)} bytes)")


if __name__ == "__main__":
    main()
