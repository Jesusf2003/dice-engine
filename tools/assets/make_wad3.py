"""Builds a WAD3 (Half-Life style, per-texture 256 colour palette) holding one texture,
plus a full colour PNG that FTE loads instead of it in game.

usage: make_wad3.py <image> <texname> <out.wad> <out.png> <width> <height>
width/height must be multiples of 16 (Quake miptex requirement).
"""
import struct
import sys

from PIL import Image

src, texname, out_wad, out_png, width, height = sys.argv[1:7]
width, height = int(width), int(height)
assert width % 16 == 0 and height % 16 == 0, "miptex sizes must be multiples of 16"
assert len(texname) <= 15, "wad names are 16 bytes including the terminator"

img = Image.open(src).convert("RGB")

# full quality copy for the engine (same aspect ratio as the miptex, any resolution works)
img.save(out_png)

# 8-bit version: one adaptive palette shared by all four mip levels
base = img.resize((width, height), Image.LANCZOS)
pal_img = base.quantize(colors=256, method=Image.Quantize.MEDIANCUT, dither=Image.Dither.FLOYDSTEINBERG)
palette = pal_img.getpalette()[:256 * 3]
palette += [0] * (256 * 3 - len(palette))

mips = [pal_img.tobytes()]
for level in (1, 2, 3):
    small = img.resize((width >> level, height >> level), Image.LANCZOS)
    mips.append(small.quantize(palette=pal_img, dither=Image.Dither.FLOYDSTEINBERG).tobytes())

# miptex lump: header, 4 mips, palette count + palette, padded to 4 bytes
name16 = texname.encode("ascii").ljust(16, b"\0")
offsets, ofs = [], 40
for m in mips:
    offsets.append(ofs)
    ofs += len(m)
lump = name16 + struct.pack("<II4I", width, height, *offsets) + b"".join(mips)
lump += struct.pack("<H", 256) + bytes(palette)
lump += b"\0" * (-len(lump) % 4)

# wad: header, lump data, directory
header_size = 12
dir_ofs = header_size + len(lump)
wad = struct.pack("<4sii", b"WAD3", 1, dir_ofs) + lump
wad += struct.pack("<iiibbh", header_size, len(lump), len(lump), 0x43, 0, 0) + name16

with open(out_wad, "wb") as f:
    f.write(wad)
print(f"{out_wad}: {texname} {width}x{height}, {len(wad)} bytes; {out_png}: {img.size[0]}x{img.size[1]}")
